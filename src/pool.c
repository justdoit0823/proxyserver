/*

  Author: Just do it.
  Email: justdoit920823@gmail.com
  Website: http://www.shareyou.net.cn/

*/

#include "pool.h"
#include "stdio.h"
#include "unistd.h"
#include "string.h"
#include "initnet.h"
#include "jobqueue.h"
#include "http.h"

int initthreadpool()
{
  int i;
  pthread_t tid;
  for(i=0;i < DEFTHREADS;i++){
    tid=pthread_create(&tid,NULL,handlerequest,NULL);
    if(tid != 0) return -1;
    pt[i]=tid;
  }
  return 0;
}

void * handlerequest(void * arg)
{
  int reqfd;
  while(1){
    reqfd=queueout();
    if(reqfd <= 0){
      continue;
    }
    printf("request fd is %d.\n",reqfd);
    dojob(reqfd);
  }
}

int dojob(int fd)
{
  /*
  char buf[DEFBUFSIZE];
  int rtfd,qrsum,rrsum;
  if((qrsum=recvfromhost(fd,buf)) <= 0){
    logwarn("no data is recvied from host.\n");
    close(fd);
    return -1;
  }
  loginfo("get rquest data.\n");
  if((rtfd=connecthost(buf, 80)) == -1){
    close(fd);
    logerr("connect to remote host error");
    return -1;
  }
  if(sendtohost(rtfd,buf,qrsum) != 0) logwarn("not all data are sended.\n");
  memset(buf,0,qrsum);
  loginfo("send remote data.\n");
  if((rrsum=recvfromhost(rtfd,buf)) <= 0){
    logwarn("no data is recvied from host.\n");
    close(rtfd);
    close(fd);
    return -1;
  }
  loginfo("get remote data.\n");
  close(rtfd);
  if(sendtohost(fd,buf,rrsum) != 0) logwarn("not all data are sended.\n");
  close(fd);
  loginfo("send request data.\n");
  return 0;
  */
}
