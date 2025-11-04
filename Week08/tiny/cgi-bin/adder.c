/* A minimal CGI program that adds two integers from QUERY_STRING */
#include "csapp.h"

int main(void)
{
    char *buf, *p;
    char arg1[64], arg2[64];
    int n1=0, n2=0;

    // CGI must write Content-type header then a blank line
    printf("Content-type: text/html\r\n\r\n");
    printf("<html><head><title>add</title></head><body>\n");
    printf("<h2>Welcome to add.com</h2>\n");

    if ((buf = getenv("QUERY_STRING")) != NULL) {
        if ((p = strchr(buf, '&')) != NULL) {
            *p = '\0';
            strncpy(arg1, buf, sizeof(arg1)-1); arg1[sizeof(arg1)-1] = '\0';
            strncpy(arg2, p+1, sizeof(arg2)-1); arg2[sizeof(arg2)-1] = '\0';
            n1 = atoi(arg1); n2 = atoi(arg2);
            printf("<p>The answer is: %d + %d = %d</p>\n", n1, n2, n1+n2);
        } else {
            printf("<p>Usage: /cgi-bin/adder?A&B</p>\n");
        }
    } else {
        printf("<p>No QUERY_STRING provided.</p>\n");
    }

    printf("</body></html>\n");
    return 0;
}