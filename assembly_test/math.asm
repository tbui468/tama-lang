;6 * 2 - 14 / 3 + 2
;answer: 10

section     .text
global      _start

print_int:
    ;make new call frame
    push    ebp         ;save old call frame
    mov     ebp, esp    ;intialize new call frame

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

    ;restore old call frame
    mov     esp, ebp
    pop     ebp

    xor     eax, eax
    ret

_start:
    ;******************compiler should start writing this***********
    ;put constants into stack
    push    6
    push    2
    
    ;pop operands and mutiply (with eax, and result stored there), then push result
    pop     eax
    pop     ecx
    mul     ecx ;TODO: check if carry flag is set (if so, edx contains significant bits)
    push    eax

    ;pop operand and divide (edx and eax are dividend, and operand for div is divisor)
    ;quotient goes in eax and remainder goes in edx
    push    14
    push    3

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
    push    1
    pop     eax
    pop     ebx
    add     eax, ebx
    push    eax

    ;***************Compiler should end writing here***************
    ;Caller will clean the stack of all the pushed arguments
    ;pushed eax in line above for the print_int argument
    call    print_int
    add     esp, 4
    ;return value is in eax if we want to use it

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
