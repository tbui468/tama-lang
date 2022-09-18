#include <string.h>

#include "lexer.h"
#include "error.h"

uint8_t tmd_reserved[] = {
    0, 1
};

//set this in the lexer
uint8_t asm_reserved[] = {
    2, 3
};


struct ReservedWord reserved_words[] = {
    {"while", 4, T_WHILE},
    {"pop", 3, T_POP},
    {"push", 4, T_PUSH}
};


void lexer_init(struct Lexer *l, char* code, enum SyntaxType st) {
    l->code = code;
    l->current = 0;
    l->line = 1;
    l->st = st;
}

void lexer_skip_ws(struct Lexer *l) {
    while (l->code[l->current] == ' ')
        l->current++;
}

bool is_digit(char c) {
    return '0' <= c && c <= '9';
}

bool is_char(char c) {
    return ('A' <= c && c <= 'Z') || ('a' <= c && c <= 'z');
}

//Could be a keyword or a user defined word
struct Token lexer_read_word(struct Lexer *l) {
    struct Token t;
    t.start = &l->code[l->current];
    t.len = 1;
    t.line = l->line;

    while (1) {
        char next = l->code[l->current + t.len];
        if (!(is_digit(next) || is_char(next) || next == '_'))
            break;
        t.len++;
    }

    if (l->st == ST_TMD) {
        if (t.len == 5 && strncmp("print", t.start, 5) == 0) {
            t.type = T_PRINT;
        } else if (t.len == 3 && strncmp("int", t.start, 3) == 0) {
            t.type = T_INT_TYPE;
        } else if (t.len == 4 && strncmp("bool", t.start, 4) == 0) {
            t.type = T_BOOL_TYPE;
        } else if (t.len == 4 && strncmp("true", t.start, 4) == 0) {
            t.type = T_TRUE;
        } else if (t.len == 5 && strncmp("false", t.start, 5) == 0) {
            t.type = T_FALSE;
        } else if (t.len == 3 && strncmp("and", t.start, 3) == 0) {
            t.type = T_AND;
        } else if (t.len == 2 && strncmp("or", t.start, 2) == 0) {
            t.type = T_OR; 
        } else if (t.len == 2 && strncmp("if", t.start, 2) == 0) {
            t.type = T_IF;
        } else if (t.len == 4 && strncmp("elif", t.start, 4) == 0) {
            t.type = T_ELIF;
        } else if (t.len == 4 && strncmp("else", t.start, 4) == 0) {
            t.type = T_ELSE;
        } else if (t.len == 5 && strncmp("while", t.start, 5) == 0) {
            t.type = T_WHILE;
        } else {
            t.type = T_IDENTIFIER;
        }
    } else if (l->st == ST_ASM) {
        if (t.len == 3 && strncmp("mov", t.start, 3) == 0) {
            t.type = T_MOV;
        } else if (t.len == 4 && strncmp("push", t.start, 4) == 0) {
            t.type = T_PUSH;
        } else if (t.len == 3 && strncmp("pop", t.start, 3) == 0) {
            t.type = T_POP;
        } else if (t.len == 3 && strncmp("add", t.start, 3) == 0) {
            t.type = T_ADD;
        } else if (t.len == 3 && strncmp("sub", t.start, 3) == 0) {
            t.type = T_SUB;
        } else if (t.len == 4 && strncmp("imul", t.start, 4) == 0) {
            t.type = T_IMUL;
        } else if (t.len == 4 && strncmp("idiv", t.start, 4) == 0) {
            t.type = T_IDIV;
        } else if (t.len == 3 && strncmp("eax", t.start, 3) == 0) {
            t.type = T_EAX;
        } else if (t.len == 3 && strncmp("edx", t.start, 3) == 0) {
            t.type = T_EDX;
        } else if (t.len == 3 && strncmp("ecx", t.start, 3) == 0) {
            t.type = T_ECX;
        } else if (t.len == 3 && strncmp("ebx", t.start, 3) == 0) {
            t.type = T_EBX;
        } else if (t.len == 3 && strncmp("esi", t.start, 3) == 0) {
            t.type = T_ESI;
        } else if (t.len == 3 && strncmp("edi", t.start, 3) == 0) {
            t.type = T_EDI;
        } else if (t.len == 3 && strncmp("esp", t.start, 3) == 0) {
            t.type = T_ESP;
        } else if (t.len == 3 && strncmp("ebp", t.start, 3) == 0) {
            t.type = T_EBP;
        } else if (t.len == 3 && strncmp("int", t.start, 3) == 0) {
            t.type = T_INTR;
        } else if (t.len == 1 && strncmp("$", t.start, 1) == 0) {
            t.type = T_DOLLAR;
        } else if (t.len == 3 && strncmp("equ", t.start, 3) == 0) {
            t.type = T_EQU;
        } else if (t.len == 3 && strncmp("org", t.start, 3) == 0) {
            t.type = T_ORG;
        } else if (t.len == 3 && strncmp("cdq", t.start, 3) == 0) {
            t.type = T_CDQ;
        } else if (t.len == 3 && strncmp("xor", t.start, 3) == 0) {
            t.type = T_XOR;
        } else {
            t.type = T_IDENTIFIER;
        }
    }

