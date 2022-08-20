
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

    ;if zero
    cmp     eax, 0
    je      print_zero

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

print_zero:
    push    eax
    inc     edi

print_all: 
    call    _print_digit
    add     esp, 4
    dec     edi 
    cmp     edi, 0
    jg      print_all 

    pop     ebp
    xor     eax, eax
    ret
