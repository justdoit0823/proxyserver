/*

  Author: Just do it.
  Email: justdoit920823@gmail.com
  Website: http://www.shareyou.net.cn/

*/


#include "connection.h"
#include "stdlib.h"
#include "string.h"
#include "http.h"
#include "logging.h"
#include "errno.h"

int AddDispatchConnect(struct DispatchList ** list, struct DispatchList * dispatch)
{
  if(list == NULL) return -1;
  dispatch->next = *list;
  dispatch->prev = NULL;
  if(*list != NULL) (*list)->prev = dispatch;
  *list = dispatch;
  return 0;
}

struct DispatchConnection * FindDispatchConnect(struct DispatchList * list, const char * host, int status)
{
  /*
    if status is greater than 0, then status is the search condition.
  */

  if(host == NULL) return NULL;
  struct DispatchList * pcon;
  for(pcon=list; pcon != NULL; pcon=pcon->next){
    if(strcmp(host, pcon->dispatch->host) == 0){
      if(status == 0) return pcon->dispatch;
      else if(status > 0 && status == pcon->dispatch->status) return pcon->dispatch;
    }
  }
  return NULL;
}

int RemoveDispatchConnect(struct DispatchList ** list, struct DispatchList * dispatch)
{
  if(list == NULL) return -1;
  if(dispatch->prev == NULL){
    *list = dispatch->next;
    if(*list != NULL) (*list)->prev = NULL;
  }
  else{
    dispatch->prev->next = dispatch->next;
    if(dispatch->next != NULL) dispatch->next->prev = dispatch->prev;
  }
  return 0;
}


int initConnectManager()
{
  int i;
  manager.count = 0;
  for(i=0; i < MAXCONNECTIONS; i++) manager.conlist[i] = NULL;
  return 0;
}

int AddConnectToManager(int confd, void * con)
{
  if(confd <0 || confd >= MAXCONNECTIONS){
    printf("invild connect fd.\n");
    return -1;
  }
  else if(manager.conlist[confd] != NULL){
    printf("already connected.\n");
    return -1;
  }
  manager.conlist[confd] = con;
  manager.count += 1;
  return 0;
}

int RmConnectFromManager(int confd)
{
  if(confd <0 || confd >= MAXCONNECTIONS){
    printf("invild connect fd.\n");
    return -1;
  }
  manager.conlist[confd] = NULL;
  manager.count -= 1;
  return 0;
}

struct ProxyConnection * newProxyConnect(int fd, int events, callbackfun callback)
{
  struct ProxyConnection * proxy = NULL;
  proxy = malloc(sizeof(*proxy));
  if(proxy == NULL){
    logerr("malloc for new proxy connection error", errno);
  }
  memset(proxy, 0 ,sizeof(*proxy));
  proxy->status = PROXY_USED;
  proxy->con.fd = fd;
  proxy->con.events = events;
  proxy->con.fun = callback;
  return proxy;
}

struct DispatchConnection * newDispatchConnect(int fd, callbackfun callback, struct ProxyConnection * proxy)
{
  struct DispatchConnection * dispatch = NULL;
  dispatch = malloc(sizeof(*dispatch));
  if(dispatch == NULL){
    logerr("malloc for new dispatch connection error", errno);
  }
  memset(dispatch, 0 ,sizeof(*dispatch));
  dispatch->status = PROXY_USED;
  dispatch->con.fd = fd;
  dispatch->con.events = 0;
  dispatch->con.fun = callback;
  dispatch->con.buf = initHTTPBuffer(4096*32);
  dispatch->proxy = proxy;
  strcpy(dispatch->host, proxy->header.host);
  return dispatch;
}


struct ConnectManager * GetManager()
{
  return &manager;
}

int notifyDispatchResponse(struct DispatchConnection * dispatch)
{
  struct ProxyConnection * proxy;
  if(dispatch == NULL || (proxy = dispatch->proxy) == NULL) return -1;
  dispatch->con.buf->rdEOF = 1;
  proxy->con.buf->wrBuf = dispatch->con.buf->rdBuf;
  if(strcasecmp(dispatch->header.version, "HTTP/1.0") == 0)
    proxy->con.buf->outBytes = dispatch->con.buf->bufferedBytes;
  else proxy->con.buf->outBytes = dispatch->header.length;
  proxy->con.buf->wrReady = 1;
  return 0;
}
  
