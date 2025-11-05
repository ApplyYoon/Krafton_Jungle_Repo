#include <stdio.h>
#include "csapp.h"
#include <string.h>

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr =
    "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 "
    "Firefox/10.0.3\r\n";

int main(int argc, char **argv) {
  int listenfd, connfd;
  socklen_t clientlen;
  struct sockaddr_storage clientaddr;
  char hostname[MAXLINE], port[MAXLINE];

  // 사용자에게 입력 받은 인자가 2개가 아닐 경우 종료시킨다.
  if (argc != 2) {
      fprintf(stderr, "usage: %s <port>\n", argv[0]);
      exit(1);
  }

  listenfd = Open_listenfd(argv[1]);

  while (1)
  {
    clientlen = sizeof(clientaddr);
    connfd = Accept(listenfd, (SA*)&clientaddr, &clientlen);
    Getnameinfo((SA*)&clientaddr, clientlen, hostname, MAXLINE, port, MAXLINE, 0);
    printf("Accepted connection from (%s, %s)\n", hostname, port);
    doit(connfd);
    Close(connfd);
  }
  
  return 0;
}

void doit(int fd) {
    int is_static;                     // 정적 콘텐츠(1) / 동적 콘텐츠(0) 판별 변수
    struct stat sbuf;                  // 파일 정보 저장용 (크기, 권한 등)
    char buf[MAXLINE];                 // 요청 라인 및 헤더를 읽을 버퍼
    char method[MAXLINE];              // HTTP 메서드 (GET 등)
    char uri[MAXLINE];                 // 요청된 URI (/index.html 등)
    char version[MAXLINE];             // HTTP 버전 (HTTP/1.0 등)
    char filename[MAXLINE];            // 요청된 파일 이름 (실제 파일 경로)
    char cgiargs[MAXLINE];             // CGI 인자 (동적 콘텐츠용)
    rio_t rio;                         // Robust I/O 구조체 (입출력 버퍼 관리)

    Rio_readinitb(&rio, fd);

    if (!Rio_readlineb(&rio, buf, MAXLINE)) return;

    sscanf(buf, "%s %s %s", method, uri, version);

    // GET만 지원
    if (strcasecmp(method, "GET")) {
        clienterror(fd, method, "501", "Not Implemented",
                    "Tiny does not implement this method");
        return;
    }
  
    // URI 파싱 함수 호출
    char host[MAXLINE], port[MAXLINE], path[MAXLINE];
    parse_uri(uri, host, port, path);
    
    printf("! Result ! \n\n");
    printf("uri: %s\n", uri);
    printf("host: %s\n", host);
    printf("path: %s\n", path);
    printf("port: %s\n", port);

    char http_header[MAXLINE];

    /* 1. 원 서버에 연결 (host, port 사용) */
    int serverfd = Open_clientfd(host, port);
    if (serverfd < 0) {
        printf("connection failed to %s:%s\n", host, port);
        return;
    }
    build_http_header(http_header, host, path, &rio);
    Rio_writen(serverfd, http_header, strlen(http_header));

    /* 2. 서버 응답을 읽어서 클라이언트에게 전달 */
    rio_t rio_server;
    ssize_t n;

    Rio_readinitb(&rio_server, serverfd);
    while ((n = Rio_readnb(&rio_server, buf, MAXLINE)) > 0) {
        Rio_writen(fd, buf, n);
    }

    Close(serverfd);
}

