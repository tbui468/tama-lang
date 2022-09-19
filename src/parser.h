#ifndef TMD_PARSER_H
#define TMD_PARSER_H

#include "token.h"

struct Parser {
    struct TokenArray *ta;
    int current;
};

struct Token parser_peek_two(struct Parser *p);
struct Token parser_peek_one(struct Parser *p);
struct Token parser_next(struct Parser *p);
struct Token parser_consume(struct Parser *p, enum TokenType tt);
bool parser_end(struct Parser *p);
void parser_init(struct Parser *p, struct TokenArray *ta);

struct Node *parse_group(struct Parser *p);
struct Node* parse_literal(struct Parser *p);
struct Node* parse_unary(struct Parser *p);
struct Node* parse_mul_div(struct Parser *p);
struct Node* parse_add_sub(struct Parser *p);
struct Node* parse_inequality(struct Parser *p);
struct Node *parse_equality(struct Parser *p);
struct Node *parse_and(struct Parser *p);
struct Node *parse_or(struct Parser *p);
struct Node* parse_assignment(struct Parser *p);
struct Node* parse_expr(struct Parser *p);
struct Node* parse_block(struct Parser *p);
struct Node *parse_stmt(struct Parser *p);

struct Node *aparse_literal(struct Parser *p);
struct Node *aparse_expr(struct Parser *p);
struct Node *aparse_stmt(struct Parser *p);

#endif //TMD_PARSER_H
