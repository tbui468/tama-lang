_start:
    _t0 = call main
    push_arg _t0
    call _exit
double:
    begin_fun 24
    _t1x = 224 
    _t2 = 2 + 4
    _t3y = _t2 
_L0:
    _t4 = _t1x < a
    if_z_goto _t4 _L1
    _t5 = _t1x + 1
    _t1x = _t5 
    goto _L0
_L1:
    _t6 = _t1x + b
    return _t6
    end_fun 
main:
    begin_fun 12
    _t7 = 10 - 2
    _t8z = _t7 
    push_arg 21
    push_arg 0
    _t9 = call double
    pop_args 8
    return _t9
    end_fun 
