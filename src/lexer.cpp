#include <string.h>
#include <stdio.h>

#include "lexer.hpp"
#include "error.hpp"

void lexer_init(struct Lexer *l, char* code, struct ReservedWord *reserved_words, int reserved_count) {
    l->code = code;
    l->current = 0;
    l->line = 1;
    l->reserved_words = reserved_words;
    l->reserved_count = reserved_count;
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

    t.type = T_IDENTIFIER;
    for (int i = 0; i < l->reserved_count; i++) {
        struct ReservedWord *rw = &l->reserved_words[i];
        if (rw->len == t.len && strncmp(rw->word, t.start, t.len) == 0) {
            t.type = rw->type;
            break;
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
        if (is_digit(next) || (is_hex && is_char(next))) {
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
        case '$':
            t.type = T_DOLLAR;
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
