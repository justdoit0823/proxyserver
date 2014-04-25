/*

  Author: Just do it.
  Email: justdoit920823@gmail.com
  Website: http://www.shareyou.net.cn/

*/

#include "fpoll.h"
#include "list.h"
#include "stdio.h"
#include "string.h"
#include "sys/socket.h"
#include "stdlib.h"
#include "fcntl.h"
#include "netinet/in.h"
#include "errno.h"
#include "connection.h"

static List * handlelist, * freeProxylist, * freeDispatchlist;

static struct DispatchList * dispatchlist = NULL;

static struct DispatchList * freedispatchs = NULL;

int initepoll(int size)
{
  //int fd;
  int i;
  handlelist = NULL;
  freeProxylist = NULL;
  freeDispatchlist = NULL;
  for(i=0; i < MAXFD; i++) connections[i] = NULL;
  epfd = epoll_create(size);
  return epfd;
}

int addtoepoll(int efd, int fd, int events)
{
  struct epoll_event ev;
  ev.data.fd = fd;
  ev.events = events;
  return epoll_ctl(efd, EPOLL_CTL_ADD, fd, &ev);
}

int editepoll(int efd, int fd, int events)
{
  struct epoll_event ev;
  ev.data.fd = fd;
  ev.events = events;
  return epoll_ctl(efd, EPOLL_CTL_MOD, fd, &ev);
}

int rmfromepoll(int efd, int fd)
{
  struct epoll_event ev;
  return epoll_ctl(efd, EPOLL_CTL_DEL, fd, &ev);
}

int setnonblock(int fd)
{
  int flag;
  flag = fcntl(fd,F_GETFL);
  flag |= O_NONBLOCK;
  return fcntl(fd,F_SETFL,flag);
}

int handleaccept(int efd, int fd)
{
  char logbuf[256];
  char addrbuf[64];
  struct sockaddr_in sain;
  socklen_t sl = sizeof(sain);
  int requestfd;
  while(1){
    requestfd = accept(fd,(struct sockaddr *)&sain,&sl);
    if(requestfd == -1){
      if(errno == EINTR) continue;
      else break;
    }
    else{
      inet_ntop(AF_INET, &sain.sin_addr, addrbuf, sizeof(addrbuf));
      sprintf(logbuf, "recieve a request from %s.\n", addrbuf);
      loginfo(logbuf);
      setnonblock(requestfd);
      if(addtoepoll(efd, requestfd, ETINOUT) != 0){
	logwarn("epoll add request fd error.\n");
	close(requestfd);
	continue;
      }
    }
  }
  return 0;
}

void runepoll(int epfd, int listenfd)
{
  struct epoll_event ev;
  struct epoll_event events[EVENTMAX];
  int i,nfds,requestfd,timeout;
  char logbuf[256];
  List * pcon, * pnext, * nl;;
  ListItem * pitem;
  struct HTTPConnection * tempcon;
  struct ProxyConnection * proxycon;
  int eventfd, backevents;
  struct ConnectManager * mainmanager;
  initConnectManager();
  mainmanager = GetManager();
  if(addtoepoll(epfd, listenfd, ETIN) != 0){
    logerr("epoll add listen fd error", errno);
    exit(-1);
  }
  timeout = 200;
  while(1){
    nfds = epoll_wait(epfd, events, EVENTMAX, timeout);
    if(nfds == -1) logerr("epoll wait error", errno);
    for(i=0; i < nfds; i++){
      eventfd = events[i].data.fd;
      backevents = events[i].events;
      if(eventfd == listenfd){
	handleaccept(epfd, listenfd);
      }
      else{
	if(mainmanager->conlist[eventfd] != NULL){
	  tempcon = (struct HTTPConnection *)(mainmanager->conlist[eventfd]);
	  tempcon->events |= backevents;
	}
	else{
	  if(freeProxylist == NULL){
	    proxycon = newProxyConnect(eventfd, backevents, handlehost);
	    nl = newListNode();
	    nl->item = (struct HTTPConnection *)proxycon;
	  }
	  else{
	    nl = freeProxylist;
	    proxycon = (struct ProxyConnection *)nl->item;
	    rmFromList(&freeProxylist, nl);
	    proxycon->con.fd = eventfd;
	    proxycon->con.events = backevents;
	    proxycon->con.fun = handlehost;
	  }
	  AddConnectToManager(eventfd, proxycon);
	  addToList(&handlelist, nl);
	}
      }
    }
    for(pcon=handlelist; pcon != NULL; pcon = pnext){
      pitem = pcon->item;
      pnext = pcon->next;
      (pitem->fun)(pitem);
    }
  }
}

