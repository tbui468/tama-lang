org     0x08048000 
start:
mov     eax, 9
end:
mov     eax, end
sub     eax, start
mov     ebx, eax
mov     eax, 1
int     0x80
