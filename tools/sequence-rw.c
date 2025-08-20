#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <stdint.h>
#include <errno.h>
#include <assert.h>
#include <string.h>

#define MEMORY_BASE 0x200000000
#define MEMORY_SIZE 0x200000000



int main() {

    int dev_fd = open("/dev/mem", O_RDWR | O_SYNC);
    if (dev_fd < 0) {
        printf("[%s] Failed to open /dev/mem", __FUNCTION__);
        exit(EXIT_FAILURE);
    }

    uint64_t *mapped_mem = mmap(NULL, MEMORY_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, dev_fd, MEMORY_BASE);
    if (mapped_mem == MAP_FAILED) {
        printf("[%s] Failed to mmap /dev/mem", __FUNCTION__);
        close(dev_fd);
        exit(EXIT_FAILURE);
    }
    volatile register uint64_t used = 0;
    // Every cache line
    for (size_t i = 0; i < MEMORY_SIZE / 64; i++) {
        used = mapped_mem[i*8];
    }

    munmap(mapped_mem, MEMORY_SIZE);
    close(dev_fd);
    return 0;
}