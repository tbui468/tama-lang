_start:
    mov     ebp, esp
    call    main
    mov     ebx, eax
    mov     eax, 0x1
    int     0x80
main:
    push    ebp
    mov     ebp, esp
    sub     esp, 4
    mov     eax, 0
    mov     [ebp + -4], eax
    mov     eax, [ebp + -4]
    cmp     eax, 0
    je      _L1
    jmp     _L0
_L0:
    mov     eax, 1
    add     esp, 4
    pop     ebp
    ret
_L1:
    mov     eax, 2
    add     esp, 4
    pop     ebp
    ret
