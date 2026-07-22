#pragma once
#include "vxair_types.h"

/**
 * @file vxair_idt.h
 * @brief Interrupt Descriptor Table (IDT) configuration
 */

// IDT and Interrupt handling
void vxair_idt_init(void);
void vxair_idt_set_gate(uint8_t vector, void* isr, uint8_t flags);
void vxair_idt_enable_interrupts(void);
void vxair_idt_disable_interrupts(void);
