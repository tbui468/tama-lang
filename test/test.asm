org     0x08048000 

mov     ebx, 30
push    ebx
push    18
mov     ebx, 0
pop     ebx
pop     ecx
add     ebx, ecx
sub     ebx, 8
mov     eax, 1
int     0x80
