#include "limine.h"
#include "franklin/mmu.h"
#include "franklin/defs.h"
#include "franklin/69.h"

static pte_t *getpte(uint64_t);


uintptr_t V2P(uintptr_t V) {
    return V - HHDM_OFFSET;
}

uintptr_t P2V(uintptr_t P) {
   return P + HHDM_OFFSET; 
}

uintptr_t getpaddr(uint64_t entry) {
    return (entry >> PAGE_SHIFT) << PAGE_SHIFT;
}



void init_vmm() {
    uint64_t addr;
    PML4E = P2V((uintptr_t)palloc(1));
    memzero((char*)PML4E, PGSIZE);
    test();    
}   

int mappage(uint64_t vaddr, uint64_t paddr, uint8_t flags) {
    uint64_t *PDPTE, *PDE, *PTE;
    Table tablearr[4] = {{PML4E, 39}, {PDPTE, 30}, {PDE, 21}, {PTE, 12}};
    uint16_t index;
    uintptr_t addr;
    for (int i = 0; i < 3; ++i) {
        index = (vaddr >> (tablearr[i].shift)) & 0x1FF;
        tablearr[i + 1].table = newentry(&tablearr[i].table[index], 0, flags);
    }
    index = (vaddr >> (tablearr[3].shift)) & 0x1FF;
    newentry(&tablearr[3].table[index], paddr, flags);


    return 1;
}

// get PTE and set present flag to 0 and free page from physical memory
void unmappage(uint64_t vaddr) {
    char *l = (char*) vaddr;
    asm("invlpg %0" : : "m" (*(char*)vaddr) : "memory");
    pte_t *pte = getpte(vaddr);
    uintptr_t paddr = getpaddr(*pte);
    *pte ^= (1 << PRESENT);
    freepg(paddr, 1);
}

// 1. if PFN is already in use, error 
// 2. get current PTE and replace the address part of its bits
void remappage(uint64_t vaddr, int pfn) {
    if (!isfree(pfn))
        panic("panic: remappage, pfn: is not free\n");
    pte_t *pte = getpte(vaddr);
    uintptr_t paddr = getpaddr(*pte);
    freepg(paddr, 1);
    *pte &= 0x1FF;
    *pte |= (uintptr_t)pallocaddr(1, pfn * PGSIZE);
}

void memzero(char* mem, int n) {
    for (int i = 0; i < n; ++i) {
        mem[i] = 0;
    }
}


static pte_t *getpte(uint64_t vaddr) {
    uintptr_t paddr;
    uint16_t index = vaddr >> 39;
    if ((PML4E[index] & PRESENT) == 0)
        panic("ERROR: getpte(), PML4E not in use\n");
    paddr = getpaddr(PML4E[index]);
    pdpte_t *PDPTE = (pdpte_t*)P2V(paddr);
    index = (vaddr >> 30) & 0x1FF;
    if ((PDPTE[index] & PRESENT) == 0)
        panic("ERROR: getpte(), PDPTE not in use\n");
    paddr = getpaddr(PDPTE[index]);
    pde_t *PDE = (pde_t*)P2V(paddr);
    index = (vaddr >> 21) & 0x1FF;
    if ((PDE[index] & PRESENT) == 0)
        panic("ERROR: getpte(), PDE not in use\n");
    paddr = getpaddr(PDE[index]);
    pte_t *PTE = (pte_t*)P2V(paddr);
    index = (vaddr >> 12) & 0x1FF;
    if ((PTE[index] & PRESENT) == 0)
        panic("ERROR: getpte(), PTE not in use\n");
    return &PTE[index];
}

// set new entry in the table_entry table
// returns the new entry
uint64_t* newentry(uint64_t *table_entry, uint64_t paddr, uint8_t flags) {
    if (*table_entry & PRESENT)
        goto noalloc;    
        
    uint64_t *page, vaddr;
    if (paddr != 0)
        page = pallocaddr(1, paddr);
    else
        page = palloc(1);
    *table_entry = (uintptr_t)page | flags;
    memzero((char*)P2V((uintptr_t)page), PGSIZE);

    noalloc:
    paddr = getpaddr(*table_entry);
    return (uint64_t*) P2V(paddr);
}



void test() {


    mappage(0, 0, KFLAGS);
    unmappage(0);

    for (int i = 0; i < 10; ++i)
        mappage(i * PGSIZE, 0, KFLAGS);

    for (int i = 10; i < 20; ++i)
        mappage(i * PGSIZE, 0, KFLAGS);

    for (int i = 10; i < 20; ++i)
        unmappage(i * PGSIZE);



    mappage(0, 0, KFLAGS);
    unmappage(0);
    mappage(10 * PGSIZE, 0, KFLAGS);
    unmappage(10 * PGSIZE);
    for (int i = 0; i < 10; ++i) {
        mappage(i * PGSIZE, 0, KFLAGS);
    }
    for (int i = 0; i < 10; ++i) {
        unmappage(i * PGSIZE);
    }

    mappage(0x3000, PGSIZE * PGSIZE, KFLAGS);
    mappage(3000 * PGSIZE, 1000 * PGSIZE, KFLAGS);
    mappage(5000* PGSIZE, 6000 * PGSIZE, KFLAGS);
    unmappage(5000 * PGSIZE);
    unmappage(3000 * PGSIZE);
    unmappage(0x3000);
    for (int i = 1000; i < (1000 + 10); ++i) {
        mappage(i * PGSIZE, 0, KFLAGS);
    }

    for (int i = 1000; i < (10 + 1000); ++i) {
        unmappage(i * PGSIZE);
    }

    for (int i = 0; i < 1000; ++i) {
        mappage(i * PGSIZE, 0, KFLAGS);
    }

    for (int i = 5000; i < 6000; ++i) {
        mappage(i * PGSIZE, 0, KFLAGS);
    }

    mappage(10000* PGSIZE, 0, KFLAGS);
    mappage(19990 * PGSIZE, 0, KFLAGS);

}
