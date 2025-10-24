#include <unistd.h>
#include <stdint.h>
#include <stdio.h>

int main(void) {
    void *before = sbrk(0);
    printf("Before: %p\n", before);

    sbrk(1);
    void *after = sbrk(0);
    printf("After : %p\n", after);

    printf("Increased by: %ld bytes\n", (char *)after - (char *)before);
    return 0;
}