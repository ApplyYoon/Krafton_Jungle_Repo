#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

int main() {
    void *brk = sbrk(0);
    printf("Current program break: %p\n", brk);

    FILE *fp = fopen("/proc/self/maps", "r");
    if (!fp) {
        perror("fopen");
        return 1;
    }

    char line[256];
    while (fgets(line, sizeof(line), fp)) {
        if (strstr(line, "[heap]")) {
            printf("Heap mapping: %s", line);
            break;
        }
    }
    fclose(fp);

    sbrk(1);
    
    brk = sbrk(0);
    printf("Current program break: %p\n", brk);
    return 0;
}