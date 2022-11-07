__add_complex:
    push    ebp
    mov     ebp, esp
    push    1
    pop     eax
    neg     eax
    push    eax
    pop     eax
    add     esp, 0
    jmp     __add_complex_ret
    add     esp, 0
__add_complex_ret:
    pop     ebp
    ret
