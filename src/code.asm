            global      main
            extern      printf

            section     .text
main:
            ;saving three registers
            ;stack pointer (rsp) needs to be 16-byte aligned before making call
            ;NOTE: making a call pushes 8-byte return address to stack!
            ;      Pushing rax and 8-byte return address will align it to 16-byte
            ;printf may destroy rax and rcx, so we push these and restore after the printf call
            push        rax
            push        rcx

            mov         rax, [const]
            add         rax, [const + 8]
            add         rax, [const + 16]
            neg         rax
            add         rax, [const + 24]

            mov         rdi, format     ;arg1 
            mov         rsi, rax        ;arg2
            xor         rax, rax        ;since printf takes variable arguments...?

            call        printf

            pop         rcx
            pop         rax
            ret

            section     .data
format:     db "%ld", 10, 0
const:     dq 1, 2, 3, 4
