    entry <alignment>
add:
    begin_fun 4
    _t0 = a + b
    return _t0
    end_fun 
main:
    begin_fun 12
    push_arg 9
    _t1 = 0 - 9
    push_arg _t1
    _t2 = call add
    pop_args 8
    _t3x = _t2 
    return _t3x
    end_fun 
