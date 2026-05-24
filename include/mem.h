#pragma once

#include <pagetable.h>
#include <stddef.h>

#define KB 0x400lu
#define MB 0x100000lu
#define GB 0x40000000lu



// base define
// #define MEMORY_BASE  (4 * GB)
#define MEMORY_BASE  (4 * GB)
#define USER_BASE    0x10000
#define MEMORY_SIZE  (4 * GB)
#define REAMAIN_SIZE (12 * GB)
#define CACHE_BASE   (MEMORY_BASE + MEMORY_SIZE + PAGE_TABLE_SIZE)
#define CACHE_SIZE   ((10 * GB) - PAGE_TABLE_SIZE)


// page table 
#define PAGE_TABLE_BASE (MEMORY_BASE + MEMORY_SIZE)
#define PAGE_TABLE_SIZE  (MB * 16)

#define PAGE_SIZE 4096
#define CACHE_LINE_SIZE 64

#ifndef CACHE_LINE_SIZE
#define CACHE_LINE_SIZE 64
#endif


// base function 
void flush_memory(void *ptr, size_t size);
void * mmap_vm_to_phy(void *addr, size_t length, int prot);
int clear_cache();
int memInit();

// pagetable function
int mem_page_build();