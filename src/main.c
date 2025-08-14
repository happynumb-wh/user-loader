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


__attribute__((naked, noreturn))
void jump_with_stack_rv64(void *entry, void *new_sp) {
#ifdef RISCV
    __asm__ volatile(
        "mv sp, a1\n\t"   // sp = new_sp
        "mv t0, a0\n\t"
        "li a0, 0\n\t"
        "jr t0\n\t"       // jump to entry
    );
#endif
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

    jump_with_stack_rv64((void *)entry_point, stack);

    munmap(user_mem, MEMORY_SIZE);
    return 0;
}