_start:
    entry 
    call main
    exit 
main:
    begin_fun 16
    _t0x = 0 
    _t1 = 0 < 10
    goto_cond _t1 _L2 _L0
_L0:
    _t2 = _t0x + 1
    _t0x = _t2 
    goto _L1
_L2:
    _t3 = _t0x + 2
    _t0x = _t3 
    goto _L1
_L1:
    return _t0x
    end_fun 
