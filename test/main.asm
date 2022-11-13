_start:
double:
    push    ebp
    sub     esp, 20
_L0:
    jmp     _L0
_L1:
main:
    push    ebp
    sub     esp, 16
    jmp     _L3
_L2:
_L3:
    push    1
    add     esp, 8
    push    21
    push    0
    add     esp, 8
