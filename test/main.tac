_start:
    entry 
    call main
    exit 
main:
    begin_fun 4
    _t0 = 10 < 0
    goto_cond _t0 _L1 _L0
_L0:
    return 1
_L1:
    return 2
    end_fun 
