__main:
    push    ebp
    mov     ebp, esp
    push    0
    push    5
    mov     eax, [ebp - 8]
    push    eax
    mov     eax, [ebp - 4]
    push    eax
    call    __sub
    add     esp, 8
    push    eax
    jmp     __while_condition0
__while_block0:
    mov     eax, [ebp - 4]
    push    eax
    push    1
    pop     ebx
    pop     eax
    add     eax, ebx
    push    eax
    pop     eax
    mov     [ebp - 4], eax
    push    eax
    pop     ebx
    add     esp, 0
__while_condition0:
    mov     eax, [ebp - 4]
    push    eax
    mov     eax, [ebp - 8]
    push    eax
    pop     ebx
    pop     eax
    cmp     eax, ebx
    setl    al
    movzx   eax, al
    push    eax
    pop     eax
    cmp     eax, 1
    je      __while_block0
    mov     eax, [ebp - 4]
    push    eax
    mov     eax, [ebp - 8]
    push    eax
    pop     ebx
    pop     eax
    cmp     eax, ebx
    setg    al
    movzx   eax, al
    push    eax
    pop     eax
    cmp     eax, 0
    je      __else_block1
    push    4
    pop     eax
    add     esp, 12
    jmp     __main_ret
    add     esp, 0
    jmp     __if_end1
__else_block1:
    mov     eax, [ebp - 8]
    push    eax
    mov     eax, [ebp - 4]
    push    eax
    call    __add
    add     esp, 8
    push    eax
    push    32
    push    0
    call    __add
    add     esp, 8
    push    eax
    pop     ebx
    pop     eax
    add     eax, ebx
    push    eax
    mov     eax, [ebp - 12]
    push    eax
    pop     ebx
    pop     eax
    add     eax, ebx
    push    eax
    pop     eax
    add     esp, 12
    jmp     __main_ret
    add     esp, 0
__if_end1:
    add     esp, 12
__main_ret:
    pop     ebp
    mov     ebx, eax
    mov     eax, 1
    int     0x80
