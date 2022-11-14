double:
    begin_fun 20
    _t0 = 4 + 38
    _t1x = _t0 
_L0:
    _t2 = _t1x < 10
    if_z_goto _t2 _L1
    _t3 = _t1x + 1
    _t1x = _t3 
    goto _L0
_L1:
    _t4 = _t1x + b
    return _t4
    end_fun 
main:
    begin_fun 16
    _t5y = 0 
    _t6 = _t5y < 10
    if_z_goto _t6 _L2
    _t5y = 1 
    goto _L3
_L2:
    _t5y = 42 
_L3:
    push_arg _t5y
    push_arg 1
    _t7 = call double
    pop_args 8
    push_arg 21
    push_arg 0
    _t8 = call double
    pop_args 8
    return _t8
    end_fun 
