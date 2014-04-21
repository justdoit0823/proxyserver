/*

  Author: Just do it.
  Email: justdoit920823@gmail.com
  Website: http://www.shareyou.net.cn/

*/



#ifndef LOGGING_H
#define LOGGING_H

#define TIMELENMAX 32
#define LOGLENMAX 1024


static int logfd;

struct Logger{
  int lfd;
  int loglevel;
  int status; /* only used in ERR log level */
  const char * logmsg;
};

enum{
  LOG_INFO = 1,
  LOG_WARN = 2,
  LOG_ERR = 3
} LOGLEVEL;

int getcurtime(char * strtime);

int initlog(const char * logpath);

int loginfo(const char * info);

int logwarn(const char * warn);

int logerr(const char * err, int status);

int logcore(const struct Logger * lg);

#endif
