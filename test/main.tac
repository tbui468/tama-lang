_start:
    _t0 = call main
    push_arg _t0
    call _exit
double:
    begin_fun 20
    _t1 = 4 + 38
    _t2x = _t1 
_L0:
    _t3 = _t2x < 10
    if_z_goto _t3 _L1
    _t4 = _t2x + 1
    _t2x = _t4 
    goto _L0
_L1:
    _t5 = _t2x + b
    return _t5
    end_fun 
main:
    begin_fun 16
    _t6y = 0 
    _t7 = _t6y < 10
    if_z_goto _t7 _L2
    _t6y = 1 
    goto _L3
_L2:
    _t6y = 42 
_L3:
    push_arg _t6y
    push_arg 1
    _t8 = call double
    pop_args 8
    push_arg 21
    push_arg 0
    _t9 = call double
    pop_args 8
    return _t9
    end_fun 
