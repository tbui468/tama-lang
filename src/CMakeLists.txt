set(Sources
    main.c
    ast.c
    memory.c
    token.c
    byte_array.c
    assembler.c
    error.c
    compiler.c
    )

set(Headers
    ast.h
    memory.h
    token.h
    byte_array.h
    assembler.h
    error.h
    compiler.h
    )

add_executable(
    tama
    ${Headers}
    ${Sources}
    )
