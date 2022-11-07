__main:
    push    ebp
    mov     ebp, esp
    push    9
    jmp     __while_condition0
__while_block0:
    mov     eax, [ebp - 4]
    push    eax
    push    1
    pop     ebx
    pop     eax
    sub     eax, ebx
    push    eax
    pop     eax
    mov     [ebp - 4], eax
    push    eax
    pop     ebx
    add     esp, 0
__while_condition0:
    mov     eax, [ebp - 4]
    push    eax
    push    0
    pop     ebx
    pop     eax
    cmp     eax, ebx
    setg    al
    movzx   eax, al
    push    eax
    pop     eax
    cmp     eax, 1
    je      __while_block0
    mov     eax, [ebp - 4]
    push    eax
    pop     eax
    add     esp, 4
    jmp     __main_ret
    add     esp, 4
__main_ret:
    pop     ebp
    mov     ebx, eax
    mov     eax, 1
    int     0x80
