#include <stdio.h>
#include <getopt.h>
#include <elf.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>

#include <loader.h>
#include <mem.h>
Elf64_Phdr phdr[SEG_NUM];

static uint64_t seg_flag_to_prot(uint64_t flags)
{
    uint64_t prot = 0;
    if (flags & PF_R) prot |= PROT_READ;
    if (flags & PF_W) prot |= PROT_WRITE;
    if (flags & PF_X) prot |= PROT_EXEC;
    return prot;
}


int main(int argc, char *argv[])
{
    FILE *target = NULL;
    int phnum;
    Elf64_Ehdr *ehdr;
    void * base;

    if (0 != getuid())
    {
        printf("[%s] Error: user loader must run with root priviledge!\n", \
            __FUNCTION__);
        exit(EXIT_FAILURE);
    }

    

    if (argc > 1) {
        target = fopen(argv[1], "r");
        assert(target != NULL);
    }

    ehdr = malloc(sizeof(Elf64_Ehdr));
    assert(ehdr != NULL);
    assert(fread(ehdr, sizeof(Elf64_Ehdr), 1, target) == 1);

    assert(memcmp((char *)ehdr->e_ident, ELFMAG, 3) == 0);

    fseek(target, ehdr->e_phoff, SEEK_SET);

    phnum = ehdr->e_phnum;
    assert(phnum <= SEG_NUM);

    fread(phdr, sizeof(Elf64_Phdr), ehdr->e_phnum, target);
    

    for(int i = 0; i < phnum; i++)
    {
        // Map segment into memory
        if (phdr[i].p_type == PT_LOAD)
        {
            user_mem = (void *)phdr[i].p_vaddr;
            break;
        }
    }
    // Initialize memory
    memInit();  

    // Mem have ok
    for (int i = 0; i < phnum; i++)
    {
        if (phdr[i].p_type == PT_LOAD)
        {
            printf("[%s] Segment<%d> loaded at address %p, length %ld\n", __FUNCTION__, i, (void *)phdr[i].p_vaddr, phdr[i].p_memsz);
            fseek(target, phdr[i].p_offset, SEEK_SET);
            fread((void *)phdr[i].p_vaddr, 1, phdr[i].p_filesz, target);
            if (phdr[i].p_memsz > phdr[i].p_filesz)
            {
                memset((void *)(phdr[i].p_vaddr + phdr[i].p_filesz), 0, phdr[i].p_memsz - phdr[i].p_filesz);
            }

        }

        // Change protection
        mprotect((void *)phdr[i].p_vaddr, phdr[i].p_memsz, seg_flag_to_prot(phdr[i].p_flags));
    }


    fclose(target);
    free(ehdr);
    munmap(user_mem, MEMORY_SIZE);

    printf("[%s] Info: user loader loaded ELF file successfully!\n", __FUNCTION__);
    
    return 0;
}