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


struct DispatchConnection * FindDispatchConnect(List * list, const char * host, int status)
{
  /*
    if status is greater than 0, then status is the search condition.
  */

  if(host == NULL) return NULL;
  List * pcon;
  struct DispatchConnection * dispatch;
  for(pcon=list; pcon != NULL; pcon=pcon->next){
    dispatch = (struct DispatchConnection *)(pcon->item);
    if(strcmp(host, dispatch->host) == 0){
      if(status == 0) return dispatch;
      else if(status > 0 && (status == dispatch->status)) return dispatch;
    }
  }
  return NULL;
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

List * getProxyResource(List ** queue, int fd, int events, callbackfun callback)
{
  List * nl;
  struct ProxyConnection * proxycon;
  if(*queue == NULL){
    proxycon = newProxyConnect(fd, events, callback);
    nl = newListNode();
    nl->item = (ListItem *)proxycon;
  }
  else{
    nl = *queue;
    rmFromList(queue, nl);
    proxycon = (struct ProxyConnection *)nl->item;
    proxycon->status = PROXY_USED;
    proxycon->con.fd = fd;
    proxycon->con.events = events;
    proxycon->con.fun = callback;
  }
  return nl;
}

int freeProxyConnect(struct ProxyConnection * proxy)
{
  if(proxy == NULL) return -1;
  close(proxy->con.fd);
  RmConnectFromManager(proxy->con.fd);
  proxy->con.events = 0;
  proxy->con.fd = NULLFD;
  return 0;
}

int freeResource(List ** list, List ** freelist, ListItem * proxy)
{
  if(proxy == NULL) return -1;
  List * node = findListNode(*list, proxy);
  rmFromList(list, node);
  addToList(freelist, node);
  return 0;
}

struct DispatchConnection * newDispatchConnect(int fd, callbackfun callback)
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
  dispatch->con.buf = initHTTPBuffer(4096*64);
  return dispatch;
}

List * getDispatchResource(List ** queue, int fd, callbackfun callback)
{
  List * pl;
  struct DispatchConnection * dispatchcon;
  if(*queue == NULL){
    pl = newListNode();
    dispatchcon = newDispatchConnect(fd, callback);
    pl->item = (ListItem *)dispatchcon;
  }
  else{
    pl = *queue;
    rmFromList(queue, pl);
    dispatchcon = (struct DispatchConnection *)(pl->item);
    dispatchcon->con.fd = fd;
    dispatchcon->con.fun = callback;
    dispatchcon->status = PROXY_USED;
  }
  return pl;
}

int freeDispatchConnect(struct DispatchConnection * dispatch)
{
  if(dispatch == NULL) return -1;
  close(dispatch->con.fd);
  RmConnectFromManager(dispatch->con.fd);
  dispatch->con.events = 0;
  dispatch->con.fd = NULLFD;
  if(dispatch->status == PROXY_USED) dispatch->status = PROXY_CLOSED;
  else if(dispatch->status == PROXY_FINISHED) dispatch->status = PROXY_FREE;
  return 0;
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

int notifyProxyFinished(struct ProxyConnection * proxy)
{
  /*
    when the proxy server sends request and recives response,
    then notify the proxy and dispatch status.

  */

  struct DispatchConnection * dispatch;
  if(proxy == NULL || (dispatch = proxy->dispatch) == NULL) return -1;
  if(proxy->status == PROXY_CLOSED) proxy->status = PROXY_FREE;
  else if(proxy->status == PROXY_USED) proxy->status = PROXY_FINISHED;
  if(dispatch->status == PROXY_CLOSED) dispatch->status = PROXY_FREE;
  else if(dispatch->status == PROXY_USED) dispatch->status = PROXY_FINISHED;
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
}

int cleanDispatchResponse(struct DispatchConnection * dispatch)
{
  struct ProxyConnection * proxy;
  if(dispatch == NULL || ((proxy=dispatch->proxy) == NULL)) return -1;
  memset(dispatch->con.buf->rdBuf, 0, dispatch->con.buf->bufferedBytes);
  dispatch->con.buf->bufferedBytes = 0;
  dispatch->con.buf->rdEOF = 0;
  memset(&(dispatch->header), 0, sizeof(dispatch->header));
  memset(&(proxy->header), 0, sizeof(proxy->header));
  proxy->con.buf->wrReady = 0;
  proxy->con.buf->outBytes = 0;
  proxy->con.buf->wrBuf = NULL;
  detachProxyDispatch(proxy, dispatch);
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

int attachProxyDispatch(struct ProxyConnection * proxy, struct DispatchConnection * dispatch)
{
  if(proxy == NULL || dispatch == NULL) return -1;
  proxy->dispatch = dispatch;
  dispatch->proxy = proxy;
  return 0;
}

int detachProxyDispatch(struct ProxyConnection * proxy, struct DispatchConnection * dispatch)
{
  if(proxy == NULL || dispatch == NULL) return -1;
  proxy->dispatch = NULL;
  dispatch->proxy = NULL;
  return 0;
}
