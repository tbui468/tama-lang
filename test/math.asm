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
