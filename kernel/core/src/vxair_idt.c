#include "../include/vxair_idt.h"
#include "../include/vxair_log.h"

struct idt_entry {
    uint16_t isr_low;
    uint16_t kernel_cs;
    uint8_t  ist;
    uint8_t  attributes;
    uint16_t isr_mid;
    uint32_t isr_high;
    uint32_t reserved;
} __attribute__((packed));

struct idtr {
    uint16_t limit;
    uint64_t base;
} __attribute__((packed));

__attribute__((aligned(0x10)))
static struct idt_entry idt[256];
static struct idtr idtr;

struct interrupt_frame {
    uint64_t rax, rbx, rcx, rdx, rsi, rdi, rbp;
    uint64_t r8, r9, r10, r11, r12, r13, r14, r15;
    uint64_t vector;
    uint64_t error_code;
    uint64_t rip;
    uint64_t cs;
    uint64_t rflags;
    uint64_t rsp;
    uint64_t ss;
};

extern void* isr_stub_table[];

void vxair_fault_handler(struct interrupt_frame* frame) {
    vxair_log_error("Exception %d at RIP 0x%x! Error Code: 0x%x", 
                    (int)frame->vector, frame->rip, frame->error_code);
    if (frame->vector == 14) { // Page Fault
        uint64_t cr2;
        __asm__ volatile ("mov %%cr2, %0" : "=r"(cr2));
        vxair_log_error("Page Fault at CR2 0x%x", cr2);
    } else if (frame->vector == 13) {
        vxair_log_error("General Protection Fault. CS: 0x%x", frame->cs);
    }
    __asm__ volatile ("cli; hlt");
}

void vxair_irq_handler(struct interrupt_frame* frame) {
    (void)frame;
}

void vxair_idt_init(void) {
    vxair_log_info("IDT: Initializing...");
    
    idtr.base = (uint64_t)&idt[0];
    idtr.limit = (uint16_t)sizeof(struct idt_entry) * 256 - 1;

    for (int i = 0; i < 256; i++) {
        vxair_idt_set_gate(i, isr_stub_table[i], 0x8E);
    }
    
    __asm__ volatile ("lidt %0" : : "m"(idtr));
}

void vxair_idt_set_gate(uint8_t vector, void* isr, uint8_t flags) {
    uint64_t addr = (uint64_t)isr;
    
    idt[vector].isr_low = (uint16_t)(addr & 0xFFFF);
    idt[vector].kernel_cs = 0x08; 
    idt[vector].ist = 0;
    idt[vector].attributes = flags;
    idt[vector].isr_mid = (uint16_t)((addr >> 16) & 0xFFFF);
    idt[vector].isr_high = (uint32_t)((addr >> 32) & 0xFFFFFFFF);
    idt[vector].reserved = 0;
}

void vxair_idt_enable_interrupts(void) {
    __asm__ volatile ("sti");
}

void vxair_idt_disable_interrupts(void) {
    __asm__ volatile ("cli");
}
