#pragma once

// leave 256MB to kernel
#define MEMORY_BASE  0x100000000
#define USER_BASE    0x10000
#define MEMORY_SIZE  0x100000000
#define CACHE_BASE   (MEMORY_BASE + MEMORY_SIZE)
#define CACHE_SIZE   0x100000000

void flush_memory(void *ptr, size_t size);
void * mmap_vm_to_phy(void *addr, size_t length, int prot);
int clear_cache();
int memInit();