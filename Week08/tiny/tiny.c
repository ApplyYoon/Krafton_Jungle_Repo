/* Tiny: a simple iterative HTTP/1.0 web server */
/* Based on CS:APP3e tiny.c with minor cleanups */
#include "csapp.h"

void doit(int fd);
void read_requesthdrs(rio_t *rp);
int  parse_uri(char *uri, char *filename, char *cgiargs);
void serve_static(int fd, char *filename, int filesize);
void get_filetype(char *filename, char *filetype);
void serve_dynamic(int fd, char *filename, char *cgiargs);
void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg);

int main(int argc, char **argv)
{
    int listenfd, connfd;
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;
    char hostname[MAXLINE], port[MAXLINE];

    // 사용자에게 입력 받은 인자가 2개가 아닐 경우 종료시킨다.
    if (argc != 2) {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(1);
    }

    // argv[1] -> port 번호
    // Open_listenfd() 함수를 통해 소켓 생성, 바인딩 후 Listen 상태 유지
    // 반환 값(생성된 소켓의 FD번호)을 listenfd 변수에 대입
    listenfd = Open_listenfd(argv[1]);

    // 무한 루프: 서버는 게속해서 클라이언트 요청을 기다린다.
    while (1) {

        // 클라이언트의 주소 정보를 저장할 구조체의 크기를 준비한다.
        // (Accept가 여기에 실제 클라이언트의 주소를 채워넣는다)
        clientlen = sizeof(clientaddr);

        // 클라이언트 연결 수락 (요청이 올 떄 까지 블로킹됨)
        // listenfd : 29번줄에서 만든 대기(Listen) 소켓 FD 번호
        // connfd   : 클라이언트와 연결된 연결(Connection) 소켓 FD 번호
        // -> Accpet() 함수의 반환값은 연결된 클라이언트의 소켓 FD 번호다.
        connfd = Accept(listenfd, (SA*)&clientaddr, &clientlen);

        // 클라이언트의 IP 주소와 포트 번호를 사람이 읽을 수 있는 형태로 변환한다.
        // ex) 127.0.0.1 -> localhost
        Getnameinfo((SA*)&clientaddr, clientlen, hostname, MAXLINE, port, MAXLINE, 0);

        // 연결이 성공했음을 콘솔에 출력한다 (로그용)
        // printf result) Accepted connection from (localhost, 54321)
        printf("Accepted connection from (%s, %s)\n", hostname, port);

        // 핵심 처리부: 요청을 읽고, !!정적/동적 콘텐츠를 판별하고!!, 응답을 생성한다.
        // doit() 함수 내부에서 HTTP 요청 분석, serve static/serve_dynamic 실행이 이루어진다.
        doit(connfd);

        // 요청 처리가 끝났으므로 클라이언트와의 연결을 종료한다.
        // TCP 연결은 반드시 close()로 닫아줘야 한다 (리소스 누수 방지)
        // 다만 CS:APP 교재에서 wapper 함수 Close()를 지원해주기에 대신 사용한다. (예외처리 추가됨)
        Close(connfd);
    }
}

void doit(int fd)
{
    // 클라이언트 요청을 처리하기 위한 변수를 선언한다.
    int is_static;                     // 정적 콘텐츠(1) / 동적 콘텐츠(0) 판별 변수
    struct stat sbuf;                  // 파일 정보 저장용 (크기, 권한 등)
    char buf[MAXLINE];                 // 요청 라인 및 헤더를 읽을 버퍼
    char method[MAXLINE];              // HTTP 메서드 (GET 등)
    char uri[MAXLINE];                 // 요청된 URI (/index.html 등)
    char version[MAXLINE];             // HTTP 버전 (HTTP/1.0 등)
    char filename[MAXLINE];            // 요청된 파일 이름 (실제 파일 경로)
    char cgiargs[MAXLINE];             // CGI 인자 (동적 콘텐츠용)
    rio_t rio;                         // Robust I/O 구조체 (입출력 버퍼 관리)


    // Robust I/O 초기화: fd(소켓)을 rio에 연결한다.
    Rio_readinitb(&rio, fd);

    // 요청 라인의 첫 줄을 읽는다. ex) "GET /index.html HTTP/1.0"
    if (!Rio_readlineb(&rio, buf, MAXLINE)) return;        

    // 요청 라인을 파싱한다. (공백 기준 분리)
    sscanf(buf, "%s %s %s", method, uri, version);

    // Tiny는 GET 메서드만 지원한다. (POST, HEAD 등은 미구현)
    if (strcasecmp(method, "GET")) {
        clienterror(fd, method, "501", "Not Implemented",
                    "Tiny does not implement this method");
        return;
    }

    // 요청 헤더들을 읽는다.
    read_requesthdrs(&rio);

    // parse_uri() 함수를 통해 URI를 분석하여 filename, cgiargs 추출 및
    // 정적/동적 콘텐츠 판단한다. (정적이면 filename만, 동적이면 cgiargs도 함께 저장)
    is_static = parse_uri(uri, filename, cgiargs);

    // 요청한 파일의 존재 여부를 stat()으로 확인한다.
    // (파일 정보가 sbuf에 저장됨)
    if (stat(filename, &sbuf) < 0) {
        clienterror(fd, filename, "404", "Not found",
                    "Tiny couldn't find this file");  // 파일 없으면 404 반환
        return;
    }

    // 정적 콘텐츠 처리
    if (is_static) {  
        // 요청된 파일이 일반 파일인지, 읽기 권한이 있는지를 검사한다
        if (!(S_ISREG(sbuf.st_mode)) || !(S_IRUSR & sbuf.st_mode)) {
            clienterror(fd, filename, "403", "Forbidden",
                        "Tiny couldn't read the file");    // 읽기 불가 -> 403 반횐
            return;
        }

        // 정적 콘텐츠 전송 함수 호출
        serve_static(fd, filename, sbuf.st_size);
    }

    // 동적 콘텐츠 처리 (CGI)
    else {      
        // 요청된 파일이 실행 가능한 일반 파일인지 검사한다
        if (!(S_ISREG(sbuf.st_mode)) || !(S_IXUSR & sbuf.st_mode)) {
            clienterror(fd, filename, "403", "Forbidden",
                        "Tiny couldn't run the CGI program"); // 싱행 불가 -> 403 반환
            return;
        }

        // CGI 프로그램 실행 (자식 프로세스 생성 -> 환경변수 설정 -> Exeeve 실행)
        serve_dynamic(fd, filename, cgiargs);
    }
}

