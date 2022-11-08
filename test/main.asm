__main:
    push    ebp
    mov     ebp, esp
    push    10
    push    10
    pop     eax
    neg     eax
    push    eax
    call    __add
    add     esp, 8
    push    eax
    pop     eax
    add     esp, 0
    jmp     __main_ret
    add     esp, 0
__main_ret:
    pop     ebp
    mov     ebx, eax
    mov     eax, 1
    int     0x80
