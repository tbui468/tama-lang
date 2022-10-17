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
    push    ebp
    mov     ebp, esp

    mov     eax, [ebp + 8]
    add     eax, 0x30
    mov     [ebp + 8], eax

    mov     ecx, ebp
    add     ecx, 8
    mov     edx, 1

    mov     eax, 0x4    
    mov     ebx, 0x1
    int     0x80

    pop     ebp
    xor     eax, eax
    ret

_print_int:
    push    ebp
    mov     ebp, esp

    mov     eax, [ebp + 8] 
    mov     edi, 0 

    cmp     eax, 0
    je      print_zero

    test    eax, 0x80000000
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

    push    edx     
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

_print_bool:
    push    ebp
    mov     ebp, esp

    mov     eax, [ebp + 8] 
    cmp     eax, 0
    jg      print_true

    push    0x66
    call    _print_char
    add     esp, 4

    push    0x61
    call    _print_char
    add     esp, 4

    push    0x6c
    call    _print_char
    add     esp, 4

    push    0x73
    call    _print_char
    add     esp, 4

    push    0x65
    call    _print_char
    add     esp, 4

    jmp     print_bool_end

print_true:
    push    0x74
    call    _print_char
    add     esp, 4

    push    0x72
    call    _print_char
    add     esp, 4

    push    0x75
    call    _print_char
    add     esp, 4

    push    0x65
    call    _print_char
    add     esp, 4

print_bool_end:

    pop     ebp
    xor     eax, eax
    ret

