#ifndef TMD_LEXER_H
#define TMD_LEXER_H

#include <stdbool.h>

#include "token.hpp"

struct Lexer {
    char* code;
    int current;
    int line;
    struct ReservedWord *reserved_words;
    int reserved_count;
};

struct ReservedWord {
    char* word;
    int len;
    enum TokenType type;
};


void lexer_init(struct Lexer *l, char* code, struct ReservedWord *reserved_words, int reserved_count);
void lexer_skip_ws(struct Lexer *l);
bool is_digit(char c);
bool is_char(char c);
struct Token lexer_read_word(struct Lexer *l);
struct Token lexer_read_number(struct Lexer *l);
struct Token lexer_next_token(struct Lexer *l);

#endif //TMD_LEXER_H

