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

    if (argc != 2) {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(1);
    }

    listenfd = Open_listenfd(argv[1]);
    while (1) {
        clientlen = sizeof(clientaddr);
        connfd = Accept(listenfd, (SA*)&clientaddr, &clientlen);
        Getnameinfo((SA*)&clientaddr, clientlen, hostname, MAXLINE, port, MAXLINE, 0);
        printf("Accepted connection from (%s, %s)\n", hostname, port);
        doit(connfd);
        Close(connfd);
    }
}

void doit(int fd)
{
    int is_static;
    struct stat sbuf;
    char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
    char filename[MAXLINE], cgiargs[MAXLINE];
    rio_t rio;

    Rio_readinitb(&rio, fd);
    if (!Rio_readlineb(&rio, buf, MAXLINE)) return;         // request line
    sscanf(buf, "%s %s %s", method, uri, version);

    // Only GET is supported (optional: add HEAD/POST later)
    if (strcasecmp(method, "GET")) {
        clienterror(fd, method, "501", "Not Implemented",
                    "Tiny does not implement this method");
        return;
    }

    // Read and ignore headers for now
    read_requesthdrs(&rio);

    // Parse URI into filename and (maybe) cgiargs
    is_static = parse_uri(uri, filename, cgiargs);
    if (stat(filename, &sbuf) < 0) {
        clienterror(fd, filename, "404", "Not found",
                    "Tiny couldn't find this file");
        return;
    }

    if (is_static) {  // serve static
        if (!(S_ISREG(sbuf.st_mode)) || !(S_IRUSR & sbuf.st_mode)) {
            clienterror(fd, filename, "403", "Forbidden",
                        "Tiny couldn't read the file");
            return;
        }
        serve_static(fd, filename, sbuf.st_size);
    } else {          // serve dynamic (CGI)
        if (!(S_ISREG(sbuf.st_mode)) || !(S_IXUSR & sbuf.st_mode)) {
            clienterror(fd, filename, "403", "Forbidden",
                        "Tiny couldn't run the CGI program");
            return;
        }
        serve_dynamic(fd, filename, cgiargs);
    }
}

void read_requesthdrs(rio_t *rp)
{
    char buf[MAXLINE];
    do {
        Rio_readlineb(rp, buf, MAXLINE);
        if (!strcasecmp(buf, "\r\n")) break; // empty line
        // (optional) printf("%s", buf); // for 11.6 header print
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