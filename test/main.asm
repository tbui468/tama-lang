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
    push    5
    mov     eax, 0
    mov     ecx, 5
    sub     eax, ecx
    mov     [ebp + -4], eax
    mov     eax, [ebp + -4]
    push    eax
    call    myadd
    mov     [ebp + -8], eax
    add     esp, 8
    mov     eax, [ebp + -8]
    add     esp, 8
    pop     ebp
    ret
