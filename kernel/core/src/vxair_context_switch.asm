global vxair_context_switch

section .text
bits 64

; void vxair_context_switch(vxair_thread_t* prev, vxair_thread_t* next)
; RDI = prev, RSI = next
vxair_context_switch:
    ; TODO: Push general purpose registers to current stack
    ; TODO: Save current stack pointer (RSP) to prev->stack_ptr
    ; TODO: Load next->stack_ptr to RSP
    ; TODO: Pop general purpose registers from new stack
    ret