void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg)
{
    char buf[MAXBUF], body[MAXBUF];   // HTTP 헤더용(buf)와 HTML 본문용(body) 버퍼
       
    snprintf(body, sizeof(body),
        "<html><title>Tiny Error</title>"          // HTML 제목
        "<body bgcolor=\"ffffff\">\r\n"            // 배경 흰색
        "%s: %s\r\n<p>%s: %s\r\n"                  // 상태 코드, 메시지, 설명, 원인
        "<hr><em>The Tiny Web server</em>\r\n",    // 서버 이름 표시 (footer)
        errnum, shortmsg, longmsg, cause);

    // body 결과 예시:
    // <html><title>Tiny Error</title><body bgcolor="ffffff">
    // 404: Not found
    // <p>Tiny couldn't find this file: ./none.html
    // <hr><em>The Tiny Web server</em>

    // 상태 줄(Status line): HTTP 버전, 상태 코드, 상태 메시지
    snprintf(buf, sizeof(buf), "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
    Rio_writen(fd, buf, strlen(buf));

    // 헤더 1: 콘텐츠 타입(Content-type)
    snprintf(buf, sizeof(buf), "Content-type: text/html\r\n");
    Rio_writen(fd, buf, strlen(buf));
    
    // 헤더 2: 콘텐츠 길이(Content-length)
    snprintf(buf, sizeof(buf), "Content-length: %zu\r\n\r\n", strlen(body));
    Rio_writen(fd, buf, strlen(buf));

    // 실제 본문 전송
    Rio_writen(fd, body, strlen(body));
}

void parse_uri(const char *uri, char *host, char *port, char *path) {
    const char *host_start, *path_start;
    char authority[1024];
    const char *colon;

    /* 1. "http://" 부분 건너뛰기 */
    host_start = strstr(uri, "//");
    if (host_start)
        host_start += 2;   // "http://" 이후부터
    else
        host_start = uri;  // "http://" 없을 경우 대비

    /* 2. path 시작점 찾기 */
    path_start = strchr(host_start, '/');

    /* 3. host+port 영역(authority) 추출 */
    if (path_start) {
        size_t auth_len = path_start - host_start;
        strncpy(authority, host_start, auth_len);
        authority[auth_len] = '\0';
    } else {
        strcpy(authority, host_start);
    }

    /* 4. authority 안에서 ':' 찾아 port 구분 */
    colon = strchr(authority, ':');
    if (colon) {
        * (char *)colon = '\0';     // host와 port 분리
        strcpy(host, authority);
        strcpy(port, colon + 1);
    } else {
        strcpy(host, authority);
        strcpy(port, "80");         // 기본 포트
    }

    /* 5. path 설정 */
    if (path_start)
        strcpy(path, path_start);
    else
        strcpy(path, "/");
}

void build_http_header(char *http_header, char *host, char *path, rio_t *client_rio) {
    char buf[MAXLINE], request[MAXLINE];
    char other_hdr[MAXLINE] = "";
    char host_hdr[MAXLINE] = "";

    /* 1. 요청 라인 만들기 */
    sprintf(request, "GET %s HTTP/1.0\r\n", path);

    /* 2. 클라이언트가 보낸 헤더 읽기 */
    while (Rio_readlineb(client_rio, buf, MAXLINE) > 0) {
        if (!strcmp(buf, "\r\n"))  // 빈 줄 -> 헤더 끝
            break;

        /* Host 헤더 따로 저장 */
        if (!strncasecmp(buf, "Host:", 5)) {
            strcpy(host_hdr, buf);
            continue;
        }

        /* 아래 4개는 우리가 새로 넣을 거니까 복사 안 함 */
        if (strncasecmp(buf, "Connection:", 11)
            && strncasecmp(buf, "Proxy-Connection:", 17)
            && strncasecmp(buf, "User-Agent:", 11)) {
            strcat(other_hdr, buf);
        }
    }

    /* 3. Host 헤더 없으면 직접 추가 */
    if (strlen(host_hdr) == 0)
        sprintf(host_hdr, "Host: %s\r\n", host);

    /* 4. 최종 헤더 조립 */
    sprintf(http_header, "%s%s%s%s%s%s",
            request,
            host_hdr,
            "Connection: close\r\n",
            "Proxy-Connection: close\r\n",
            "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3\r\n",
            other_hdr);

    /* 5. 헤더 끝 표시 */
    strcat(http_header, "\r\n");
}