    return t;
}

struct Token lexer_read_number(struct Lexer *l) {
    struct Token t;
    t.start = &l->code[l->current];
    t.len = 1;
    t.line = l->line;
    bool has_decimal = *t.start == '.';
    bool is_hex = false;

    if (l->code[l->current] == '0' && l->code[l->current + 1] == 'x') {
        is_hex = true;
        t.len++;
    }

    while (1) {
        char next = l->code[l->current + t.len];
        if (is_digit(next)) {
            t.len++;
        } else if (next == '.') {
            t.len++;
            if (has_decimal) {
                ems_add(&ems, l->line, "Token Error: Too many decimals!");
            } else {
                has_decimal = true;
            }
        } else {
            break;
        }
    }

    t.type = has_decimal ? T_FLOAT : is_hex ? T_HEX : T_INT;
    return t;
}

struct Token lexer_next_token(struct Lexer *l) {

    lexer_skip_ws(l);

    struct Token t;
    t.line = l->line;
    char c = l->code[l->current];
    switch (c) {
        case '\n':
            t.type = T_NEWLINE;
            t.start = &l->code[l->current];
            t.len = 1;
            break;
        case '+':
            t.type = T_PLUS;
            t.start = &l->code[l->current];
            t.len = 1;
            break;
        case '-':
            t.type = T_MINUS;
            t.start = &l->code[l->current];
            t.len = 1;
            break;
        case '*':
            t.type = T_STAR;
            t.start = &l->code[l->current];
            t.len = 1;
            break;
        case '/':
            t.type = T_SLASH;
            t.start = &l->code[l->current];
            t.len = 1;
            break;
        case '(':
            t.type = T_L_PAREN;
            t.start = &l->code[l->current];
            t.len = 1;
            break;
        case ')':
            t.type = T_R_PAREN;
            t.start = &l->code[l->current];
            t.len = 1;
            break;
        case ':':
            t.type = T_COLON;
            t.start = &l->code[l->current];
            t.len = 1;
            break;
        case '<':
            t.start = &l->code[l->current];
            if (l->current + 1 < (int)strlen(l->code) && l->code[l->current + 1] == '=') {
                t.type = T_LESS_EQUAL;
                t.len = 2;
            } else {
                t.type = T_LESS;
                t.len = 1;
            }
            break;
        case '>':
            t.start = &l->code[l->current];
            if (l->current + 1 < (int)strlen(l->code) && l->code[l->current + 1] == '=') {
                t.type = T_GREATER_EQUAL;
                t.len = 2;
            } else {
                t.type = T_GREATER;
                t.len = 1;
            }
            break;
        case '=':
            t.start = &l->code[l->current];
            if (l->current + 1 < (int)strlen(l->code) && l->code[l->current + 1] == '=') {
                t.type = T_EQUAL_EQUAL;
                t.len = 2;
            } else {
                t.type = T_EQUAL;
                t.len = 1;
            }
            break;
        case '!':
            t.start = &l->code[l->current];
            if (l->current + 1 < (int)strlen(l->code) && l->code[l->current + 1] == '=') {
                t.type = T_NOT_EQUAL;
                t.len = 2;
            } else {
                t.type = T_NOT;
                t.len = 1;
            }
            break;
        case '{':
            t.type = T_L_BRACE;
            t.start = &l->code[l->current];
            t.len = 1;
            break;
        case '}':
            t.type = T_R_BRACE;
            t.start = &l->code[l->current];
            t.len = 1;
            break;
        case '[':
            t.type = T_L_BRACKET;
            t.start = &l->code[l->current];
            t.len = 1;
            break;
        case ']':
            t.type = T_R_BRACKET;
            t.start = &l->code[l->current];
            t.len = 1;
            break;
        case ',':
            t.type = T_COMMA;
            t.start = &l->code[l->current];
            t.len = 1;
            break;
        default:
            if (is_digit(c)) {
                t = lexer_read_number(l);
            } else {
                t = lexer_read_word(l);
            }
            break;
    }
    l->current += t.len;
    return t;
}
