_start:
    _t0 = call main
    push_arg _t0
    call _exit
double:
    begin_fun 20
    _t1x = 0 
    _t2y = 0 
_L0:
    _t3 = _t1x < a
    if_z_goto _t3 _L1
    _t4 = _t1x + 1
    _t1x = _t4 
    goto _L0
_L1:
    _t5 = _t1x + b
    return _t5
    end_fun 
main:
    begin_fun 4
    push_arg 21
    push_arg 0
    _t6 = call double
    pop_args 8
    return _t6
    end_fun 
