# Tamarind Compiler

## Tamarind Lexer
## Tamarind Parser
## Tamarind Compiler
### IR data structure goes here
## x86 Code Generator
## x86 Assembler
### Introduction
Compiles x86 assembly to x86 machine code (ELF executable).
Outputs a binary executable.

x86 machine code varies in length depending on opcode types.
This is designed to use little memory, but
this also makes translating from assembly to machine code
a bit more complex (when compared to fixed-size machine
code in MIPS processors).  MIPS assembly is easier to
compile to machine code, BUT it takes up a lot more 
memory.

### x86 Op Codes
x86 machine instructions have the following structure:
    [opcode byte][ModR/RM][SIB][DISPLACEMENT][IMMEDIATE]

The first byte is the opcode.  Some instructions only have a single byte opcode,
such as 'ret'.  The remaining bytes are only required under certain conditions.


    

    [opcode] 1 or 2 bytes (only part that is required)
        first few bits are for opcode
        last two bits are for direction/???

    [ModR/RM] 1 byte (if required)
        First two bits are for mode (eg, 11 means dst and src are registers)
            00 - first operand is register, second is register in r/m
                    if r/m is 'ebp' (101), then 32-bit displacement is needed
                    if r/m is 'esp', then SIB needed
            01 - first operand is register, second is register
                    8-bit displacement required
                    if r/m is 'esp', then SIB needed
            10 - first operand is register, second is register
                    32-bit displacement required
                    same exceptions as 01...?
            11 - first and second operand are registers
        Next 3 bits are register
        Final 3 bits are R/M
            11000000 

    [SIB] 1 byte (if required)
        SIB (scale, index, base) is necessary for some opcode/ModR/RM combinations

    [DISPLACEMENT] 1, 2, 4 bytes (if required)
        Needed for some operations (such as dereferencing stack pointer/frame pointer)

    [IMMEDIATE] 1, 2, 4 bytes (if required)
        Needed for some operations (such as pushing an immediate value)

###Dereferencing (only supports 8-bit displacement with ebp for now)
Opcode byte
    0x89 for moving register into ebp with 8-bit displacement
    0x8b for moving from ebp with 8-bit displacement into register
ModR/RM byte
    First two bits are 01 to allow 8-bit displacement
    Next 3 bits are the register
    Last 3 bits are the register holding the memory address
Displacement byte
    Displacement from memory address in ebp

## ELF Format
###Minimum ELF executable format
ELF header
Program header
text
