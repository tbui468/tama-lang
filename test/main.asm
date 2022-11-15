_start:
    mov     ebp, esp
    call    main
    mov     ebx, eax
    mov     eax, 0x1
    int     0x80
main:
    push    ebp
    mov     ebp, esp
    sub     esp, 8
    mov     eax, 1
    mov     [ebp + -4], eax
    mov     eax, 2
    mov     [ebp + -8], eax
    mov     eax, 3
    mov     [ebp + -8], eax
    mov     eax, [ebp + -4]
    add     esp, 8
    pop     ebp
    ret
