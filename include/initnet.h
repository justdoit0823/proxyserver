/*

  Author: Just do it.
  Email: justdoit920823@gmail.com
  Website: http://www.shareyou.net.cn/

*/

#ifndef INITNET_H
#define INITNET_H

#define SOCKADDRLEN 256
#define DEFQUELEN 128

#include "sys/socket.h"
#include "netinet/in.h"

static serverrunning=0;

int initsocket(int socktype,int protocol);

int initserver(int fd,const struct sockaddr * addr,int socklen);

void runserver(int fd);


#endif
