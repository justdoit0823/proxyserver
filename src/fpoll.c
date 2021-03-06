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

static List * proxylist = NULL; /* active http proxy connection list */

/* 
   inactive http proxy connection list,
   which can be reused in later http proxy conenction.
*/

static List * freeProxylist = NULL; 


/* 
   inactive http dispatch connection list,
   which can be reused in later http dispatch conenction.
*/

static List * freeDispatchlist = NULL;

static List * dispatchlist = NULL; /* active http dispatch connection list */


int initepoll(int size)
{
  //int fd;
  int i;
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
  List * pcon, * pnext;
  ListItem * pitem;
  struct HTTPConnection * tempcon;
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
	  List * nl;
	  struct ProxyConnection * proxycon;
	  nl = getProxyResource(&freeProxylist, eventfd, backevents, handlehost);
	  proxycon = (struct ProxyConnection *)nl->item;
	  AddConnectToManager(eventfd, proxycon);
	  addToList(&proxylist, nl);
	}
      }
    }
    /*
      do the proxy list loop

    */

    for(pcon = proxylist; pcon != NULL; pcon = pnext){
      pitem = pcon->item;
      pnext = pcon->next;
      (pitem->fun)(pitem);
    }

    /*
      do the dispatch list loop

    */
    
    for(pcon = dispatchlist; pcon != NULL; pcon = pnext){
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
  char * readBuf;
  proxy = (struct ProxyConnection *)p;
  int parseResult, proxyclosed;
  char requestbuf[1024];
  if(proxy == NULL){
    logwarn("invalid handle pointer.\n");
    return -1;
  }
  /*
    check the proxy buffer, allocate if buffer is NULL.

  */
  if(proxy->con.buf == NULL){
    proxy->con.buf = initHTTPBuffer(4096*4);
    if(proxy->con.buf == NULL){
      logerr("allocate for proxy buffer.\n");
      return -1;
    }
  }
  if(proxy->con.events & EPOLLIN){
    proxyclosed = 0;
    proxy->con.events &= ~EPOLLIN;
    readBuf = proxy->con.buf->rdBuf + proxy->con.buf->bufferedBytes;
    hrsum = recvfromhost(proxy->con.fd, readBuf, &proxyclosed);
    if(hrsum > 0){
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
	  if((dispatch = FindDispatchConnect(dispatchlist, proxy->header.host, PROXY_FINISHED)) != NULL){
	    /*
	      The dispatch connection to the specified host already existed.
	    */

	    printf("host %s connection already existed.\n", proxy->header.host);
	    attachProxyDispatch(proxy, dispatch);
	    dispatch->status = PROXY_USED;
	  }
	  else{
	    dispatchfd = NULLFD;
	    if(connecthost(proxy->header.host, proxy->header.port, &dispatchfd) == -1){
	      if(dispatchfd != NULLFD){
		close(dispatchfd);
		rmfromepoll(epfd, proxy->con.fd);
		freeProxyConnect(proxy);
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
	    pl = getDispatchResource(&freeDispatchlist, dispatchfd, handleremote);
	    dispatchcon = (struct DispatchConnection *)(pl->item);
	    AddConnectToManager(dispatchfd, dispatchcon);
	    addToList(&dispatchlist, pl);
	    strcpy(dispatchcon->host, proxy->header.host);
	    attachProxyDispatch(proxy, dispatchcon);
	  }
	}
      }
      if(proxy->header.parsePass == 1 && proxy->con.buf->bufferedBytes >= proxy->header.length){
	notifyProxyRequest(proxy);
      }
    }
    if(proxyclosed == 1){
      /*
	avoid web browser open a connection, but send no data and close after.

      */

      if(proxy->con.buf->bufferedBytes == 0) proxy->status = PROXY_FREE; 
      else if(proxy->status == PROXY_USED) proxy->status = PROXY_CLOSED;
      else if(proxy->status == PROXY_FINISHED) proxy->status = PROXY_FREE;
    }
  }
  if(proxy->con.events & EPOLLOUT){
    dispatch = proxy->dispatch;
    if(proxy->con.buf != NULL && proxy->con.buf->wrBuf != NULL){
      /*
	when the dispatch response is recived and the socket is ready for write,
	proxy sends the response to host.

      */

      sendnum = proxy->con.buf->outBytes;
      if(proxy->con.buf->wrReady == 1 && sendnum > 0){
	proxyclosed = 0;
	hssum = sendtohost(proxy->con.fd, proxy->con.buf->wrBuf, sendnum, &proxyclosed);
	if(hssum == sendnum){
	  printf("send %d bytes to host.\n", hssum);
	  sprintf(requestbuf, "%d %s %s %s.\n", dispatch->header.status, proxy->header.method, proxy->header.uri, proxy->header.version);
	  loginfo(requestbuf);
	  notifyProxyFinished(proxy);
	  cleanDispatchResponse(dispatch);
	}
	else{
	  if(proxyclosed == 1){
	    notifyProxyFinished(proxy);
	    detachProxyDispatch(proxy, dispatch);
	  }
	  else{
	    proxy->con.events &= ~EPOLLOUT;
	    printf("send error and bytes %d.\n", hssum);
	  }
	}
      }
    }
  }
  if(proxy->status == PROXY_FREE){
    /*
      when the host close the connection and proxy send the response to host,
      the proxy could also close the connection to free the socket.

    */
    
    rmfromepoll(epfd, proxy->con.fd);
    freeProxyConnect(proxy);
    freeResource(&proxylist, &freeProxylist, (ListItem *)proxy);
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
  proxy = dispatch->proxy;
  if(dispatch->con.buf == NULL){
    dispatch->con.buf = initHTTPBuffer(4096*64);
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
    dispatch->con.events &= ~EPOLLIN;
    rrsum = recvfromhost(dispatch->con.fd, bufpos, &dispatchclosed);
    if(rrsum > 0){
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

      if(dispatch->con.buf->rdEOF != 1) notifyDispatchResponse(dispatch);
      rmfromepoll(epfd, dispatch->con.fd);
      freeDispatchConnect(dispatch);
    }
  }
  if(dispatch->status == PROXY_FREE){
    /*
      when the dispatch socket is closed and proxy socket has sended the response to host,
      then free the dispatch resource.

    */

    freeResource(&dispatchlist, &freeDispatchlist, (ListItem *)dispatch);
  }
  return 0;
}
