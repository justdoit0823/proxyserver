/*

  Author: Just do it.
  Email: justdoit920823@gmail.com
  Website: http://www.shareyou.net.cn/

*/

#ifndef JOBQUEUE_H
#define JOBQUEUE_H

#define DEFQUEUELEN 16

#define QITEMNULL 0

#include "pthread.h"


struct jobitem{
  int fd;
  struct jobitem * next;
} ;

static pthread_mutex_t qmutex=PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t qcond=PTHREAD_COND_INITIALIZER;
static struct jobitem * qhead, * qtail;

int initqueue();

int queuein(int fd);

int queueout();

#endif
