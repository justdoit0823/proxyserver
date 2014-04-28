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

#include "list.h"

enum{
  /*
    proxy connection status for the specified socket

  */

  PROXY_CLOSED = 2, /* the socket is closed, but proxy-dispatch is not finished. */
  PROXY_FREE = 3, /* the socket is closed and proxy-dispatch is finished. */
  PROXY_USED = 4, /* the socket is still active and do proxy-dispatch work. */
  PROXY_FINISHED = 5 /* proxy-dispatch is finished, but the socket is still active. */
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

struct ConnectManager{
  int count;
  void * conlist[MAXCONNECTIONS];
};

/*
  A connection manage table for fast lookup.

*/

static struct ConnectManager manager;

struct DispatchConnection * FindDispatchConnect(List * list, const char * host,  int status);

int initConnectManager();

int AddConnectToManager(int confd, void * con);

int RmConnectFromManager(int confd);

struct ConnectManager * GetManager();

int attachProxyDispatch(struct ProxyConnection * proxy, struct DispatchConnection * dispatch);

int detachProxyDispatch(struct ProxyConnection * proxy, struct DispatchConnection * dispatch);

struct ProxyConnection * newProxyConnect(int fd, int events, callbackfun callback);

List * getProxyResource(List ** queue, int fd, int events, callbackfun callback);

int freeProxyConnect(struct ProxyConnection * proxy);

int freeResource(List ** list, List ** freelist, ListItem * proxy);

struct DispatchConnection * newDispatchConnect(int fd, callbackfun callback);

List * getDispatchResource(List ** queue, int fd, callbackfun callback);

int freeDispatchConnect(struct DispatchConnection * dispatch);

int notifyDispatchResponse(struct DispatchConnection * dispatch);

int notifyProxyRequest(struct ProxyConnection * proxy);

int DispatchResulted(const struct DispatchConnection * dispatch);

int cleanDispatchResponse(struct DispatchConnection * dispatch);

int cleanProxyRequest(struct ProxyConnection * proxy);

#endif
