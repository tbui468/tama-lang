section .text
global  _start

;3 * 2 - 4

_start:
    ;mov ebx, 0x1    ;file descriptor (stdout)
    ;mov ecx, hello
    ;mov edx, helloLen
    ;mov eax, 0x4    ;syscall number (write)
    ;system_call

    ;load single digit constant into register, add 0x30 to make into a character, and push onto stack
    mov     ecx, 9
    add     ecx, 0x30
    push    ecx

    ;write requires an address, so put top of stack address (the digit) into ecx, and set length to write (1 byte)
    mov     ecx, esp    ;use char on stack
    mov     edx, 1      ;length of char

    ;set syscall, set file descriptor to stdout, and call write
    mov     eax, 0x4
    mov     ebx, 0x1
    int     0x80

    ;pop the stack to remove the pushed digit
    pop     ecx

    ;print newline
    push    0xa
    mov     ecx, esp
    mov     edx, 1
    mov     eax, 0x4
    mov     ebx, 0x1
    int     0x80
    pop     ecx
   
    ;exit 
    xor     ebx, ebx
    mov     eax, 0x1    ;exit system call
    int     0x80

section .data
    hello db "Hello World", 0xa
    helloLen    equ $-hello
