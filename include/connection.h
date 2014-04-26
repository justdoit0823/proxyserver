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

struct ConnectManager{
  int count;
  void * conlist[MAXCONNECTIONS];
};

static struct ConnectManager manager;

struct DispatchConnection * FindDispatchConnect(List * list, const char * host,  int status);

int initConnectManager();

int AddConnectToManager(int confd, void * con);

int RmConnectFromManager(int confd);

struct ConnectManager * GetManager();

struct ProxyConnection * newProxyConnect(int fd, int events, callbackfun callback);

List * getProxyNode(List ** queue, int fd, int events, callbackfun callback);

int freeProxyConnect(struct ProxyConnection * proxy);

struct DispatchConnection * newDispatchConnect(int fd, callbackfun callback, struct ProxyConnection * proxy);

List * getDispatchNode(List ** queue, int fd, callbackfun callback, struct ProxyConnection * proxy);

int freeDispatchConnect(struct DispatchConnection * dispatch);

int notifyDispatchResponse(struct DispatchConnection * dispatch);

int notifyProxyRequest(struct ProxyConnection * proxy);

int DispatchResulted(const struct DispatchConnection * dispatch);

int newProxyDispatch(struct ProxyConnection * proxy);

int cleanDispatchResponse(struct DispatchConnection * dispatch);

int cleanProxyRequest(struct ProxyConnection * proxy);

#endif
