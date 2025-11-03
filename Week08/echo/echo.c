/*
 * echo - read and echo text lines until client closes connection
 */
/* $begin echo */
#include "csapp.h"

void echo(int connfd) 
{
    size_t n; 
    char buf[MAXLINE]; 
    rio_t rio;

    // 버퍼를 초기화한다.
    Rio_readinitb(&rio, connfd);

    // 클라이언트가 보낸 데이터를 한 줄씩 읽어서(buf에 저장),
    // 읽은 데이터의 길이(n)가 0이 될 때까지(즉, 연결이 종료될 때까지) 반복한다.
    while((n = Rio_readlineb(&rio, buf, MAXLINE)) != 0) { 
	printf("server received %d bytes\n", (int)n);

    // 버퍼(buf)에 저장된 데이터를 한 줄씩,
    // 그 길이(strlen(buf))만큼 클라이언트 소켓(connfd)으로 전송한다.
	Rio_writen(connfd, buf, n);
    }
}

