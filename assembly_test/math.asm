;((6 * 2 - 14 / 3 - 10) * 2) / -4
;answer: -4
section     .text
global      _start

%include "fun.asm"

_start:
    ;******************compiler should start writing this***********
    ;put constants into stack
    push    6
    push    2
    
    ;pop operands and mutiply (with eax, and result stored there), then push result
    pop     eax
    pop     ecx
    imul    ecx ;TODO: check if carry flag is set (if so, edx contains significant bits)
    push    eax

    ;pop operand and divide (edx and eax are dividend, and operand for div is divisor)
    ;quotient goes in eax and remainder goes in edx
    push    14
    push    3

    pop     ecx
    pop     eax
    cdq     ;sign extend to 64-bits for edx:eax (convert double-word to quad-word)
    idiv    ecx
    push    eax

    ;push constant, subtract, and push result
    pop     ebx
    pop     eax
    sub     eax, ebx
    push    eax

    ;push constant, add and push result
    push    10
    pop     ebx
    pop     eax
    sub     eax, ebx
    push    eax

    ;signed multiply
    push    2
    pop     ebx
    pop     eax
    imul    eax, ebx
    push    eax

    ;signed divide
    push    -4
    pop     eax
    cdq     ;sign extend to 64-bit edx:eax (convert double-word to quad-word)
    pop     ebx
    idiv    ebx

    ;***************Compiler should end writing here***************
    push    eax         ;holds the number we want to print (could be multiple digits long)
    call    _print_int
    add     esp, 4

    push    0xa
    call    _print_char
    add     esp, 4

    push    -100
    call    _print_int
    add     esp, 4

    push    0xa
    call    _print_char
    add     esp, 4

    push    131
    call    _print_int
    add     esp, 4

    push    0xa
    call    _print_char
    add     esp, 4

    ;exit syscall with argument 0
    mov     eax, 0x1
    xor     ebx, ebx
    int     0x80

section     .data
