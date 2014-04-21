/*

  Author: Just do it.
  Email: justdoit920823@gmail.com
  Website: http://www.shareyou.net.cn/

*/

#include "logging.h"
#include "stdio.h"
#include "unistd.h"
#include "sys/fcntl.h"
#include "time.h"
#include "errno.h"
#include "string.h"


/*
  open a file for logging.

*/

int initlog(const char * path)
{
  if((logfd = open(path, O_CREAT|O_APPEND|O_WRONLY, 0662)) == -1){
      printf("initlog error:%s.\n",strerror(errno));
      return -1;
  }
  return 0;
}



/*
  get current locale time and formate as a string buffer.

*/

int getcurtime(char * strtime)
{
  time_t ct;
  struct tm tm;
  if(time(&ct) == -1) return -1;
  if(localtime_r(&ct,&tm) == NULL) return -1;
  if(strftime(strtime,TIMELENMAX,"%Y-%m-%d %X",&tm) <= 0) return -1;
  return 0;
}
  

/*
  log normal message into file.

*/

int loginfo(const char * info)
{
  struct Logger log;
  log.lfd = logfd;
  log.loglevel = LOG_INFO;
  log.logmsg = info;
  return logcore(&log);
}
  

/*
  log warning message into file.

*/

int logwarn(const char * warn)
{
  struct Logger log;
  log.lfd = logfd;
  log.loglevel = LOG_WARN;
  log.logmsg = warn;
  return logcore(&log);
}

/*
  log error message into file.

*/

int logerr(const char * err, int status)
{
  struct Logger log;
  log.lfd = logfd;
  log.loglevel = LOG_ERR;
  log.logmsg = err;
  log.status = status;
  return logcore(&log);
}

int logcore(const struct Logger * lg)
{
  char timebuf[TIMELENMAX];
  char logbuf[LOGLENMAX];
  int loglen;
  if(getcurtime(timebuf) == -1){
    memset(timebuf,0,TIMELENMAX);
    memcpy(timebuf,"unknow time",sizeof("unknow time")-1);
  }
  switch(lg->loglevel){
    case LOG_INFO :
      sprintf(logbuf,"[I %s server] %s",timebuf,lg->logmsg);
      break;
    case LOG_WARN :
      sprintf(logbuf,"[W %s server] %s",timebuf,lg->logmsg);
      break;
    case LOG_ERR :
      if(lg->status > 0) sprintf(logbuf,"[E %s server]%s:%s.\n",timebuf,lg->logmsg,strerror(lg->status));
      else sprintf(logbuf,"[E %s server] %s.\n",timebuf,lg->logmsg);
      break;
    default :
      sprintf(logbuf, "unknow log level message.\n");
      break;
  }
  loglen = strlen(logbuf);
  if(write(lg->lfd, logbuf, loglen) != loglen){
    printf("logerr to %d fail:%s.\n",lg->lfd,strerror(errno));
    return -1;
  }
  return 0;
}
