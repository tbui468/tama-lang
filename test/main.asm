_start:
    mov     ebp, esp
    call    main
    mov     ebx, eax
    mov     eax, 0x1
    int     0x80
main:
    push    ebp
    mov     ebp, esp
    sub     esp, 20
    mov     eax, 10
    mov     [ebp + -4], eax
    mov     eax, [ebp + -4]
    mov     [ebp + -12], eax
    mov     eax, 0
    mov     [ebp + -20], eax
    mov     eax, 0
    add     esp, 20
    pop     ebp
    ret