int handlehost(struct HTTPConnection * p)
{
  int dispatchfd,hssum,hrsum,sendnum;
  struct epoll_event reqev;
  struct ProxyConnection * proxy;
  struct DispatchConnection * dispatch;
  proxy = (struct ProxyConnection *)p;
  int parseResult, proxyclosed;
  char requestbuf[1024];
  if(proxy == NULL){
    logwarn("invalid handle pointer.\n");
    return -1;
  }
  if(proxy->con.events & EPOLLIN){
    if(proxy->con.buf == NULL){
      proxy->con.buf = initHTTPBuffer(4096*4);
      if(proxy->con.buf == NULL){
	return -1;
      }
    }
    proxyclosed = 0;
    hrsum = recvfromhost(proxy->con.fd, proxy->con.buf->rdBuf + proxy->con.buf->bufferedBytes, &proxyclosed);
    if(hrsum > 0){
      proxy->con.events &= ~EPOLLIN;
      printf("recv %d bytes from host.\n", hrsum);
      proxy->con.buf->bufferedBytes += hrsum;
      if(proxy->header.parsePass != 1){
	parseResult = parseRequest(proxy->con.buf->rdBuf, &(proxy->header));
	printf("request's host is %s.\n", proxy->header.host);
	if(parseResult == 0){
	  printf("http header length is %d.\n", proxy->header.length);
	  printf("http uri is %s.\n", proxy->header.uri);
	}
      }
      if(proxy->dispatch == NULL){
	if(proxy->header.host[0] != '\0'){
	  if((dispatch = FindDispatchConnect(dispatchlist, proxy->header.host, PROXY_FREE)) != NULL){
	    /*
	      The dispatch connection to the specified host already existed.
	    */

	    printf("host %s connection already existed.\n", proxy->header.host);
	    dispatch->proxy = proxy;
	    proxy->dispatch = dispatch;
	    dispatch->status = PROXY_USED;
	  }
	  else{
	    dispatchfd = NULLFD;
	    if(connecthost(proxy->header.host, proxy->header.port, &dispatchfd) == -1){
	      if(dispatchfd != NULLFD){
		close(dispatchfd);
		rmfromepoll(epfd, proxy->con.fd);
		close(proxy->con.fd);
		RmConnectFromManager(proxy->con.fd);
		proxy->con.events = 0;
		proxy->con.fd = NULLFD;
	      }
	      return -1;
	    }
	    printf("connect success.\n");
	    setnonblock(dispatchfd);
	    if(addtoepoll(epfd, dispatchfd, ETINOUT) !=0 ){
	      logerr("add remote fd to epoll error", errno);
	      return -1;
	    }
	    List * pl;
	    struct DispatchConnection * dispatchcon;
	    if(freeDispatchlist == NULL){
	      pl = newListNode();
	      dispatchcon = newDispatchConnect(dispatchfd, handleremote, proxy);
	    }
	    else{
	      pl = freeDispatchlist;
	      rmFromList(&freeDispatchlist, pl);
	      dispatchcon = (struct DispatchConnection *)(pl->item);
	      dispatchcon->con.fd = dispatchfd;
	      dispatchcon->con.fun = handleremote;
	      dispatchcon->proxy = proxy;
	    }
	    proxy->dispatch = dispatchcon;
	    AddConnectToManager(dispatchfd, dispatchcon);
	    struct DispatchList * dl;
	    if(freedispatchs == NULL){
	      dl = malloc(sizeof(*dl));
	    }
	    else{
	      dl = freedispatchs;
	      rmFromDispatchlist(&freedispatchs, dl);
	    }
	    dl->dispatch = dispatchcon;
	    AddDispatchConnect(&dispatchlist, dl);
	    pl->item = (struct HTTPConnection *)dispatchcon;
	    addToList(&handlelist, pl);
	  }
	}
      }
      if(proxy->header.parsePass == 1 && proxy->con.buf->bufferedBytes >= proxy->header.length){
	notifyProxyRequest(proxy);
      }
    }
    if(proxy->dispatch == NULL && proxyclosed == 1){
      rmfromepoll(epfd, proxy->con.fd);
      close(proxy->con.fd);
      RmConnectFromManager(proxy->con.fd);
      proxy->con.events = 0;
      proxy->con.fd = NULLFD;
      dispatch = proxy->dispatch;
      if(dispatch != NULL && dispatch->con.fd != NULLFD){
	dispatch->proxy = NULL;
      }
      List * node = findListNode(handlelist, (struct HTTPConnection *)proxy);
      rmFromList(&handlelist, node);
      addToList(&freeProxylist, node);
    }
  }
  if(proxy->con.events & EPOLLOUT){
    dispatch = proxy->dispatch;
    if(proxy->con.buf != NULL && proxy->con.buf->wrBuf != NULL){
      sendnum = proxy->con.buf->outBytes;
      if(proxy->con.buf->wrReady == 1 && sendnum > 0){
	proxyclosed = 0;
	hssum = sendtohost(proxy->con.fd, proxy->con.buf->wrBuf, sendnum, &proxyclosed);
	if(hssum == sendnum){
	  printf("send %d bytes to host.\n", hssum);
	  sprintf(requestbuf, "%d %s %s %s.\n", dispatch->header.status, proxy->header.method, proxy->header.uri, proxy->header.version);
	  loginfo(requestbuf);
	  cleanDispatchResponse(proxy->dispatch);
	}
	else{
	  proxy->con.events &= ~EPOLLOUT;
	  printf("send error and bytes %d.\n", hssum);
	}
      }
    }
  }
  return 0;
}


