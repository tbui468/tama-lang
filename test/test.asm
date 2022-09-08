org     0x08048000 
mov     eax, 26
mov     ecx, 30
add     eax, ecx
mov     ecx, 35
sub     eax, ecx
mov     ecx, 2
imul    eax, ecx
mov     ebx, eax
mov     eax, 1
int     0x80
