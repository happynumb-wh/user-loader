#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <stdint.h>
#include <errno.h>
#include <assert.h>
#include <string.h>

#include <mem.h>

int devmem_fd = -1;
void * user_mem;
#define PAGE_SIZE 4096

void * mmap_vm_to_phy(void *addr, size_t length, int prot)
{
    length = (length + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);

    uint64_t offset = (uint64_t)addr - (uint64_t)user_mem;
    void *ret = mmap(addr, length, prot, MAP_SHARED | MAP_FIXED, devmem_fd, MEMORY_BASE + offset);
    if (ret == MAP_FAILED)
    {
        printf("[%s] Error: Failed to mmap virtual memory to physical memory, code: %ld, errno: %d\n", __FUNCTION__, (long)ret, errno);
    }
    assert(ret == addr);
    return ret;
}


int memInit()
{

    if (-1 == (devmem_fd = open("/dev/mem", O_RDWR)))
	{
		printf("[%s] Error: Failed to open /dev/mem!\n", __FUNCTION__);
        exit(EXIT_FAILURE);
	}

    printf("[%s] /dev/mem opened\n", __FUNCTION__);

    assert(user_mem != NULL);
    void * ret = mmap(user_mem, MEMORY_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_FIXED, devmem_fd, MEMORY_BASE);
    memset(user_mem, 0, MEMORY_SIZE);
    // void * ret = mmap(user_mem, MEMORY_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (ret == MAP_FAILED)
    {
        printf("[%s] Error: Failed to mmap user memory, code: %ld, errno: %d\n", __FUNCTION__, (long)ret, errno);
        close(devmem_fd);
        exit(EXIT_FAILURE);
    }
    if (ret != (void *)user_mem)
    {
        printf("[%s] Error: User memory is not mapped at the expected address!\n", __FUNCTION__);
        close(devmem_fd);
        exit(EXIT_FAILURE);
    }

    printf("[%s] User memory ADDR: %p, Length: 0x%lx mapped\n", __FUNCTION__, user_mem, (uint64_t)MEMORY_SIZE);
    return 0;
}