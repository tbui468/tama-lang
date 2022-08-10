global      _start              ;starts with calling _start...?


            section .text       ;section with code
_start:     mov     rax, 1      ;system call to write
            mov     rdi, 1      ;stdout is fileno 1

            mov     r8, const1
            add     r8, shift 
            mov     rsi, r8
            mov     rdx, 8
            syscall

            mov     rax, 60     ;syscall for exit
            xor     rdi, rdi
            syscall


            section .data       ;section with data (constants, reserved space, etc)
msg:        db      'Hello', 10
const1:     dq      4
shift:      dq      48
