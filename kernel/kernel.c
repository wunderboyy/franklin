#include <stdint.h>
#include <stddef.h>
#include "limine.h"
#include "franklin/defs.h"
#include "franklin/mmu.h"
#include "franklin/cpu.h"
#include "franklin/idt.h"
#include "franklin/69.h"
#include "franklin/spinlock.h"
#include "franklin/apic.h"
#include "franklin/kbd.h"
#include "franklin/pic.h"
#include "franklin/acpi.h"



static volatile struct limine_terminal_request terminal_request = {
    .id = LIMINE_TERMINAL_REQUEST,
    .revision = 0
};

static volatile struct limine_memmap_request memmap_request = {
    .id = LIMINE_MEMMAP_REQUEST,
    .revision = 0,
};


void print(void* s) {
    struct limine_terminal_response *terminal_res = terminal_request.response;
    struct limine_terminal *terminal = terminal_res->terminals[0];
    acquire(&spinlock);
    terminal_res->write(terminal, s, strlen(s));
    release(&spinlock);
}

extern void isr_kbd(void);
extern void isr_apic_timer(void);
extern void isr_timer(void);

void kmain(void) {
    init_idt();
    struct limine_memmap_response *memmap = memmap_request.response;
    initbmap(memmap);

    int i = 0;
    while (isfree(i++));

    init_lock(&spinlock);
    init_vmm();
    init_acpi(); // set global variable RSDT
    MADT *madt = get_acpi_sdt(MADT_C);
    walk_madt(madt); // get info about MADT table
    pic_remap(0x20); 
    new_irq(33, isr_kbd); 
    new_irq(34, isr_apic_timer);
    new_irq(32, isr_timer);
    unmask_irq(1);
    unmask_irq(0);
    init_kbd(); // init ps/2 keyboard
    init_pit(1000); // 1000 hz
    init_apic((unsigned int*)((unsigned long)madt->lapic + HHDM_OFFSET));
    init_cpu();

    for(;;)
        asm ("hlt");
}


