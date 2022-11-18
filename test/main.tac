    entry <alignment>
mysub:
    begin_fun 16
    _t0 = a < b
    goto_cond _t0 _L2 _L0
_L0:
    _t1x = 3 
    goto _L1
_L2:
    _t2x = 10 
    goto _L1
_L1:
    _t3 = a - b
    return _t3
    end_fun 
myadd:
    begin_fun 4
    _t4 = a + b
    return _t4
    end_fun 
main:
    begin_fun 28
    _t5x = 0 
    goto _L3
_L3:
    _t6 = _t5x < 10
    goto_cond _t6 _L5 _L4
_L4:
    _t7 = _t5x < 0
    goto_cond _t7 _L8 _L6
_L6:
    push_arg 1
    push_arg _t5x
    _t8 = call myadd
    pop_args 8
    _t5x = _t8 
    goto _L7
_L8:
    push_arg 2
    push_arg _t5x
    _t9 = call myadd
    pop_args 8
    _t5x = _t9 
    goto _L7
_L7:
    _t10 = 0 < _t5x
    goto_cond _t10 _L10 _L9
_L9:
    return 0
_L10:
    goto _L3
_L5:
    return _t5x
_L11:
    _t11z = 10 
    return _t11z
    end_fun 
