/*

  Author: Just do it.
  Email: justdoit920823@gmail.com
  Website: http://www.shareyou.net.cn/

*/


#include "stdio.h"
#include "stdlib.h"
#include "netinet/in.h"
#include "sys/resource.h"
#include "signal.h"
#include "http.h"
#include "initnet.h"
#include "jobqueue.h"
#include "logging.h"
#include "pool.h"
#include "option.h"
#include "errno.h"

void serverstop(int signum);

int main(int argc,char * argv[])
{
  /*if(argc < 4){
    printf("usage:a.out <host> <port> <logfile>.\n");
    exit(-1);
    }*/
  struct Arguments arg;
  int r;
  addLongOption("host", required_argument, 0, 'i');
  addOption("i:");
  addLongOption("help", no_argument, 0, 'h');
  addOption("h");
  addLongOption("port", required_argument, 0 , 'p');
  addOption("p:");
  addLongOption("path", required_argument, 0 , 'f');
  addOption("f:");
  addLongOption(0, 0, 0 , 0);
  if(parseArguments(argc, argv, &arg) == -1 || arg.showhelp == 1){
    usage();
    exit(0);
  }
  if(initlog(arg.logpath) == -1) exit(-1);
  int serfd, epfd;
  struct sockaddr_in sain;
  if(signal(SIGQUIT,serverstop) == SIG_ERR){
    logerr("bind signal handler error", errno);
    exit(-1);
  }
  if(signal(SIGPIPE,SIG_IGN) == SIG_ERR){
    logerr("ignore signal pipe error", errno);
    exit(-1);
  }
  if((serfd=initsocket(SOCK_STREAM|SOCK_NONBLOCK,0)) == -1){
    logerr("init a server socket error", errno);
    exit(-1);
  }
  sain.sin_family=AF_INET;
  inet_aton(arg.addr, &sain.sin_addr);
  sain.sin_port=htons(arg.port);
  if(initserver(serfd,(struct sockaddr *)&sain,sizeof(sain)) == -1){
    logerr("start a server error", errno);
    exit(-1);
  }
  if(initqueue() == 0){
    logerr("init job queue error", errno);
    exit(-1);
  }
  initnamelist();
  if((epfd = initepoll(1)) == -1){
    logerr("init epoll error", errno);
    exit(-1);
  }
  loginfo("server start successfully.\n");
  //runserver(serfd);
  runepoll(epfd, serfd);
  exit(0);
}
  
void serverstop(int signum)
{
  loginfo("server stop.\n");
  exit(0);
}