void read_requesthdrs(rio_t *rp)
{
    char buf[MAXLINE];
    do {
        Rio_readlineb(rp, buf, MAXLINE);
        if (!strcasecmp(buf, "\r\n")) break;
    } while (1);
}

int parse_uri(char *uri, char *filename, char *cgiargs)
{
    char *ptr;

    if (!strstr(uri, "cgi-bin")) { // static
        strcpy(cgiargs, "");
        strcpy(filename, ".");
        strcat(filename, uri);
        if (uri[strlen(uri)-1] == '/')
            strcat(filename, "home.html");
        return 1;
    }
    else { // dynamic
        ptr = index(uri, '?');
        if (ptr) {
            *ptr = '\0';
            strcpy(cgiargs, ptr+1);
        } else strcpy(cgiargs, "");
        strcpy(filename, ".");
        strcat(filename, uri);
        return 0;
    }
}

void serve_static(int fd, char *filename, int filesize)
{
    int srcfd;
    char *srcp;
    char filetype[MAXLINE], buf[MAXBUF];

    get_filetype(filename, filetype);
    // Response headers
    snprintf(buf, sizeof(buf),
        "HTTP/1.0 200 OK\r\n"
        "Server: Tiny Web Server\r\n"
        "Connection: close\r\n"
        "Content-length: %d\r\n"
        "Content-type: %s\r\n\r\n",
        filesize, filetype);
    Rio_writen(fd, buf, strlen(buf));

    // Send file body
    srcfd = Open(filename, O_RDONLY, 0);
    srcp  = Mmap(0, filesize, PROT_READ, MAP_PRIVATE, srcfd, 0);
    Close(srcfd);
    Rio_writen(fd, srcp, filesize);
    Munmap(srcp, filesize);
}

void get_filetype(char *filename, char *filetype)
{
    if (strstr(filename, ".html"))      strcpy(filetype, "text/html");
    else if (strstr(filename, ".gif"))  strcpy(filetype, "image/gif");
    else if (strstr(filename, ".png"))  strcpy(filetype, "image/png");
    else if (strstr(filename, ".jpg"))  strcpy(filetype, "image/jpeg");
    else if (strstr(filename, ".css"))  strcpy(filetype, "text/css");
    else if (strstr(filename, ".js"))   strcpy(filetype, "application/javascript");
    else                                 strcpy(filetype, "text/plain");
}

void serve_dynamic(int fd, char *filename, char *cgiargs)
{
    char buf[MAXLINE];
    char *emptylist[] = { NULL };

    // Minimal headers for CGI response start (some CGI print their own headers)
    // (주: adder.c는 스스로 Content-type 헤더를 출력합니다)

    if (Fork() == 0) { // child
        Setenv("QUERY_STRING", cgiargs, 1);
        Dup2(fd, STDOUT_FILENO);              // stdout -> client
        Execve(filename, emptylist, environ); // run CGI
    }
    Wait(NULL); // parent waits
}

void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg)
{
    char buf[MAXBUF], body[MAXBUF];

    // Build the HTTP response body
    snprintf(body, sizeof(body),
        "<html><title>Tiny Error</title>"
        "<body bgcolor=\"ffffff\">\r\n"
        "%s: %s\r\n<p>%s: %s\r\n"
        "<hr><em>The Tiny Web server</em>\r\n",
        errnum, shortmsg, longmsg, cause);

    // Print the HTTP response
    snprintf(buf, sizeof(buf), "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
    Rio_writen(fd, buf, strlen(buf));
    snprintf(buf, sizeof(buf), "Content-type: text/html\r\n");
    Rio_writen(fd, buf, strlen(buf));
    snprintf(buf, sizeof(buf), "Content-length: %zu\r\n\r\n", strlen(body));
    Rio_writen(fd, buf, strlen(buf));
    Rio_writen(fd, body, strlen(body));
}