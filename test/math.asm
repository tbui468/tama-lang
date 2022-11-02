__add:
    push    ebp
    mov     ebp, esp
    mov     eax, [ebp - -8]
    push    eax
    mov     eax, [ebp - -12]
    push    eax
    pop     ebx
    pop     eax
    add     eax, ebx
    push    eax
    pop     eax
    add     esp, 0
    jmp     __add_ret
    add     esp, 0
__add_ret:
    pop     ebp
    ret