int notifyProxyRequest(struct ProxyConnection * proxy)
{
  struct DispatchConnection * dispatch;
  if(proxy == NULL || (dispatch = proxy->dispatch) == NULL) return -1;
  proxy->con.buf->rdEOF = 1;
  dispatch->con.buf->wrBuf = proxy->con.buf->rdBuf;
  dispatch->con.buf->outBytes = proxy->header.length;
  dispatch->con.buf->wrReady =1;
  return 0;
}

int DispatchResulted(const struct DispatchConnection * dispatch)
{
  /* check whether the dispatch response is over */

  if(dispatch == NULL) return -1;
  /*if(strcasecmp(dispatch->header.version, "HTTP/1.1") == 0){
    return (dispatch->con.buf->bufferedBytes >= dispatch->header.length);
    }*/
  return (dispatch->con.buf->bufferedBytes >= dispatch->header.length);
  return 0;
}

/*
int newProxyDispatch(struct ProxyConnection * proxy)
{
  struct DispatchConnection * dispatch;
  dispatch = proxy->dispatch;
  if(dispatch != NULL) return -1;
  if(proxy->header.host[0] != '\0'){
    if((dispatchfd = connecthost(proxy->header.host, proxy->header.port)) == -1) return -1;
    printf("connect success.\n");
    if(setnonblock(dispatchfd) == -1){
      logwarn("set fd nonblock error.\n");
      close(dispatchfd);
      return -1;
    }
    if(addtoepoll(epfd, dispatchfd, ETINOUT) !=0 ){
      logerr("add remote fd to epoll error", errno);
      return -1;
    }
    List * pl = newListItem();
    struct DispatchConnection * dispatchcon;
    dispatchcon = newDispatchConnect();
    dispatchcon->status = PROXY_USED;
    dispatchcon->con.fd = dispatchfd;
    dispatchcon->con.events = 0;
    dispatchcon->con.fun = handleremote;
    dispatchcon->proxy = proxy;
    proxy->dispatch = dispatchcon;
    strcpy(dispatchcon->host, proxy->header.host);
    AddConnectToManager(dispatchfd, dispatchcon);
    struct DispatchList * dl;
    dl = malloc(sizeof(*dl));
    dl->dispatch = dispatchcon;
    AddDispatchConnect(&dispatchlist, dl);
    pl->item = (struct HTTPConnection *)dispatchcon;
    addToList(&handlelist, pl);
    printf("add a list node %p,%p.\n", handlelist, pl->item);
  }
  return 0;
}
*/


int cleanDispatchResponse(struct DispatchConnection * dispatch)
{
  struct ProxyConnection * proxy;
  if(dispatch == NULL || ((proxy=dispatch->proxy) == NULL)) return -1;
  memset(dispatch->con.buf->rdBuf, 0, dispatch->con.buf->bufferedBytes);
  dispatch->con.buf->bufferedBytes = 0;
  dispatch->con.buf->rdEOF = 0;
  memset(dispatch->host, 0, sizeof(dispatch->host));
  memset(&(dispatch->header), 0, sizeof(dispatch->header));
  dispatch->proxy = NULL;
  dispatch->status = PROXY_FREE;
  memset(&(proxy->header), 0, sizeof(proxy->header));
  proxy->con.buf->wrReady = 0;
  proxy->con.buf->outBytes = 0;
  proxy->con.buf->wrBuf = NULL;
  proxy->dispatch = NULL;
  return 0;
}

int cleanProxyRequest(struct ProxyConnection * proxy)
{
  struct DispatchConnection * dispatch;
  if(proxy == NULL || (dispatch=proxy->dispatch) == NULL) return -1;
  memset(proxy->con.buf->rdBuf, 0, proxy->con.buf->bufferedBytes);
  proxy->con.buf->bufferedBytes = 0;
  proxy->con.buf->rdEOF = 0;
  dispatch->con.buf->outBytes = 0;
  dispatch->con.buf->wrReady = 0;
  dispatch->con.buf->wrBuf = NULL;
  return 0;
}
