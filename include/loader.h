#pragma once
#include <elf.h>


#define AT_NUM 60
#define SEG_NUM 16


extern void * user_mem;
extern Elf64_auxv_t auxv[AT_NUM];
extern int auxv_count;
extern int envp_count;

uint64_t load_elf_memory(char * file, Elf64_auxv_t * auxv);
void * prepare_stack(int argc, char * argv[], char * envp[], Elf64_auxv_t * auxv);