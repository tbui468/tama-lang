global _start

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
    mov     eax, 3
    mov     [ebp - 4], eax
    mov     eax, [ebp - 4]
    add     esp, 4
    pop     ebp
    ret
