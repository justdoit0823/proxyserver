/*

  Author: Just do it.
  Email: justdoit920823@gmail.com
  Website: http://www.shareyou.net.cn/

*/

#ifndef POOL_H
#define POOL_H

#define DEFTHREADS 4
#define DEFBUFSIZE (4096*32)
#define HEADMAX 256

#include "pthread.h"

static pthread_t pt[DEFTHREADS];

int initthreadpool();

void * handlerequest(void * arg);

int dojob(int fd);


#endif 
