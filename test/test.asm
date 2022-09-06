org     0x08048000 

mov     ebx, 34
push    ebx
push    58
mov     ebx, 0
pop     ebx
pop     ecx
add     ebx, ecx
sub     ebx, 8
mov     eax, ebx
cdq
mov     ebx, 42
idiv    ebx
mov     esi, 40
imul    eax, esi
sub     eax, 38
mov     ebx, eax
mov     eax, 1
int     0x80
