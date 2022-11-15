nasm -f elf32 main.s
ld -m elf_i386 main.o
chmod +x a.out
./a.out ; echo $?
