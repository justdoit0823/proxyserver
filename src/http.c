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

/*

  Hypertext Transfer Protocol -- HTTP/1.1(RFC2616)

  HTTP-message   = Request | Response

  message-header = field-name ":" [ field-value ]
  field-name     = token
  field-value    = *( field-content | LWS )
  field-content  = <the OCTETs making up the field-value
                    and consisting of either *TEXT or combinations
                    of token, separators, and quoted-string>

  message-body = entity-body
               | <entity-body encoded as per Transfer-Encoding>

  entity-header  = Allow                    ; Section 14.7
                 | Content-Encoding         ; Section 14.11
                 | Content-Language         ; Section 14.12
                 | Content-Length           ; Section 14.13
                 | Content-Location         ; Section 14.14
                 | Content-MD5              ; Section 14.15
                 | Content-Range            ; Section 14.16
                 | Content-Type             ; Section 14.17
                 | Expires                  ; Section 14.21
                 | Last-Modified            ; Section 14.29
                 | extension-header

  extension-header = message-header

  entity-body    = *OCTET

  entity-body := Content-Encoding( Content-Type( data ) )

*/

int parseRequest(const char * requeststr,struct requestHeader * header)
{
  /*

    The format of Request(RFC2616)

    Request       = Request-Line              ; Section 5.1
                    *(( general-header        ; Section 4.5
                         | request-header         ; Section 5.3
                         | entity-header ) CRLF)  ; Section 7.1
                        CRLF
                        [ message-body ]          ; Section 4.3

    general-header = Cache-Control            ; Section 14.9
                      | Connection               ; Section 14.10
                      | Date                     ; Section 14.18
                      | Pragma                   ; Section 14.32
                      | Trailer                  ; Section 14.40
                      | Transfer-Encoding        ; Section 14.41
                      | Upgrade                  ; Section 14.42
                      | Via                      ; Section 14.45
                      | Warning                  ; Section 14.46

    request-header = Accept                   ; Section 14.1
                      | Accept-Charset           ; Section 14.2
                      | Accept-Encoding          ; Section 14.3
                      | Accept-Language          ; Section 14.4
                      | Authorization            ; Section 14.8
                      | Expect                   ; Section 14.20
                      | From                     ; Section 14.22
                      | Host                     ; Section 14.23
                      | If-Match                 ; Section 14.24
		      | If-Modified-Since        ; Section 14.25
                      | If-None-Match            ; Section 14.26
                      | If-Range                 ; Section 14.27
                      | If-Unmodified-Since      ; Section 14.28
                      | Max-Forwards             ; Section 14.31
                      | Proxy-Authorization      ; Section 14.34
                      | Range                    ; Section 14.35
                      | Referer                  ; Section 14.36
                      | TE                       ; Section 14.39
                      | User-Agent               ; Section 14.43

    
    Request-Line   = Method SP Request-URI SP HTTP-Version CRLF

    Method         = "OPTIONS"                ; Section 9.2
                      | "GET"                    ; Section 9.3
                      | "HEAD"                   ; Section 9.4
                      | "POST"                   ; Section 9.5
                      | "PUT"                    ; Section 9.6
                      | "DELETE"                 ; Section 9.7
                      | "TRACE"                  ; Section 9.8
                      | "CONNECT"                ; Section 9.9
                      | extension-method
       extension-method = token

    Request-URI    = "*" | absoluteURI | abs_path | authority

  */

  char * line, * split, * headend, * hostptr;
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
  hostptr = strstr(line, "Host:");
  if(hostptr == NULL)
  {
	  printf("invalid http header!\n");
	  return HTTP_INVALID_HEADER;
  }
  if(sscanf(hostptr,"%s%s",optionkey,header->host) != 2){
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
  /*

    Response      = Status-Line               ; Section 6.1
    *(( general-header        ; Section 4.5
                        | response-header        ; Section 6.2
                        | entity-header ) CRLF)  ; Section 7.1
                       CRLF
                       [ message-body ]          ; Section 7.2

    Status-Line = HTTP-Version SP Status-Code SP Reason-Phrase CRLF

    Status-Code    =
            "100"  ; Section 10.1.1: Continue
          | "101"  ; Section 10.1.2: Switching Protocols
          | "200"  ; Section 10.2.1: OK
          | "201"  ; Section 10.2.2: Created
          | "202"  ; Section 10.2.3: Accepted
          | "203"  ; Section 10.2.4: Non-Authoritative Information
          | "204"  ; Section 10.2.5: No Content
          | "205"  ; Section 10.2.6: Reset Content
          | "206"  ; Section 10.2.7: Partial Content
          | "300"  ; Section 10.3.1: Multiple Choices
          | "301"  ; Section 10.3.2: Moved Permanently
          | "302"  ; Section 10.3.3: Found
          | "303"  ; Section 10.3.4: See Other
          | "304"  ; Section 10.3.5: Not Modified
          | "305"  ; Section 10.3.6: Use Proxy
          | "307"  ; Section 10.3.8: Temporary Redirect
          | "400"  ; Section 10.4.1: Bad Request
          | "401"  ; Section 10.4.2: Unauthorized
          | "402"  ; Section 10.4.3: Payment Required
          | "403"  ; Section 10.4.4: Forbidden
          | "404"  ; Section 10.4.5: Not Found
          | "405"  ; Section 10.4.6: Method Not Allowed
          | "406"  ; Section 10.4.7: Not Acceptable
	  | "407"  ; Section 10.4.8: Proxy Authentication Required
          | "408"  ; Section 10.4.9: Request Time-out
          | "409"  ; Section 10.4.10: Conflict
          | "410"  ; Section 10.4.11: Gone
          | "411"  ; Section 10.4.12: Length Required
          | "412"  ; Section 10.4.13: Precondition Failed
          | "413"  ; Section 10.4.14: Request Entity Too Large
          | "414"  ; Section 10.4.15: Request-URI Too Large
          | "415"  ; Section 10.4.16: Unsupported Media Type
          | "416"  ; Section 10.4.17: Requested range not satisfiable
          | "417"  ; Section 10.4.18: Expectation Failed
          | "500"  ; Section 10.5.1: Internal Server Error
          | "501"  ; Section 10.5.2: Not Implemented
          | "502"  ; Section 10.5.3: Bad Gateway
          | "503"  ; Section 10.5.4: Service Unavailable
          | "504"  ; Section 10.5.5: Gateway Time-out
          | "505"  ; Section 10.5.6: HTTP Version not supported
          | extension-code

    extension-code = 3DIGIT

    Reason-Phrase  = *<TEXT, excluding CR, LF>

    response-header = Accept-Ranges           ; Section 14.5
                       | Age                     ; Section 14.6
                       | ETag                    ; Section 14.19
                       | Location                ; Section 14.30
                       | Proxy-Authenticate      ; Section 14.33
		       | Retry-After             ; Section 14.37
                       | Server                  ; Section 14.38
                       | Vary                    ; Section 14.44
                       | WWW-Authenticate        ; Section 14.47

  */

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
      /*

	The format of Chunked Transfer Coding(RFC2616)

	Chunked-Body   = *chunk
                        last-chunk
                        trailer
                        CRLF

       chunk          = chunk-size [ chunk-extension ] CRLF
                        chunk-data CRLF
       chunk-size     = 1*HEX
       last-chunk     = 1*("0") [ chunk-extension ] CRLF

       chunk-extension= *( ";" chunk-ext-name [ "=" chunk-ext-val ] )
       chunk-ext-name = token
       chunk-ext-val  = token | quoted-string
       chunk-data     = chunk-size(OCTET)
       trailer        = *(entity-header CRLF)

      */

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
  sighandler_t presighandler = signal(SIGALRM, timeout);
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
