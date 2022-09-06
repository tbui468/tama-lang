org     0x08048000 

mov     ebx, 30
push    ebx
push    12
mov     ebx, 0
pop     ebx
pop     ecx
add     ebx, ecx
mov     eax, 1
int     0x80
