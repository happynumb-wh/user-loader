#include <stdio.h>
#include <getopt.h>
#include <elf.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>

#include <mem.h>
#include <loader.h>

Elf64_Phdr phdr[SEG_NUM];
Elf64_auxv_t auxv[AT_NUM];
int auxv_count = 0;
int envp_count = 0;

static uint64_t seg_flag_to_prot(uint64_t flags)
{
    uint64_t prot = 0;
    if (flags & PF_R) prot |= PROT_READ;
    if (flags & PF_W) prot |= PROT_WRITE;
    if (flags & PF_X) prot |= PROT_EXEC;
    return prot;
}


uint64_t load_elf_memory(char * file, Elf64_auxv_t * auxv)
{
    Elf64_Ehdr ehdr;

    int phnum;

    FILE *target = NULL;
    target = fopen(file, "r");
    uint64_t entry_point;

    if (target == NULL)
    {
        printf("[%s] Error: failed to open ELF file %s\n", __FUNCTION__, file);
        exit(EXIT_FAILURE);
    }

    assert(fread(&ehdr, sizeof(Elf64_Ehdr), 1, target) == 1);
    assert(memcmp((char *)ehdr.e_ident, ELFMAG, 3) == 0);
    assert(ehdr.e_type == ET_EXEC);

    entry_point = ehdr.e_entry;
    printf("[%s] Info: ELF file %s loaded, entry point at 0x%lx\n", __FUNCTION__, file, entry_point);

    fseek(target, ehdr.e_phoff, SEEK_SET);

    phnum = ehdr.e_phnum;
    assert(phnum <= SEG_NUM);

    assert(fread(phdr, sizeof(Elf64_Phdr), ehdr.e_phnum, target) == ehdr.e_phnum);


    for(int i = 0; i < phnum; i++)
    {
        if (phdr[i].p_type == PT_LOAD)
        {
            user_mem = (void *)phdr[i].p_vaddr;
            break;
        }
    }
    // Change axuv
    for(int i = 0; auxv[i].a_type != 0; i++)
    {
        if (auxv[i].a_type == AT_PHDR)
        {
            auxv[i].a_un.a_val = (uint64_t)user_mem + ehdr.e_phoff;
        }

        if (auxv[i].a_type == AT_PHNUM)
        {
            auxv[i].a_un.a_val = phnum;
        }

        if (auxv[i].a_type == AT_PHENT)
        {
            auxv[i].a_un.a_val = ehdr.e_phentsize;
        }

        if (auxv[i].a_type == AT_ENTRY)
        {
            auxv[i].a_un.a_val = entry_point;
        }
    }

    // Initialize memory
    memInit();  

    // Mem have ok
    for (int i = 0; i < phnum; i++)
    {
        if (phdr[i].p_type == PT_LOAD)
        {
            printf("[%s] Segment<%d> loaded at address %p, length 0x%lx\n", __FUNCTION__, i, (void *)phdr[i].p_vaddr, phdr[i].p_memsz);
            fseek(target, phdr[i].p_offset, SEEK_SET);
            assert(fread((void *)phdr[i].p_vaddr, 1, phdr[i].p_filesz, target) == phdr[i].p_filesz);
            if (phdr[i].p_memsz > phdr[i].p_filesz)
            {
                memset((void *)(phdr[i].p_vaddr + phdr[i].p_filesz), 0, phdr[i].p_memsz - phdr[i].p_filesz);
            }
            mprotect((void *)(phdr[i].p_vaddr & ~0xFFF), phdr[i].p_memsz, seg_flag_to_prot(phdr[i].p_flags));
        }
    }


    fclose(target);

    printf("[%s] Info: user loader loaded ELF file %s\n", __FUNCTION__, file);

    return entry_point;
}

void * prepare_stack(int argc, char * argv[], char * envp[], Elf64_auxv_t * auxv)
{
    // sp is the stack pointer
    uint64_t sp = (uint64_t)user_mem + MEMORY_SIZE;
    char * new_argv[argc + 1];
    char * new_envp[envp_count + 1];
    new_argv[argc] = NULL;
    new_envp[envp_count] = NULL;
    // Push command line arguments into the stack
    for (int i = argc - 1; i >= 0; i--)
    {
        sp -= (strlen(argv[i]) + 1);
        new_argv[i] = (char *)sp;
        strcpy((char *)sp, argv[i]);
    }

    // Push environment variables into the stack
    for (int i = envp_count - 1; i >= 0; i--)
    {
        sp -= (strlen(envp[i]) + 1);
        new_envp[i] = (char *)sp;
        strcpy((char *)sp, envp[i]);
    }

    sp = sp & ~0xF;
    // Push auxiliary vector into the stack
    sp -= sizeof(Elf64_auxv_t) * (auxv_count + 1);
    memcpy((void *)sp, auxv, sizeof(Elf64_auxv_t) * (auxv_count + 1));

    // Push envp into the stack
    sp -= (envp_count + 1) * sizeof(char *);
    memcpy((void *)sp, new_envp, sizeof(char *) * (envp_count + 1));

    // Push argv into the stack
    sp -= (argc + 1) * sizeof(char *);
    memcpy((void *)sp, new_argv, sizeof(char *) * (argc + 1));
    sp -= sizeof(char *);

    // Push argc into the stack
    *(uint64_t *)sp = argc;

    printf("[%s] Info: stack prepared at %p\n", __FUNCTION__, (void *)sp);
    return (void *)sp;
}

