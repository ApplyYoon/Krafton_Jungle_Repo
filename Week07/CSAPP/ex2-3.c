#include <unistd.h>
#include <stdint.h>
#include <stdio.h>

int main(void) {
    void *b1 = sbrk(0);
    printf("Before: %p\n", b1);

    sbrk(1);
    void *b2 = sbrk(0);
    printf("After sbrk(1): %p (diff = %ld)\n", b2, (char *)b2 - (char *)b1);

    sbrk(1);
    void *b3 = sbrk(0);
    printf("After sbrk(2): %p (diff = %ld)\n", b3, (char *)b3 - (char *)b2);

    return 0;
}