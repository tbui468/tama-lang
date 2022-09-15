org     0x08048000 
push    10
push    10
pop     ecx
pop     eax
push    eax
pop     eax
sub     eax, 5
sub     ecx, 5
add     eax, ecx
imul    eax, ecx
imul    eax, 3
cdq
mov     ecx, 15
idiv    ecx
mov     ebx, eax
mov     ebx, 0
xor     ebx, ebx
mov     eax, 1
int     0x80
