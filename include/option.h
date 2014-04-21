/*

  Author: Just do it.
  Email: justdoit920823@gmail.com
  Website: http://www.shareyou.net.cn/

*/

#ifndef OPTION_H

#include "stdio.h"
#include "stdlib.h"
#include "getopt.h"

#define MAXOPTIONS 128
#define MAXOPTSTRLEN 256

struct Arguments{
  short port;
  int showhelp;
  char addr[16];
  char logpath[512];
};

struct OptionsList{
  int usedCount;
  int maxCount;
  struct option options[MAXOPTIONS];
};

struct OptionString{
  int tail;
  int total;
  char optstring[MAXOPTSTRLEN];
};

/*
static struct option options[] = {
  {"host", required_argument, 0 , 'i'},
  {"help", no_argument,         0 , 1},
  {"port", required_argument, 0 , 'p'},
  {"path", required_argument, 0 , 'f'},
  {0, 0, 0 , 0}
};

static const char optstring = "i:p:f:" ;
*/

static struct OptionsList opts = {0, MAXOPTIONS, {0, 0, 0, 0}};

static struct OptionString optstr = {0, MAXOPTSTRLEN, 0};

void usage(void);

int parseArguments(int argc, char * argv[], struct Arguments * args);

int addLongOption(const char * name, int has_arg, int * flag, int val);

int addOption(const char * opt);

#endif
