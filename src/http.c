/*

  Author: Just do it.
  Email: justdoit920823@gmail.com
  Website: http://www.shareyou.net.cn/

*/


#include "http.h"
#include "stdio.h"
#include "signal.h"
#include "unistd.h"
#include "stdlib.h"
#include "initnet.h"
#include "netdb.h"
#include "arpa/inet.h"
#include "errno.h"
#include "string.h"
#include "logging.h"

int initnamelist()
{
  namelist = NULL;
  return 0;
}

int addNameList(const char * name,const struct in_addr * addr)
{
  struct namenode * p;
  p = (struct namenode *)malloc(sizeof(struct namenode));
  if(p == NULL) logwarn("can not alloc memory for namelist.\n");
  else{
    strcpy(p->name,name);
    memcpy(&p->addr, addr, sizeof(*addr));
    p->next = namelist;
    namelist = p;
  }
}

struct in_addr * findNameList(const char * name)
{
  struct namenode * p;
  for(p=namelist; p != NULL; p=p->next){
    if(strcmp(name,p->name) == 0){
      return &p->addr;
    }
  }
  return NULL;
}

int hnametoaddr(const char * name,struct in_addr * addr)
{
  struct hostent * he;
  struct in_addr * p;
  if(inet_aton(name, addr) == 1) return 0;
  p = findNameList(name);
  if(p != NULL){
    memcpy(addr, p, sizeof(*p));
    return 0;
  }
  unsigned char * ar = (unsigned char *)&(addr->s_addr);
  if((he = gethostbyname(name)) == NULL){
    logerr(hstrerror(h_errno), 0);
    return -1;
  }
  memcpy(&(addr->s_addr), he->h_addr_list[0], he->h_length);
  printf("host %s's address is %u.%u.%u.%u\n", name, ar[0],ar[1],ar[2],ar[3]);
  addNameList(name, addr);
  return 0;
}

int checkmethod(const char * method)
{
  int i;
  if(strlen(method) > METHODLEN) return HTTP_UNSUPPORT_METHOD; 
  for(i=0; methods[i] != NULL; i++){
    if(strcasecmp(methods[i], method) == 0) return 0;
  }
  return HTTP_UNSUPPORT_METHOD;
}

int strstrip(char * str)
{
  int i,n;
  if(str == NULL) return -1;
  n = 0;
  while(*str == ' ') n++;
  for(i=n; str[i] != '\0' || str[i] != ' '; i++){
    if(n > 0){
      str[i-n] = str[i];
    }
  }
  str[i-n] = '\0';
  return 0;
}

int parseRequest(const char * requeststr,struct requestHeader * header)
{
  char * line, * split, * headend;
  char optionkey[SHORTSTRLEN];
  char optionval[SHORTSTRLEN];
  int portlen;
  if(requeststr == NULL || header == NULL) return -1;
  headend = strstr(requeststr, "\r\n\r\n");
  if(headend == NULL) return -1;
  if(sscanf(requeststr,"%s%s%s",header->method,header->uri,header->version) != 3){
    printf("invalid http header!\n");
    return HTTP_INVALID_HEADER;
  }
  if(checkmethod(header->method) != 0){
    printf("unsupport http method.\n");
    return HTTP_UNSUPPORT_METHOD;
  }
  if((line = strstr(requeststr,"\r\n")) == NULL) return -1;
  else line += 2;
  if(sscanf(line,"%s%s",optionkey,header->host) != 2){
    printf("invalid http header!\n");
    return HTTP_INVALID_HEADER;
  }
  if((split = strchr(header->host, ':')) != NULL){
    header->port = atoi(split + 1);
    *split = '\0';
  }
  else header->port = DEFHTTPPORT;
  header->length = headend - requeststr + 4;
  if(strcasecmp(header->method, "POST") == 0){
    if((line = (char *)strcasestr(requeststr,"Content-Length")) != NULL){
      if(sscanf(line,"%s%s",optionkey, optionval) != 2){
	printf("invalid http header!\n");
	return HTTP_INVALID_HEADER;
      }
      header->length += atoi(optionval);
    }
  }
  header->parsePass = 1;
  return 0;
}

int parseResponse(const char * responsestr,int bytes, struct responseHeader * header)
{
  const char * line, * split, * headend, * body;
  char optionkey[32];
  char optionval[32];
  int chunked;
  if(responsestr == NULL || header == NULL) return -1;
  headend = strstr(responsestr, "\r\n\r\n");
  if(headend == NULL) return -1;
  if(sscanf(responsestr,"%s%s%s",header->version, optionval, header->text) != 3){
    printf("invalid http header!\n");
    return HTTP_INVALID_HEADER;
  }
  header->status = atoi(optionval);
  memset(optionval, 0 , sizeof(optionval));
  header->length = headend - responsestr + 4;
  if((line = (char *)strcasestr(responsestr,"Content-Length")) != NULL){
    if(sscanf(line,"%s%s",optionkey, optionval) != 2){
      printf("invalid http header!\n");
      return HTTP_INVALID_HEADER;
    }
    header->length += atoi(optionval);
    header->parsePass = 1;
    printf("the content length is %d.\n", header->length);
    return 0;
  }
  else if((line = (char *)strcasestr(responsestr,"Transfer-Encoding")) != NULL){
    if(sscanf(line,"%s%s",optionkey, optionval) != 2){
      printf("invalid http header!\n");
      return HTTP_INVALID_HEADER;
    }
    if(strcasecmp(optionval, "chunked") == 0){
      body = headend + 4;
      line = responsestr + bytes;
      chunked = 0;
      printf("headend at %p, body begin at %p, line at %p.\n", headend, body, line);
      if(line >= body) chunked = (*((int *)(line - 4)) == 0x0a0d0a0d);
      if(chunked){
	header->length += line - body;
	printf("the chunked length is %d.\n", header->length);
	header->parsePass = 1;
	return 0;
      }
    }
  }
  return HTTP_INVALID_HEADER;
}

