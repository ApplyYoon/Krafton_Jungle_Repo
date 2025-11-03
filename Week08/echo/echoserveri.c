#include "csapp.h"
#include "echo.c"

void echo(int connfd);

int main(int argc, char **argv) {
    int listenfd, connfd;
    socklen_t clientlen;
    struct sockaddr_storage clientaddr; // 클라이언트 주소 정보
    char client_hostname[MAXLINE], client_port[MAXLINE];

    // argc: 프로그램을 실행할 때 명령줄에서 입력한 인자의 개수를 나타내는 변수다.

    // argv: 프로그램을 실행할 때 명령줄에서 입력한 인자들이 담긴 배열이다.
    // ./echoserveri 8080 -> argv[0]: ./echoserver, argv[1]: 8080

    // if (argc != 2) { ... }: 입력 받은 인자가 2개가 아닐 경우 예외처리하는 부분이다. (무조건 파일명과, 포트만 받기에)
    if (argc != 2) {
        fprintf(stderr, "사용법: %s <port>\n", argv[0]);
        exit(0);
    }

    // 입력 받은 포트번호를 기반으로 소켓을 생성하고, 바인딩하고, 대기 상태로 만든다.
    // 그 후, 반환된 FD 값을 listenfd 변수에 대입한다.
    listenfd = Open_listenfd(argv[1]); 

    while (1) {

        // 구조체의 크기 구해 변수에 대입한다.
        clientlen = sizeof(struct sockaddr_storage);

        // 클라이언트가 Connect()를 호출했을 때 그 요청을 받아들이고,
        // 두 프로그램 간의 연결을 맺어 새로운 통신 소켓(connfd)을 반환한다.
        connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);

        // 연결된 클라이 트의 정보를 사람이 읽기 쉽게 출력한다.
        Getnameinfo((SA *)&clientaddr, clientlen,
                    client_hostname, MAXLINE,
                    client_port, MAXLINE, 0);
        printf("클라이언트 연결됨: (%s, %s)\n", client_hostname, client_port);

        // echo 함수를 호출하여 로직을 처리한다.
        echo(connfd); 

        // 연결 종료
        Close(connfd); 
    }
}

