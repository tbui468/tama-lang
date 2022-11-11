_start:
    _t0 = call main
    push_arg _t0
    call _exit
main:
    begin_fun 8
    _t1x = 1 
    _t1x = 2 
    push_arg _t1x
    call _print_int
    pop_args 4
    _t2x = 3 
    _t2x = 4 
    _t1x = 5 
    return 0
    end_fun 
