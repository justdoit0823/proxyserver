/*

  Author: Just do it.
  Email: justdoit920823@gmail.com
  Website: http://www.shareyou.net.cn/

*/

#ifndef CONNECTION_H
#define CONNECTION_H

#define HOSTLEN 256
#define MAXCONNECTIONS 1024

#include "http.h"

enum{
  /*
    proxy connection status for the specified socket

  */

  PROXY_CLOSED = 1,
  PROXY_FREE = 2,
  PROXY_USED = 3
};

struct ProxyConnection{
  struct HTTPConnection con;
  int status;
  struct DispatchConnection * dispatch;
  struct requestHeader header;
};


struct DispatchConnection{
  struct HTTPConnection con;
  int status;
  struct ProxyConnection * proxy;
  char host[HOSTLEN];
  struct responseHeader header;
};

struct DispatchList{
  struct DispatchConnection * dispatch;
  struct DispatchList * prev;
  struct DispatchList * next;
};

struct ProxyList{
  struct ProxyConnection * dispatch;
  struct ProxyList * prev;
  struct ProxyList * next;
};


struct ConnectManager{
  int count;
  void * conlist[MAXCONNECTIONS];
};

static struct ConnectManager manager;

int AddDispatchConnect(struct DispatchList ** list, struct DispatchList * dispatch);

struct DispatchConnection * FindDispatchConnect(struct DispatchList * list, const char * host,  int status);

int RemoveDispatchConnect(struct DispatchList ** list, struct DispatchList * dispatch);

int initConnectManager();

int AddConnectToManager(int confd, void * con);

int RmConnectFromManager(int confd);

struct ConnectManager * GetManager();

struct ProxyConnection * newProxyConnect(int fd, int events, callbackfun callback);

int freeProxyConnect(struct ProxyList ** proxylist, struct ProxyList * proxy);

struct DispatchConnection * newDispatchConnect(int fd, callbackfun callback, struct ProxyConnection * proxy);

int freeDispatchConnect(struct DispatchList ** dispatchlist, struct DispatchList * dispatch);

int notifyDispatchResponse(struct DispatchConnection * dispatch);

int notifyProxyRequest(struct ProxyConnection * proxy);

int DispatchResulted(const struct DispatchConnection * dispatch);

int newProxyDispatch(struct ProxyConnection * proxy);

int cleanDispatchResponse(struct DispatchConnection * dispatch);

int cleanProxyRequest(struct ProxyConnection * proxy);

struct DispatchList * findDispatchNode(struct DispatchList * search, struct DispatchConnection * item);

int rmFromDispatchlist(struct DispatchList ** head, struct DispatchList * node);

#endif
