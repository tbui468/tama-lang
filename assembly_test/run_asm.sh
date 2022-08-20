#!/bin/bash
nasm -felf32 stack_test.asm && ld -m elf_i386 stack_test.o && ./a.out