void timeout(int signum)
{
  int err = errno;
  printf("conenct time out.\n"); /* it's not safe. */
  errno = err;
  return;
}

int connecthost(const char * host, short port, int * fd)
{
  struct sockaddr_in sain;
  char logbuf[64];
  int fetchfd;
  sain.sin_family = AF_INET;
  sain.sin_port = htons(port);
  if(host == NULL && host[0] == '\0') return -1;
  if(hnametoaddr(host, &sain.sin_addr) == -1) return -1;
  if((fetchfd = initsocket(SOCK_STREAM,0)) == -1){
    logerr("can not init a socket", errno);
    return -1;
  }
  *fd = fetchfd;
  __sighandler_t presighandler = signal(SIGALRM, timeout);
  alarm(CONTIMEOUT);/* start timer */
  if(connect(fetchfd, (struct sockaddr *)&sain, sizeof(sain)) == -1){
    if(errno != EINTR){
      sprintf(logbuf, "connect to remote host %s error", host);
      logerr(logbuf, errno);
    }
    return -1;
  }
  alarm(0);/* conenct success and cancel timer. */
  signal(SIGALRM, presighandler);
  return 0;
}

char * read_untill(int fd,const char * split)
{
  /*int rdbys,sum;
  sum = 0;
  struct HTTPBuffer * buf = initHTTPBuffer(4096*4096);
  while((rdbys = read(fd,buf->buf,4096)) > 0){
    sum += rdbys;
  }
  return buf->buf;*/
}

int sendtohost(int fd, const char * strbuf, int size, int * closed)
{
  int wsize,wsum;
  wsum=0;
  printf("write:");
  while(size > 0){
    wsize = write(fd,strbuf,size);
    if(wsize == -1){
      if(errno == EINTR) continue;
      else{
	if(errno != EAGAIN){
	  logerr("write error", errno);
	  *closed = 1;
	}
	break;
      }
    }
    strbuf += wsize;
    wsum += wsize;
    size -= wsize;
    printf("%d,", wsize);
  }
  printf("\n");
  return wsum;
}

int recvfromhost(int fd, char * strbuf, int * closed)
{
  int rsize,rsum;
  rsum=0;
  printf("read:");
  while(1){
    rsize = read(fd, strbuf, BUFLEN);
    if(rsize == -1){
      if(errno == EINTR) continue;
      else{
	if(errno != EAGAIN){
	  logerr("read error", errno);
	  *closed = 1;
	}
	break;
      }
    }
    else if(rsize == 0){
      *closed = 1;
      printf("%d,", rsize);
      break;
    }
    strbuf += rsize;
    rsum += rsize;
    printf("%d,", rsize);
    //if(rsize < BUFLEN) break;
  }
  printf("\n");
  return rsum;
}

struct SegmentEntery * new_SegmentEntry(void)
{
  struct SegmentEntery * se;
  if((se = malloc(sizeof(*se))) == NULL) return NULL;
  se->head = NULL;
  se->current = NULL;
  se->tail = NULL;
  return se;
}

struct SegmentEntery * initBufferEntry(struct HTTPBuffer * hb)
{
  if(hb == NULL) return NULL;
  if(hb->entry == NULL) hb->entry = new_SegmentEntry();
  return hb->entry;
}

void free_SegmentEntry(struct SegmentEntery * entry)
{
  if(entry == NULL) return;
  freeBufSegment(entry->head);
  free(entry);
  return;
}

struct HTTPBuffer * initHTTPBuffer(int size)
{
  struct HTTPBuffer * hb;
  if((hb = malloc(sizeof(*hb))) == NULL) return NULL;
  memset(hb,0,sizeof(*hb));
  hb->rdBuf = malloc(size);
  hb->wrBuf = NULL;
  if(hb->rdBuf != NULL) hb->maxBytes = size;
  hb->entry = initBufferEntry(hb);
  return hb;
}


void freeHTTPBuffer(struct HTTPBuffer * hb)
{
  if(hb == NULL) return;
  free(hb->rdBuf);
  free_SegmentEntry(hb->entry);
  free(hb);
  return;
}

struct BufferSegment * createBufSegment(int start,int length)
{
  struct BufferSegment * bs;
  if((bs = malloc(sizeof(*bs))) == NULL) return NULL;
  memset(bs,0,sizeof(*bs));
  bs->start = start;
  bs->length = length;
  bs->hc = NULL;
  return bs;
}

void freeBufSegment(struct BufferSegment * bs)
{
  struct BufferSegment * next = NULL;
  for(; bs != NULL; bs=next){
    next = bs->next;
    free(bs);
  }
  return;
}

struct HTTPConnection * new_connect(int fd)
{
  struct HTTPConnection * hc;
  if((hc = malloc(sizeof(*hc))) == NULL){
    logerr("malloc memory for http connection error", errno);
    return NULL;
  }
  hc->fd = fd;
  hc->count = 0;
  hc->events = 0;
  if((hc->buf = initHTTPBuffer(4096*64)) == NULL){
    logerr("malloc memory for http buffer error", errno);
  }
  return hc;
}

void free_connect(struct HTTPConnection * hc)
{
  if(hc == NULL) return;
  freeHTTPBuffer(hc->buf);
  hc->buf = NULL;
  free(hc);
  return;
}
