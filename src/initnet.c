/*

  Author: Just do it.
  Email: justdoit920823@gmail.com
  Website: http://www.shareyou.net.cn/

*/

#include "initnet.h"
#include "stdio.h"
#include "stdlib.h"
#include "pool.h"
#include "jobqueue.h"
#include "errno.h"
#include "logging.h"

int initsocket(int socktype,int protocol)
{
  return socket(AF_INET,socktype,protocol);
}

int initserver(int fd,const struct sockaddr * addr,int socklen)
{
  int optval=1;
  setsockopt(fd,SOL_SOCKET,SO_REUSEADDR,&optval,sizeof(optval));
  if(bind(fd,addr,socklen) == -1) return -1;
  if(listen(fd,DEFQUELEN) == -1) return -1;
  return 0;
}

void runserver(int fd)
{
  int requestfd;
  char logbuf[256];
  char addrbuf[32];
  struct sockaddr_in sain;
  socklen_t sl=SOCKADDRLEN;
  while(1){
    if((requestfd=accept(fd,(struct sockaddr *)&sain,&sl)) == -1) logerr("accept error", errno);
    else{
      queuein(requestfd);
      inet_ntop(AF_INET,&sain.sin_addr,addrbuf,32);
      sprintf(logbuf,"recieve a request from %s.\n",addrbuf);
      loginfo(logbuf);
      if(serverrunning == 0){
	if(initthreadpool() == -1){
	  logerr("init a thread pool error", errno);
	  exit(0);
	}
	serverrunning=1;
      }
    }
  }
}
  
