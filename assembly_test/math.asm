;6 * 2 - 14 / 3 - 10
;answer: 18

section     .text
global      _start

_print_char:
    push    ebp
    mov     ebp, esp

    mov     ecx, ebp
    add     ecx, 8
    mov     edx, 1

    mov     eax, 0x4
    mov     ebx, 0x1
    int     0x80

    pop     ebp
    xor     eax, eax
    ret

_print_digit:
    ;create new call frame
    push    ebp
    mov     ebp, esp

    ;convert byte into char
    mov     eax, [ebp + 8]
    add     eax, 0x30
    mov     [ebp + 8], eax

    ;set arguments for write syscall 
    mov     ecx, ebp
    add     ecx, 8  ;need to go down stack 2 bytes (top is old base pointer address, and second to top is return address)
    mov     edx, 1

    ;set syscall to write, file descriptor to stdout, and trigger interrupt
    mov     eax, 0x4    
    mov     ebx, 0x1
    int     0x80

    ;restore previous call frame, and return 0
    pop     ebp
    xor     eax, eax
    ret

_print_int:
    push    ebp
    mov     ebp, esp

    mov     eax, [ebp + 8] 
    mov     edi, 0      ;counter to keep track of how many digits we pushed so that they can be popped/printed in order

    test    eax, 0x80000000 ;hex for 2^31
    jnz     do_negative
    jmp     negative_done
do_negative:

    push    0x2d
    call    _print_char
    add     esp, 4

    mov     eax, [ebp + 8]
    neg     eax

negative_done: 

    push_digit:
    cmp     eax, 0
    je      print_all
  
    mov     edx, 0 
    mov     ecx, 10
    div     ecx
    ;divide number by 10 and store quotient in edx
    push    edx         ;push remainder for later printing
    inc     edi
    jmp     push_digit

print_all: 
    call    _print_digit
    add     esp, 4
    dec     edi 
    cmp     edi, 0
    jg      print_all 

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
    push    10
    pop     ebx
    pop     eax
    sub     eax, ebx
    ;push    eax

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
