_start:
    entry 
    call main
    exit 
myadd:
    begin_fun 4
    _t0 = a + b
    return _t0
    end_fun 
main:
    begin_fun 16
    _t1x = 1 
    _t2y = 5 
    _t3 = _t1x + _t2y
    _t4z = _t3 
    return _t4z
    end_fun 
