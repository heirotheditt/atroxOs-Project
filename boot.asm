bits 32
section .text
    align 4
    dd 0x1BADB002            ; Multiboot Magic
    dd 0x00                  ; Flags
    dd - (0x1BADB002 + 0x00) ; Checksum

global start
extern kernel_main

start:
    cli                      ; Disable interrupts
    mov esp, stack_top       ; Set up our C stack
    call kernel_main         ; Jump into AtroxOS
.halt:
    hlt
    jmp .halt

section .bss
resb 8192                    ; 8KB Stack
stack_top:

