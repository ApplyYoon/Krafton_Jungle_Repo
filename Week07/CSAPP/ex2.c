#include <unistd.h>
#include <stdint.h>

int main(void) {

    // Returns: old brk pointer on success, -1 on error
    void *sbrk(intptr_t incr);

    return 0;
}