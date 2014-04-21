/*

  Author: Just do it.
  Email: justdoit920823@gmail.com
  Website: http://www.shareyou.net.cn/

*/

#include "jobqueue.h"
#include "stdio.h"
#include "stdlib.h"
#include "errno.h"
#include "logging.h"


int initqueue()
{
  qhead=qtail=NULL;
  return (pthread_mutex_init(&qmutex,NULL) == 0 && pthread_cond_init(&qcond,NULL) == 0);
}

int queuein(int fd)
{
  struct jobitem * newjob;
  if((newjob=(struct jobitem *)malloc(sizeof(struct jobitem))) == NULL){
    logerr("memory alloc error", errno);
    return -1;
  }
  printf("queue in %d.\n",fd);
  newjob->fd=fd;
  newjob->next=NULL;
  pthread_mutex_lock(&qmutex);
  if(qtail == NULL){
    qhead=qtail=newjob;
  }
  else{
    qtail->next=newjob;
    qtail=newjob;
  }
  pthread_cond_broadcast(&qcond);
  pthread_mutex_unlock(&qmutex);
  return 0;
}

int queueout()
{
  int outfd;
  struct jobitem * outjob;
  pthread_mutex_lock(&qmutex);
  while(qhead == NULL){
    if(pthread_cond_wait(&qcond,&qmutex) != 0){
      pthread_mutex_unlock(&qmutex);
      return -1;
    }
  }
  outjob=qhead;
  outfd=outjob->fd;
  printf("queue out %d at thread %x.\n",outfd,(unsigned int)pthread_self());
  qhead=qhead->next;
  if(outjob == qtail) qtail=NULL;
  pthread_mutex_unlock(&qmutex);
  free(outjob);
  return outfd;
}
