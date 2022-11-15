_start:
    mov     ebp, esp
    call    main
    mov     ebx, eax
    mov     eax, 0x1
    int     0x80
main:
    push    ebp
    sub     esp, 0
    mov     eax, 42
    add     esp, 0
    pop     ebp
    ret
