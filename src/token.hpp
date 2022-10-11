#ifndef TMD_TOKEN_H
#define TMD_TOKEN_H

#include <stdint.h>

enum TokenType {
    //@Note: order here is necessary for machine code mapping tables
    T_EAX = 0,
    T_ECX,
    T_EDX,
    T_EBX,
    T_ESP,
    T_EBP,
    T_ESI,
    T_EDI,

    //tokenizing tamarind file
    T_INT,
    T_HEX,
    T_FLOAT,
    T_PLUS,
    T_MINUS,
    T_STAR,
    T_SLASH,
    T_L_PAREN,
    T_R_PAREN,
    T_NEWLINE,
    T_PRINT,
    T_NIL,
    T_EOF,
    T_COLON,
    T_INT_TYPE,
    T_NIL_TYPE,
    T_EQUAL,
    T_IDENTIFIER,
    T_TRUE,
    T_FALSE,
    T_BOOL_TYPE,
    T_L_BRACE,
    T_R_BRACE,
    T_LESS,
    T_GREATER,
    T_LESS_EQUAL,
    T_GREATER_EQUAL,
    T_EQUAL_EQUAL,
    T_NOT_EQUAL,
    T_NOT,
    T_AND,
    T_OR,
    T_IF,
    T_ELIF,
    T_ELSE,
    T_WHILE,
    T_L_BRACKET,
    T_R_BRACKET,
    T_COMMA,

    //tokening assembly file
    T_MOV,
    T_PUSH,
    T_POP,
    T_ADD,
    T_SUB,
    T_IMUL,
    T_IDIV,
    T_INTR,
    T_DOLLAR,   //Let's just force user to define labels instead of compiling $$
    T_EQU,       //used to compute sizes (should compute the value and then patch immediately)
    T_ORG,
    T_CDQ,
    T_XOR,
    T_CALL,
    T_RET,
    T_JMP,
    T_JG,
    T_CMP
};

struct Token {
    enum TokenType type;
    char *start;
    int len; 
    int line;
};

struct TokenArray {
    struct Token *tokens;
    int count;
    int max_count;
};




uint32_t get_double(struct Token t);
uint8_t get_byte(struct Token t);
void ta_init(struct TokenArray *ta);
void ta_free(struct TokenArray *ta);
void ta_add(struct TokenArray *ta, struct Token t);

#endif //TMD_TOKEN_H