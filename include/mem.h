#pragma once

// leave 256MB to kernel
#define MEMORY_BASE  0xc0000000
#define USER_BASE    0x10000
#define MEMORY_SIZE  0x78000000

int memInit();