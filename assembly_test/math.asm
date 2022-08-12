;6 * 2 - 14 / 3 + 1
;answer: 9

section     .text
global      _start

_start:
    ;put constants into stack
    mov     eax, 6
    push    eax
    mov     eax, 2
    push    eax
    
    ;pop operands and mutiply (with eax, and result stored there), then push result
    pop     eax
    pop     ecx
    mul     ecx ;TODO: check if carry flag is set (if so, edx contains significant bits)
    push    eax

    ;pop operand and divide (edx and eax are dividend, and operand for div is divisor)
    ;quotient goes in eax and remainder goes in edx
    mov     eax, 14
    push    eax
    mov     eax, 3
    push    eax

    pop     ecx
    mov     edx, 0  ;clear upper bytes of dividend
    pop     eax
    div     ecx
    push    eax

    ;push constant, subtract, and push result
    pop     ebx
    pop     eax
    sub     eax, ebx
    push    eax

    ;push constant, add and push result
    mov     eax, 1
    push    eax
    pop     eax
    pop     ebx
    add     eax, ebx
    push    eax

    ;convert byte into char
    pop     eax
    add     eax, 0x30
    push    eax

    ;set arguments for write syscall 
    mov     ecx, esp
    mov     edx, 1

    ;set syscall to write, file descriptor to stdout, and trigger interrupt
    mov     eax, 0x4    
    mov     ebx, 0x1
    int     0x80

    ;remove sum (as a char) from stack
    pop     eax

    ;print newline
    mov     eax, 0xa
    push    eax
    mov     ecx, esp
    mov     edx, 1
    mov     eax, 0x4
    mov     ebx, 0x1
    int     0x80
    pop     eax

    ;exit syscall with argument 0
    mov     eax, 0x1
    xor     ebx, ebx
    int     0x80

section     .data
