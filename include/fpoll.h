/*

  Author: Just do it.
  Email: justdoit920823@gmail.com
  Website: http://www.shareyou.net.cn/

*/

#ifndef FPOLL_H
#define FPOLL_H

#define EVENTMAX 128
#define DEFREQUESTSIZE (4096*8)
#define DEFBUFSIZE (4096*72)

#include "sys/epoll.h"
#include "http.h"

//define epoll events
#define ETIN (EPOLLIN|EPOLLET)
#define ETOUT (EPOLLOUT|EPOLLET)
#define ETINOUT (EPOLLIN|EPOLLOUT|EPOLLET)

#define MAXFD 1024

static int epfd;

static struct HTTPConnection * connections[MAXFD];

int initepoll(int size);

int addtoepoll(int efd, int fd, int events);

int editepoll(int efd, int fd, int events);

int rmfromepoll(int efd, int fd);

int setnonblock(int fd);

void runepoll(int epfd, int listenfd);

int handleaccept(int efd, int fd);

int handlehost(struct HTTPConnection * p);

int handleremote(struct HTTPConnection * p);

#endif
