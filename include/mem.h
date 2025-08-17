#pragma once

// leave 256MB to kernel
#define MEMORY_BASE  0x100000000
#define USER_BASE    0x10000
#define MEMORY_SIZE  0x100000000


void * mmap_vm_to_phy(void *addr, size_t length, int prot);
int memInit();