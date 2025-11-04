/* A minimal CGI program that adds two integers from QUERY_STRING */
// QUERY_STRING 환경변수로부터 두 정수를 받아 더한 결과를 HTML로 출력하는 간단한 CGI 프로그램
#include "csapp.h"  // CS:APP 라이브러리 (I/O 및 에러 처리 함수)

int main(void)
{
    // 포인터 및 버퍼 변수 선언
    char *buf, *p;           // 환경 변수 QUERY_STRING을 저장할 포인터, '&' 구분자 위치 저장용 포인터
    char arg1[64], arg2[64]; // 각각 첫 번째와 두 번째 인자(정수) 저장
    int n1=0, n2=0;          // 정수로 변환된 두 인자값

    // -------------------------------------------------------------
    // [1] CGI는 반드시 Content-type 헤더를 출력해야 함.
    //     브라우저가 HTML로 인식할 수 있도록 먼저 헤더를 보냄.
    // -------------------------------------------------------------
    printf("Content-type: text/html\r\n\r\n");

    // HTML 문서의 기본 구조 시작
    printf("<html><head><title>add</title></head><body>\n");
    printf("<h2>Welcome to add.com</h2>\n");

    // -------------------------------------------------------------
    // [2] 환경 변수 QUERY_STRING에서 인자 가져오기
    //     Tiny의 serve_dynamic()이 Setenv("QUERY_STRING", cgiargs, 1)로 설정해줌
    //     예: QUERY_STRING="3&5"
    // -------------------------------------------------------------
    if ((buf = getenv("QUERY_STRING")) != NULL) {

        // '&' 문자를 찾아서 두 개의 인자를 분리 ("3" 과 "5")
        if ((p = strchr(buf, '&')) != NULL) {
            *p = '\0'; // '&'를 문자열 종료 문자('\0')로 바꿔서 buf를 "3"으로 자름
            // '&' 뒤는 "5"가 됨

            // arg1, arg2에 각각 복사 (strncpy로 안전하게)
            strncpy(arg1, buf, sizeof(arg1)-1); 
            arg1[sizeof(arg1)-1] = '\0'; // 널 종료 보장

            strncpy(arg2, p+1, sizeof(arg2)-1); 
            arg2[sizeof(arg2)-1] = '\0'; // 널 종료 보장

            // 문자열을 정수로 변환
            n1 = atoi(arg1); 
            n2 = atoi(arg2);

            // 결과를 HTML로 출력
            printf("<p>The answer is: %d + %d = %d</p>\n", n1, n2, n1+n2);
        } 
        // '&' 문자가 없는 경우 (잘못된 요청)
        else {
            printf("<p>Usage: /cgi-bin/adder?A&B</p>\n");
        }
    } 
    // QUERY_STRING이 존재하지 않는 경우
    else {
        printf("<p>No QUERY_STRING provided.</p>\n");
    }

    // HTML 문서 마무리
    printf("</body></html>\n");

    // 프로그램 정상 종료
    return 0;
}