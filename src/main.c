#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static char* code = "43 + 65";

struct Lexer {
    char* code;
    int current;
};

enum TokenType {
    T_NUMBER,
    T_PLUS,
    T_MINUS,
    T_EOF
};

struct Token {
    enum TokenType type;
    char *start;
    int len; 
};


struct TokenArray {
    struct Token *tokens;
    int count;
    int max_count;
};

void ta_init(struct TokenArray *ta) {
    ta->tokens = NULL;
    ta->count = 0;
    ta->max_count = 0;
}

void ta_add(struct TokenArray *ta, struct Token t) {
    if (ta->max_count == 0) {
        ta->tokens = realloc(NULL, sizeof(struct Token) * 8);
        ta->max_count = 8;
    }else if (ta->count + 1 > ta->max_count) {
        ta->max_count *= 2;
        ta->tokens = realloc(ta->tokens, sizeof(struct Token) * ta->max_count);
    }

    ta->tokens[ta->count] = t;
    ta->count++;
}

void lexer_init(struct Lexer *l) {
    l->code = code;
    l->current = 0;
}

void lexer_skip_ws(struct Lexer *l) {
    while (l->code[l->current] == ' ')
        l->current++;
}

struct Token lexer_next_token(struct Lexer *l) {

    lexer_skip_ws(l);

    struct Token t;
    char c = l->code[l->current];
    switch (c) {
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
        default:
            t.type = T_NUMBER;
            t.start = &l->code[l->current];
            t.len = 1;
            while (1) {
                char next = l->code[l->current + t.len];
                if ('0' <= next && next <= '9')
                    t.len++;
                else
                    break;
            }
            break;
    }
    l->current += t.len;
    return t;
}

int main (int argc, char **argv) {
    argc = argc;
    argv = argv;


    //Tokenize source code
    struct Lexer l;
    lexer_init(&l);

    struct TokenArray ta;
    ta_init(&ta);

    while (l.current < (int)strlen(l.code)) {
        struct Token t = lexer_next_token(&l);
        ta_add(&ta, t);
    }

    struct Token t;
    t.type = T_EOF;
    t.start = NULL;
    t.len = 0;
    ta_add(&ta, t);

    for (int i = 0; i < ta.count; i++) {
        printf("%.*s\n", ta.tokens[i].len, ta.tokens[i].start);
    }

    //Parse tokens into AST
    
    //Type check (can skip this until we have ints, floats, and booleans)

    //Compile into bytecode

    //Run bytecode on vm

    return 0;
}
