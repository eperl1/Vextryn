global vxair_syscall_entry
extern vxair_syscall_handler

section .text
bits 64

vxair_syscall_entry:
    ; SYSCALL saves RIP into RCX and RFLAGS into R11
    ; TODO: Swapgs if necessary (kernel GS base)
    ; TODO: Save user RSP and load kernel RSP
    ; TODO: Save registers (Context saving)
    
    ; Call the C handler
    ; call vxair_syscall_handler
    
    ; TODO: Restore registers
    ; TODO: Restore user RSP
    ; TODO: Swapgs back
    
    ; Return to userspace
    sysretq
