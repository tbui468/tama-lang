    entry <alignment>
myadd:
    begin_fun 4
    _t0 = a + b
    return _t0
    end_fun 
main:
    begin_fun 28
    _t1x = 0 
    goto _L0
_L0:
    _t2 = _t1x < 10
    goto_cond _t2 _L2 _L1
_L1:
    _t3 = _t1x < 0
    goto_cond _t3 _L5 _L3
_L3:
    push_arg 1
    push_arg _t1x
    _t4 = call myadd
    pop_args 8
    _t1x = _t4 
    goto _L4
_L5:
    push_arg 2
    push_arg _t1x
    _t5 = call myadd
    pop_args 8
    _t1x = _t5 
    goto _L4
_L4:
    _t6 = 0 < _t1x
    goto_cond _t6 _L7 _L6
_L6:
    return 0
_L7:
    goto _L0
_L2:
    return _t1x
_L8:
    _t7z = 10 
    return _t7z
    end_fun 
