__main:
    push    ebp
    mov     ebp, esp
    push    42
    pop     eax
    add     esp, 0
    jmp     __main_ret
    add     esp, 0
__main_ret:
    pop     ebp
    ret
