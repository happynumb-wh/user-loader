#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>


#include <pagetable.h>
#include <mem.h>


uint64_t pgdir[PAGE_SIZE / sizeof(PTE)];
void * va_pg_base;
int pg_full_size;
uint64_t pg_pa_base;
uint64_t pg_alloc_size;


static uint64_t allocPage()
{
    /**
     * allocate a physical page
     */
    uint64_t addr = pg_pa_base + pg_alloc_size;
    pg_alloc_size += PAGE_SIZE;
    if (pg_alloc_size > pg_full_size)
    {
        printf("[%s] Error: page table size exceed the limit!\n", __FUNCTION__);
        exit(EXIT_FAILURE);
    }
    return addr;
}

static uint64_t pg_pa2va(uint64_t pa)
{
    /**
     * convert pa to va
     */
    return (pa - pg_pa_base) + (uint64_t)va_pg_base;
}

static uint64_t pg_va2pa(uint64_t va)
{
    /**
     * convert va to pa
     */
    return (va - (uint64_t)va_pg_base) + pg_pa_base;
}

static inline uint64_t get_pa(PTE entry)
{
    /**
     * get pa from PTE
     */
    /* mask == 0031111111111111 */
    uint64_t mask = (uint64_t)(~0) >> _PAGE_PFN_SHIFT;
    /* ppn * 4096(0x1000) */
    return ((entry & mask) >> _PAGE_PFN_SHIFT) << 12;    
}

/* Get/Set page frame number of the `entry` */
static inline long get_pfn(PTE entry)
{
    /**
     * get pfn
     */
    /*mask == 00c1111111111111*/
    uint64_t mask = (uint64_t)(~0) >> _PAGE_PFN_SHIFT;
    return (entry & mask) >> _PAGE_PFN_SHIFT; 
}

static inline void set_pfn(PTE *entry, uint64_t pfn)
{
    /* set pfn
     */
    *entry &= ((uint64_t)(~0) >> 54);
    *entry |= (pfn << _PAGE_PFN_SHIFT);
}

/* Get/Set attribute(s) of the `entry` */
static inline long get_attribute(PTE entry)
{
    /**
     * get flag
     */

    uint64_t mask = (uint64_t)(~0) >> 54;
    return entry & mask;
}
static inline void set_attribute(PTE *entry, uint64_t bits)
{
    /**
     * set flag
     */
    *entry &= ((uint64_t)(~0) << 10);
    *entry |= bits;
}

static inline void clear_pgdir(uintptr_t pgdir_addr)
{
    /**
     * clear page
     */
    memset((void *)pgdir_addr, 0, PAGE_SIZE);
}

PTE check_page_set_flag(PTE* page, uint64_t vpn, uint64_t flag, int bzero){
    if (page[vpn] & _PAGE_PRESENT) 
    {
        /* valid */
        return (get_pfn(page[vpn]) << NORMAL_PAGE_SHIFT);
    } else
    {
        uint64_t newpage = pg_pa2va(allocPage() - PAGE_SIZE);
        if (bzero)
        {
            /* clear the second page */
            // clear_pgdir(newpage);   
            memset((void *)newpage, 0, PAGE_SIZE);         
        }
        set_pfn(&page[vpn], pg_va2pa(newpage) >> NORMAL_PAGE_SHIFT);
        /* maybe need to set the U, the kernel will not set the U 
         * which means that the User will not get it, but we will 
         * temporary set it as the User. so the U will be high
        */ 
        set_attribute(&page[vpn], flag);
        
        return newpage; 
    }

}

/* alooc the va to the pointer */
PTE * alloc_page_point_phyc(uintptr_t va, uintptr_t pgdir, uint64_t pa, uint64_t flag){
    uint64_t vpn[] = {
                      (va >> 12) & ~(~0 << 9), //vpn0
                      (va >> 21) & ~(~0 << 9), //vpn1
                      (va >> 30) & ~(~0 << 9)  //vpn2
                     };
    /* the PTE in the first page_table */
    PTE *page_base = (uint64_t *) pgdir;
    /* second page */
    PTE *second_page = NULL;
    /* finally page */
    PTE *third_page = NULL;
    /* find the second page */
    second_page = (PTE *)check_page_set_flag(page_base, vpn[2], _PAGE_PRESENT, 1);

    /* third page */
    third_page = (PTE *)check_page_set_flag(second_page, vpn[1], _PAGE_PRESENT, 1);
    /* final page */
    // if((third_page[vpn[0]] & _PAGE_PRESENT) != 0)
    //     /* return the physic page addr */
    //     return 0;//pa2kva((get_pfn(third_page[vpn[0]]) << NORMAL_PAGE_SHIFT));

    /* the physical page */
    third_page[vpn[0]] = 0;
    set_pfn(&third_page[vpn[0]], pa >> NORMAL_PAGE_SHIFT);
    /* maybe need to assign U to low */
    // Generate flags
    /* the R,W,X == 1 will be the leaf */
    // uint64_t pte_flags = _PAGE_PRESENT | _PAGE_READ  | _PAGE_WRITE    
    //                     | _PAGE_EXEC  | (mode == MAP_KERNEL ? (_PAGE_ACCESSED | _PAGE_DIRTY) :
    //                       _PAGE_USER);

    uint64_t pte_flags = _PAGE_PRESENT | flag | _PAGE_USER;

    set_attribute(&third_page[vpn[0]], pte_flags);
    return &third_page[vpn[0]];     
}


int page_table_build_range(void *va_page_base, \
                            int page_size, \
                            uint64_t pa_page_base, \
                            uint64_t va_start, \
                            uint64_t va_end, \
                            uint64_t pa_start, \
                            uint64_t pa_end, \
                            int prot)
{
    memset(pgdir, 0, PAGE_SIZE);
    va_pg_base = va_page_base;
    pg_full_size = page_size;
    pg_alloc_size = 0;
    pg_pa_base = pa_page_base;

    for (uint64_t va = va_start, pa = pa_start; va < va_end && pa < pa_end; va += PAGE_SIZE, pa += PAGE_SIZE)
    {
        PTE *pte = alloc_page_point_phyc(va, (uintptr_t)pgdir, pa, _PAGE_READ | _PAGE_WRITE);
        if (pte == NULL)
        {
            printf("[%s] Error: Failed to allocate page table entry for va: 0x%lx\n", __FUNCTION__, va);
            return -1;
        }
    }
    return 0;
}