int handleremote(struct HTTPConnection * p)
{
  int rssum,rrsum,sendnum;
  struct ProxyConnection * proxy;
  struct DispatchConnection * dispatch;
  dispatch = (struct DispatchConnection *)p;
  int parseResult, dispatchclosed;
  if(dispatch == NULL) return -1;
  if(dispatch->con.buf == NULL){
    dispatch->con.buf = initHTTPBuffer(4096*32);
    if(dispatch->con.buf == NULL){
      logerr("can't alloc memory", errno);
      return -1;
    }
  }
  if(dispatch->con.events & EPOLLOUT){
    if(dispatch->con.buf != NULL && dispatch->con.buf->wrBuf != NULL){
      sendnum = dispatch->con.buf->outBytes;
      if(dispatch->con.buf->wrReady == 1 && sendnum > 0){
	dispatchclosed = 0;
	rssum = sendtohost(dispatch->con.fd, dispatch->con.buf->wrBuf, sendnum, &dispatchclosed);
	if(rssum == sendnum){
	  printf("send %d bytes to remote.\n", rssum);
	  cleanProxyRequest(dispatch->proxy);
	}
	else{
	  dispatch->con.events &= ~EPOLLOUT;
	  printf("send error and bytes %d.\n", rssum);
	}
      }
    }
  }
  if((dispatch->con.events & EPOLLIN)){
    char * bufpos = dispatch->con.buf->rdBuf + dispatch->con.buf->bufferedBytes;
    dispatchclosed = 0;
    proxy = dispatch->proxy;
    rrsum = recvfromhost(dispatch->con.fd, bufpos, &dispatchclosed);
    if(rrsum > 0){
      dispatch->con.events &= ~EPOLLIN;
      dispatch->con.buf->bufferedBytes += rrsum;
      printf("recv %d bytes from remote,total is %d.\n", rrsum, dispatch->con.buf->bufferedBytes);
      if(dispatch->header.parsePass != 1){
	parseResponse(dispatch->con.buf->rdBuf, dispatch->con.buf->bufferedBytes, &(dispatch->header));
      }
      else printf("http response length is %d.\n", dispatch->header.length);
      if(dispatch->header.parsePass == 1 && DispatchResulted(dispatch)){
	notifyDispatchResponse(dispatch);
      }
    }
    if(dispatchclosed == 1){
      /* remote server closed the conenction
	 In the http 1.0,the server would close the connection and indicate 
	 that the response is over.
      */

      /*if(strcasecmp(dispatch->header.version, "HTTP/1.0") == 0){
	printf("request %s is finished.\n", proxy->header.uri);
	notifyDispatchResponse(dispatch);
	}*/
      if(dispatch->con.buf->rdEOF != 1) notifyDispatchResponse(dispatch);
      rmfromepoll(epfd, dispatch->con.fd);
      close(dispatch->con.fd);
      RmConnectFromManager(dispatch->con.fd);
      dispatch->con.events = 0;
      dispatch->con.fd = NULLFD;
      List * node = findListNode(handlelist, (struct HTTPConnection *)dispatch);
      rmFromList(&handlelist, node);
      addToList(&freeDispatchlist, node);
      struct DispatchList * dispatchnode = findDispatchNode(dispatchlist, dispatch);
      rmFromDispatchlist(&dispatchlist, dispatchnode);
      AddDispatchConnect(&freedispatchs, dispatchnode);
    }
  }
  return 0;
}
