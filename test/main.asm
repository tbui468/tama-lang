_start:
    mov     ebp, esp
    call    main
    mov     ebx, eax
    mov     eax, 0x1
    int     0x80
myadd:
    push    ebp
    mov     ebp, esp
    sub     esp, 4
    mov     eax, [ebp + 8]
    mov     ecx, [ebp + 12]
    add     eax, ecx
    mov     [ebp + -4], eax
    mov     eax, [ebp + -4]
    add     esp, 4
    pop     ebp
    ret
main:
    push    ebp
    mov     ebp, esp
    sub     esp, 28
    mov     eax, 0
    mov     [ebp + -4], eax
    jmp     _L0
_L0:
    mov     eax, [ebp + -4]
    mov     ecx, 10
    cmp     eax, ecx
    setl    al
    movzx   eax, al
    mov     [ebp + -8], eax
    mov     eax, [ebp + -8]
    cmp     eax, 0
    je      _L2
_L1:
    mov     eax, [ebp + -4]
    mov     ecx, 0
    cmp     eax, ecx
    setl    al
    movzx   eax, al
    mov     [ebp + -12], eax
    mov     eax, [ebp + -12]
    cmp     eax, 0
    je      _L5
_L3:
    push    1
    mov     eax, [ebp + -4]
    push    eax
    call    myadd
    mov     [ebp + -16], eax
    add     esp, 8
    mov     eax, [ebp + -16]
    mov     [ebp + -4], eax
    jmp     _L4
_L5:
    push    2
    mov     eax, [ebp + -4]
    push    eax
    call    myadd
    mov     [ebp + -16], eax
    add     esp, 8
    mov     eax, [ebp + -16]
    mov     [ebp + -4], eax
    jmp     _L4
_L4:
    mov     eax, 0
    mov     ecx, [ebp + -4]
    cmp     eax, ecx
    setl    al
    movzx   eax, al
    mov     [ebp + -16], eax
    mov     eax, [ebp + -16]
    cmp     eax, 0
    je      _L7
_L6:
    mov     eax, 0
    add     esp, 28
    pop     ebp
    ret
_L7:
    jmp     _L0
_L2:
    mov     eax, [ebp + -4]
    add     esp, 28
    pop     ebp
    ret
_L8:
    mov     eax, 10
    mov     [ebp + -12], eax
    mov     eax, [ebp + -12]
    add     esp, 28
    pop     ebp
    ret
