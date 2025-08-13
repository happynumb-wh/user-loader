#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <mem.h>
#include <stdint.h>
#include <errno.h>
#include <assert.h>


int devmem_fd = -1;
void * user_mem;



int memInit()
{

    if (-1 == (devmem_fd = open("/dev/mem", O_RDWR | O_SYNC)))
	{
		printf("[%s] Error: Failed to open /dev/mem!\n", __FUNCTION__);
        exit(EXIT_FAILURE);
	}

    printf("[%s] /dev/mem opened successfully!\n", __FUNCTION__);

    assert(user_mem != NULL);
    void * ret = mmap(user_mem, MEMORY_SIZE, PROT_READ | PROT_WRITE | MAP_FIXED, MAP_SHARED, devmem_fd, MEMORY_BASE);
    if (ret == MAP_FAILED)
    {
        printf("[%s] Error: Failed to mmap user memory, code: %ld, errno: %d\n", __FUNCTION__, (long)ret, errno);
        close(devmem_fd);
        exit(EXIT_FAILURE);
    }
    if (ret != (void *)user_mem)
    {
        printf("[%s] Error: User memory is not mapped at the expected address!\n", __FUNCTION__);
        munmap(user_mem, MEMORY_SIZE);
        close(devmem_fd);
        exit(EXIT_FAILURE);
    }


    printf("[%s] User memory ADDR: %p, Length: 0x%lx mapped successfully!\n", __FUNCTION__, user_mem, (uint64_t)MEMORY_SIZE);
    return 0;
}