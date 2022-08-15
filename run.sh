#!/bin/bash
cd build
cmake --build .
cd src
./tama
nasm -felf32 out.asm && ld -m elf_i386 out.o && ./a.out
