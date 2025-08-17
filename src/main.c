#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include <string.h>
#include <unistd.h>
#include <elf.h>
#include <sys/mman.h>

#include <loader.h>
#include <mem.h>


__attribute__((noinline, noreturn))
void jump_with_stack(void *entry, void *new_sp) {
    __asm__ volatile(
        "mov %0, %%rax\n\t"
        "mov %1, %%rsp\n\t"
        "jmp *%%rax\n\t"
        :
        : "r"(entry), "r"(new_sp)
        : "rax"
    );

#ifdef RISCV
    __asm__ volatile(
        "mv sp, a1\n\t"   // sp = new_sp
        "mv t0, a0\n\t"
        "li a0, 0\n\t"
        "jr t0\n\t"       // jump to entry
    );
#endif
    __builtin_unreachable();
}

int main(int argc, char *argv[], char *envp[])
{
    
    void * base;
    void * stack;
    uint64_t entry_point;
    Elf64_auxv_t * init_auxv;

    for (envp_count = 0; envp[envp_count] != NULL; envp_count++);

    init_auxv = (Elf64_auxv_t *)(&envp[envp_count + 1]);

    if (0 != getuid())
    {
        printf("[%s] Error: user loader must run with root priviledge!\n", \
            __FUNCTION__);
        exit(EXIT_FAILURE);
    }

    for (; init_auxv[auxv_count].a_type != 0; auxv_count++)
    {
        auxv[auxv_count] = init_auxv[auxv_count];
    }

    auxv[auxv_count].a_type = 0;
    auxv[auxv_count].a_un.a_val = 0;
    assert(auxv_count < AT_NUM);
    assert(argc > 1);

    entry_point = load_elf_memory(argv[1], auxv);

    stack = prepare_stack(argc - 1, &(argv[1]), envp, auxv);

    jump_with_stack((void *)entry_point, stack);

    munmap(user_mem, MEMORY_SIZE);
    return 0;
}