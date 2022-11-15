    entry <alignment>
main:
    begin_fun 12
    _t0x = 0 
_L0:
    _t1 = _t0x < 5
    goto_ifz _t1 _L1
    _t2 = _t0x + 1
    _t0x = _t2 
    goto _L0
_L1:
    return _t0x
    end_fun 
