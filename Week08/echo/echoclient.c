#include "csapp.h"

int main(int argc, char **argv) 
{
    int clientfd;
    char *host, *port, buf[MAXLINE];
    rio_t rio;

    // argc: 프로그램을 실행할 때 명령줄에서 입력한 인자의 개수를 나타내는 변수다.
    // argv: 프로그램을 실행할 때 명령줄에서 입력한 인자들이 담긴 배열이다.
    // ./echoserveri 127.0.0.1 8080 -> argv[0]: ./echoserver, argv[1]: 127.0.0.1, argv[2]: 8080

    // if (argc != 3) { ... }: 입력 받은 인자가 3개가 아닐 경우 예외처리하는 부분이다. (무조건 파일명, 주소, 포트만 받기에)
    if (argc != 3) {
	fprintf(stderr, "usage: %s <host> <port>\n", argv[0]);
	exit(0);
    }

    // ./echoserveri 127.0.0.1 8080 -> argv[0]: ./echoserver, argv[1]: 127.0.0.1, argv[2]: 8080
    host = argv[1];
    port = argv[2];

    // 입력 받은 호스트와 포트번호를 기반으로 소켓을 생성하고,
    // 서버에 연결(connect)한다.
    // 그 후, 반환된 FD 값을 clientfd 변수에 대입한다. (연결된 서버의 FD 값)
    clientfd = Open_clientfd(host, port);

    // 입력을 받기 위해 버퍼를 초기화시켜준다.
    Rio_readinitb(&rio, clientfd);

    // While (...) ->  사용자가 키보드(표준입력, stdin)로 한 줄 입력할 때마다, 그걸 buf(버퍼)에 저장하고 반복을 계속한다.
    while (Fgets(buf, MAXLINE, stdin) != NULL) {

    // 버퍼(buf)에 저장된 데이터를 한 줄씩,
    // 그 길이(strlen(buf))만큼 서버 소켓(clientfd)으로 전송한다.
	Rio_writen(clientfd, buf, strlen(buf));

    // 서버가 돌려보낸 데이터를 한 줄씩 읽어와서 버퍼에 저장한다.
	Rio_readlineb(&rio, buf, MAXLINE);

    // 버퍼에 있는 데이터를 터미널에 출력한다.
	Fputs(buf, stdout);
    }

    // 서버와 연결을 끊는다.
    Close(clientfd); 

    // 프로그램 종료
    exit(0);
}