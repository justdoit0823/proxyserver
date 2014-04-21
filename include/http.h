/*

  Author: Just do it.
  Email: justdoit920823@gmail.com
  Website: http://www.shareyou.net.cn/

*/

#ifndef HTTP_H
#define HTTP_H

#define HTTPHEADMAX 512
#define HOSTLENMAX 512
#define HOSTNAMELEN 256
#define SHORTOPLEN 16
#define DEFHTTPPORT 80
#define BUFLEN 4096
#define METHODLEN 8
#define URILEN 768
#define SHORTSTRLEN 16
#define NORSTRLEN 128
#define MEDSTRLEN 256
#define LONGSTRLEN 512
#define MAXHTTPBUFFER (4096*32)

#define NULLFD ((int)(-1))

#include "netinet/in.h"
#include "stdio.h"


//define supported http method
static char * methods[] = {"GET", "POST", "HEAD", "PUT", NULL};

//define HTTP_STATUS_CODE
enum HTTP_STATUS{
  HTTP_OK = 200,
  HTTP_NOT_MODIFIED = 304,
  HTTP_BAD_REQUEST = 400,
  HTTP_FORBIDDEN = 403,
  HTTP_NOT_FOUND = 404,
  HTTP_BAD_GETWAY = 502
};

//define http header error code
enum HTTP_HEADER_ERROR{
  HTTP_INVALID_HEADER = 1,
  HTTP_UNSUPPORT_METHOD = 2,
  HTTP_TOOLONG_HEADER = 3,
  HTTP_UNSUPPORT_VERSION = 4
};

struct namenode{
  char name[HOSTNAMELEN];
  struct in_addr addr;
  struct namenode * next;
};

//http request header field
struct requestHeader{
  char method[SHORTSTRLEN];
  char uri[URILEN];
  char version[SHORTSTRLEN];
  char host[MEDSTRLEN];
  char userAgent [NORSTRLEN];
  char cookie [LONGSTRLEN];
  char connection [SHORTSTRLEN];
  int port;
  int length;
  int parsePass; /* set this flag to indicate parsing done. */
};

//http response header field
struct responseHeader{
  char version[SHORTSTRLEN];
  char text[2*SHORTSTRLEN];
  char date[2*SHORTSTRLEN];
  char server[2*SHORTSTRLEN];
  char contentType[SHORTSTRLEN];
  char connection [SHORTSTRLEN];
  int status;
  int length;
  int parsePass; /* set this flag to indicate parsing done. */
};

//buffer segment
struct BufferSegment{
  int start;
  int length;
  int finished;
  union{
    struct requestHeader req;
    struct responseHeader res;
  } header;
  struct HTTPConnection * hc;
  struct BufferSegment * next;
};


//buffer segment entry
struct SegmentEntery{
  struct BufferSegment * head;
  struct BufferSegment * current;
  struct BufferSegment * tail;
};


//http buffer
struct HTTPBuffer{
  char * rdBuf;
  char * wrBuf;
  int maxBytes;
  int bufferedBytes;
  int outBytes;
  int rdEOF;
  int wrReady;
  struct SegmentEntery * entry;
};

struct HTTPConnection;

//http request callback
typedef int (*callbackfun)(struct HTTPConnection *);


//http connection
struct HTTPConnection{
  int fd;
  int events;
  int count;
  callbackfun fun;
  struct HTTPBuffer * buf;
  struct HTTPConnection * pair;
};

static char * whitelist[]={"www.google.com","www.twitter.com","www.facebook.com"};

static struct namenode * namelist;

char * read_untill(int fd,const char * split);

int parseRequest(const char * requeststr,struct requestHeader * header);

int parseResponse(const char * responsestr, int bytes, struct responseHeader * header);

int generateHeader(struct responseHeader * header);

int addNameList(const char * name,const struct in_addr * addr);

struct in_addr * findNameList(const char * name);

int hnametoaddr(const char * name,struct in_addr * addr);

int checkmethod(const char * method);

int connecthost(const char * host, short port, int * fd);

int sendtohost(int fd,const char * strbuf,int size, int * closed);

int recvfromhost(int fd,char * strbuf, int * closed);

int initnamelist();

struct HTTPBuffer * initHTTPBuffer(int size);

struct SegmentEntery * initBufferEntry(struct HTTPBuffer * hb);

void freeHTTPBuffer(struct HTTPBuffer * hb);

struct BufferSegment * createBufSegment(int start,int length);

void freeBufSegment(struct BufferSegment * bs);

struct HTTPConnection * new_connect(int fd);

void free_connect(struct HTTPConnection * hc);

struct SegmentEntery * new_SegmentEntry(void);

void free_SegmentEntry(struct SegmentEntery * entry);

int strstrip(char * str);

#endif
