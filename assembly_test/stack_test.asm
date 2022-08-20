section     .text
global      _start

%include "fun.asm"

_start:
    mov     ebp, esp
    push    3
    push    8
    push    9
    mov     eax, [ebp - 4]

    push    eax
    call    _print_int
    add     esp, 4

    push    0xa
    call    _print_char
    add     esp, 4

    ;exit syscall with argument 0
    mov     eax, 0x1
    xor     ebx, ebx
    int     0x80
