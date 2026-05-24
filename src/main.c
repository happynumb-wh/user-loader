#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include <string.h>
#include <unistd.h>
#include <elf.h>
#include <sys/mman.h>
#include <time.h>

#include <loader.h>
#include <mem.h>

#ifdef __ASSEMBLER__
#define __ASM_STR(x)	x
#else
#define __ASM_STR(x)	#x
#endif

static double timespec_diff_seconds(const struct timespec *end,
					     const struct timespec *start)
{
	time_t sec = end->tv_sec - start->tv_sec;
	long nsec = end->tv_nsec - start->tv_nsec;
	if (nsec < 0) {
		sec -= 1;
		nsec += 1000000000L;
	}
	return (double)sec + (double)nsec / 1000000000.0;
}

static void format_timestamp(const struct timespec *ts, char *buffer, size_t size)
{
	struct tm tm_info;
	localtime_r(&ts->tv_sec, &tm_info);
	strftime(buffer, size, "%Y-%m-%d %H:%M:%S", &tm_info);
}


#ifdef RISCV
#define HMTTCFG 0x400
#define HMTTINSTRS 0x401
#define HMTTLOADINSTRS 0x402
#define HMTTSTOREINSTRS 0x403


#define csr_read(csr)                                           \
	({                                                      \
		register unsigned long __v;                     \
		__asm__ __volatile__("csrr %0, " __ASM_STR(csr) \
				     : "=r"(__v)                \
				     :                          \
				     : "memory");               \
		__v;                                            \
	})

#define csr_write(csr, val)                                        \
	({                                                         \
		unsigned long __v = (unsigned long)(val);          \
		__asm__ __volatile__("csrw " __ASM_STR(csr) ", %0" \
				     :                             \
				     : "rK"(__v)                   \
				     : "memory");                  \
	})

#endif


void print_counters()
{
#ifdef RISCV
    csr_write(HMTTCFG, 0);
    uint64_t instrs = csr_read(HMTTINSTRS);
    uint64_t load_instrs = csr_read(HMTTLOADINSTRS);
    uint64_t store_instrs = csr_read(HMTTSTOREINSTRS);
    printf("Total Load Instructions: %lu\n", load_instrs);
    printf("Total Store Instructions: %lu\n", store_instrs);
#endif
}

__attribute__((noinline, noreturn))
void jump_with_stack(void *entry, void *new_sp) {
#ifdef X86
    __asm__ volatile(
        "mov %0, %%rbx\n\t"
        "mov %1, %%rsp\n\t"
        "mov $0, %%rax\n\t"
        "mov $0, %%rdx\n\t"
        "jmp *%%rbx\n\t"
        :
        : "r"(entry), "r"(new_sp)
        : "rax"
    );
#endif

#ifdef RISCV

    // uint64_t counter_func = (uint64_t)&print_counters;
    __asm__ volatile(
        "mv sp, %1\n\t"
        "mv t0, %0\n\t"
        "csrwi 0x400, 7\n\t" // enable mstatus.HMTTCFG
        "csrwi 0x401, 0\n\t" // reset HMTTINSTRS
        "csrwi 0x402, 0\n\t" // reset HMTTLOADINSTRS
        "csrwi 0x403, 0\n\t" // reset HMTTSTOREINSTRS
        "fence.i\n\t"
        "fence\n\t"
        "li a0, 0\n\t"
        "jr t0\n\t"
        :
        : "r"(entry), "r"(new_sp)
        : "memory"
    );
    // __asm__ volatile(
    //     "mv sp, a1\n\t"   // sp = new_sp
    //     "mv t0, a0\n\t"
    //     "li a0, 0\n\t"
    //     "csrwi 0x400, 7\n\t" // enable mstatus.HMTTCFG
    //     "jr t0\n\t"       // jump to entry
    // );
#endif
    __builtin_unreachable();
}

void get_continue()
{
    char input;
    printf("Continue? (y/n): ");
    int ret =  scanf(" %c", &input);
    if (input == 'y' || input == 'Y') {
        // Continue execution
    } else {
        // Stop execution
    }
    // printf("Sleep 10 seconds\n");
    // sleep(10);
    
}

int main(int argc, char *argv[], char *envp[])
{
    struct timespec loader_start_ts = {0};
    struct timespec ready_ts = {0};
    char loader_start_buf[32];
    char ready_buf[32];
    int have_loader_start = 0;

    void * base;
    void * stack;
    uint64_t entry_point;
    Elf64_auxv_t * init_auxv;
    char * target_elf = NULL;
    int used_exist_stack_file = 0;

    if (clock_gettime(CLOCK_REALTIME, &loader_start_ts) == 0) {
        have_loader_start = 1;
        format_timestamp(&loader_start_ts, loader_start_buf,
                 sizeof(loader_start_buf));
        printf("[%s] start time: %s.%09ld\n",
               __FUNCTION__, loader_start_buf, loader_start_ts.tv_nsec);
    } else {
        perror("clock_gettime (start)");
    }

    for (envp_count = 0; envp[envp_count] != NULL; envp_count++);

    init_auxv = (Elf64_auxv_t *)(&envp[envp_count + 1]);

#ifndef USER
    if (0 != getuid())
    {
        printf("[%s] Error: user loader must run with root priviledge!\n", \
            __FUNCTION__);
        exit(EXIT_FAILURE);
    }
#endif

    for (; init_auxv[auxv_count].a_type != 0; auxv_count++)
    {
        auxv[auxv_count] = init_auxv[auxv_count];
    }

    auxv[auxv_count].a_type = 0;
    auxv[auxv_count].a_un.a_val = 0;
    assert(auxv_count < AT_NUM);
    assert(argc > 1);
    if (!strcmp("-stack", argv[1])) used_exist_stack_file = 1;
    if (used_exist_stack_file) target_elf = argv[3];
    else target_elf = argv[1];

    entry_point = load_elf_memory(target_elf, auxv);

    if (used_exist_stack_file)
    {
        stack = prepare_stack_with_file(argv[2]);
    } else 
    {
        stack = prepare_stack(argc - 1, &(argv[1]), envp, auxv);

        FILE * stack_file = fopen("stack", "w");

        fwrite(stack, 1, (uint64_t)user_mem + MEMORY_SIZE - (uint64_t)stack, stack_file);
        fclose(stack_file);        
    }
#ifndef USER
    flush_memory(user_mem, MEMORY_SIZE);
#ifdef RISCV
    clear_cache();
#endif
    get_continue();

#ifdef RISCV
    // clear all cache
    __asm__ volatile("fence.i");
    __asm__ volatile("fence");
#endif
#endif

    if (clock_gettime(CLOCK_REALTIME, &ready_ts) == 0) {
        format_timestamp(&ready_ts, ready_buf, sizeof(ready_buf));
        printf("[%s] pre-jump time: %s.%09ld\n",
               __FUNCTION__, ready_buf, ready_ts.tv_nsec);
        if (have_loader_start) {
            double prep_seconds =
                timespec_diff_seconds(&ready_ts, &loader_start_ts);
            printf("[%s] preparation duration: %.6f s\n",
                   __FUNCTION__, prep_seconds);
        }
    } else {
        perror("clock_gettime (pre-jump)");
    }

    jump_with_stack((void *)entry_point, stack);

    munmap(user_mem, MEMORY_SIZE);
    return 0;
}
