_start:
    mov     ebp, esp
    call    main
    mov     ebx, eax
    mov     eax, 0x1
    int     0x80
main:
    push    ebp
    mov     ebp, esp
    sub     esp, 16
    mov     eax, 0
    mov     [ebp + -4], eax
    mov     eax, 0
    mov     ecx, 10
    cmp     eax, ecx
    setl    al
    movzx   eax, al
    mov     [ebp + -8], eax
    mov     eax, [ebp + -8]
    cmp     eax, 0
    je      _L0
    mov     eax, [ebp + -4]
    mov     ecx, 1
    add     eax, ecx
    mov     [ebp + -12], eax
    mov     eax, [ebp + -12]
    mov     [ebp + -4], eax
    jmp     _L1
_L0:
    mov     eax, [ebp + -4]
    mov     ecx, 2
    add     eax, ecx
    mov     [ebp + -12], eax
    mov     eax, [ebp + -12]
    mov     [ebp + -4], eax
_L1:
    mov     eax, [ebp + -4]
    add     esp, 16
    pop     ebp
    ret
