/*

  Author: Just do it.
  Email: justdoit920823@gmail.com
  Website: http://www.shareyou.net.cn/

*/

#include "option.h"
#include "string.h"

void usage(void)
{
  char usagemsg[] = "Usage: proxyserver [option]\n"
                    "\t-i|--addr=local ipv4 address\n"
                    "\t-p|--port=listen port\n"
                    "\t-f|--path=log file path\n" ;
  printf("%s\n", usagemsg);
  return;
}

int parseArguments(int argc, char * argv[], struct Arguments * args)
{
  int result;
  while(1){
    result = getopt_long(argc, argv, optstr.optstring, opts.options, NULL);
    if(result == -1) break;
    else if(result == 0) continue;
    switch(result){
      case 'h' : args->showhelp = 1; break;
      case 'i' : strcpy(args->addr, optarg); break;
      case 'p' : args->port = atoi(optarg); break;
      case 'f' : strcpy(args->logpath, optarg); break;
      case '?' : return -1;
      default : printf("unknow option!\n"); break;
    }
  }
  return 0;
}

int addLongOption(const char * name, int has_arg, int * flag, int val)
{
  if(opts.usedCount == opts.maxCount) return -1;
  struct option opt = {name, has_arg, flag, val};
  opts.options[opts.usedCount] = opt;
  opts.usedCount += 1;
  return 0;
}

int addOption(const char * opt)
{
  int len = strlen(opt);
  if(len > (optstr.total - optstr.tail)) return -1;
  strncpy(optstr.optstring + optstr.tail, opt, len);
  optstr.tail += len;
  return 0;
}

