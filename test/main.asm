add:
    push    ebp
    sub     esp, 4
    mov     eax, [ebp + 8]
    mov     ecx, [ebp + 12]
    add     eax, ecx
    mov     [ebp + -4], eax
    mov     eax, _t0
    add     esp, 4
    pop     ebp
    ret
main:
    push    ebp
    sub     esp, 12
    push    9
<not implemented>
    mov     eax, [ebp + -4]
    push    eax
<not implemented>
    add     esp, 8
<not implemented>
    mov     eax, _t3x
    add     esp, 12
    pop     ebp
    ret
