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

#ifdef X86
#include <x86intrin.h>
#endif

int devmem_fd = -1;
void * user_mem;
void * brk_start;



void flush_memory(void *ptr, size_t size) {
    uintptr_t start = (uintptr_t)ptr;
    uintptr_t end   = start + size;    

    if (size == 0)
        return;

#ifdef X86
    printf("[%s] Flushing memory range: %p - %p\n", __FUNCTION__, (void *)start, (void *)end);
    for (uintptr_t p = start; p < end; p += CACHE_LINE_SIZE) {
        _mm_clflush((void*)p);
    }

    _mm_mfence();
#endif




#ifdef RISCV

    /* 向下对齐 start，向上对齐 end 到 cache line 边界 */
    start &= ~(CACHE_LINE_SIZE - 1);
    end = (end + CACHE_LINE_SIZE - 1) & ~(CACHE_LINE_SIZE - 1);

    for (uintptr_t p = start; p < end; p += CACHE_LINE_SIZE) {
        /*
         * 直接使用助记符 cbo.flush，操作数用虚拟地址 p（放入寄存器）。
         * memory clobber 告诉编译器此操作对内存有副作用，避免被优化掉。
         */
        asm volatile ("cbo.flush (%0)" :: "r"(p) : "memory");
    }

    asm volatile("fence iorw, iorw" ::: "memory");

#endif
}




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

int clear_cache()
{
    uint64_t * begin = mmap(NULL, CACHE_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, devmem_fd, CACHE_BASE);
    if (begin == MAP_FAILED)
    {
        printf("[%s] Error: Failed to mmap cache memory, code: %ld, errno: %d\n", __FUNCTION__, (long)begin, errno);
        exit(EXIT_FAILURE);
    }

    volatile uint64_t used = 0;

    for (size_t i = 0; i < CACHE_SIZE / sizeof(uint64_t); i++)
    {
        used = begin[i];
    }
    printf("[%s] Clear Cache with size: 0x%lx\n", __FUNCTION__, CACHE_SIZE);
    munmap(begin, CACHE_SIZE);

    return 0;
}


int mem_page_build()
{
    assert(devmem_fd != -1);

    void * va_page_base = mmap(NULL, PAGE_TABLE_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, devmem_fd, PAGE_TABLE_BASE);

    if (va_page_base == MAP_FAILED)
    {
        printf("[%s] Error: Failed to mmap page table memory, code: %ld, errno: %d\n", __FUNCTION__, (long)va_page_base, errno);
        exit(EXIT_FAILURE);
    }

    int ret = page_table_build_range(va_page_base, \
                                        PAGE_TABLE_SIZE, \
                                        PAGE_TABLE_BASE, \
                                        (uint64_t)user_mem, \
                                        (uint64_t)user_mem + MEMORY_SIZE, \
                                        MEMORY_BASE, \
                                        MEMORY_BASE + MEMORY_SIZE, \
                                        0);
    if (ret != 0)
    {
        printf("[%s] Error: Failed to build page table!\n", __FUNCTION__);
        exit(EXIT_FAILURE);
    }

    munmap(va_page_base, PAGE_TABLE_SIZE);

    return 0;

}



int memInit()
{

#ifndef USER
    if (-1 == (devmem_fd = open("/dev/mem", O_RDWR)))
	{
		printf("[%s] Error: Failed to open /dev/mem!\n", __FUNCTION__);
        exit(EXIT_FAILURE);
	}

    printf("[%s] /dev/mem opened\n", __FUNCTION__);

    assert(user_mem != NULL);

    void * ret = mmap(user_mem, MEMORY_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_FIXED, devmem_fd, MEMORY_BASE);
#else
    void * ret = mmap(user_mem, MEMORY_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED, -1, 0);
#endif
    
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
#ifndef USER
    memset(user_mem, 0, MEMORY_SIZE);
#endif
    printf("[%s] User memory ADDR: %p, Length: 0x%lx mapped\n", __FUNCTION__, user_mem, (uint64_t)MEMORY_SIZE);
    return 0;
}