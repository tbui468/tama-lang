#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

static char* code = "43 + 65*9/-7";


/*
 * Tokens
 */
enum TokenType {
    T_NUMBER,
    T_PLUS,
    T_MINUS,
    T_STAR,
    T_SLASH,
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


/*
 * AST Nodes
 */

enum NodeType {
    NODE_BINARY,
    NODE_UNARY,
    NODE_LITERAL
};

struct Node {
    enum NodeType type;
};

struct NodeBinary {
    struct Node n;
    struct Node *left;
    struct Token op;
    struct Node *right;
};

struct NodeUnary {
    struct Node n;
    struct Token op;
    struct Node *right;
};

struct NodeLiteral {
    struct Node n;
    struct Token value; 
};

struct NodeArray {
    struct Node **nodes;
    int count;
    int max_count;
};

void na_init(struct NodeArray *na) {
    na->nodes = NULL;
    na->count = 0;
    na->max_count = 0;
}

void na_add(struct NodeArray *na, struct Node* n) {
    if (na->max_count == 0) {
        na->max_count = 0;
        na->nodes = realloc(NULL, sizeof(struct Node*) * na->max_count);
    } else if (na->count + 1 > na->max_count) {
        na->max_count *= 2;
        na->nodes = realloc(na->nodes, sizeof(struct Node*) * na->max_count);
    }

    na->nodes[na->count++] = n;
}

struct Node* node_literal(struct Token value) {
    struct NodeLiteral *node = realloc(NULL, sizeof(struct NodeLiteral));
    node->n.type = NODE_LITERAL;
    node->value = value;
    return (struct Node*)node;
}

struct Node* node_unary(struct Token op, struct Node* right) {
    struct NodeUnary *node = realloc(NULL, sizeof(struct NodeUnary));
    node->n.type = NODE_UNARY;
    node->op = op;
    node->right = right;
    return (struct Node*)node;
}

struct Node* node_binary(struct Node* left, struct Token op, struct Node* right) {
    struct NodeBinary *node = realloc(NULL, sizeof(struct NodeBinary));
    node->n.type = NODE_BINARY;
    node->left = left;
    node->op = op;
    node->right = right;
    return (struct Node*)node;
}

void ast_print(struct Node* n) {
    switch (n->type) {
        case NODE_LITERAL: {
            struct NodeLiteral* l = (struct NodeLiteral*)n;
            printf("%.*s", l->value.len, l->value.start);
            break;
        }
        case NODE_UNARY: {
            struct NodeUnary *u = (struct NodeUnary*)n;
            printf("(");
            printf("%.*s", u->op.len, u->op.start);
            ast_print(u->right);
            printf(")");
            break;
        }
        case NODE_BINARY: {
            struct NodeBinary* b = (struct NodeBinary*)n;
            printf("(");
            ast_print(b->left);
            printf("%.*s", b->op.len, b->op.start);
            ast_print(b->right);
            printf(")");
            break;
        }
        default:
            printf("Node type not recognized\n");
            break;
    }
}

/*
 *  Parser
 */

struct Parser {
    struct TokenArray *ta;
    int current;
};

struct Token parser_peek(struct Parser *p) {
    return p->ta->tokens[p->current];
}

struct Token parser_consume(struct Parser *p) {
    return p->ta->tokens[p->current++];
}

bool parser_end(struct Parser *p) {
    return p->current == p->ta->count;
}

void parser_init(struct Parser *p, struct TokenArray *ta) {
    p->ta = ta;
    p->current = 0;
}

struct Node* parse_literal(struct Parser *p) {
    return node_literal(parser_consume(p));
}

struct Node* parse_unary(struct Parser *p) {
    struct Token next = parser_peek(p);
    if (next.type == T_MINUS) {
        struct Token op = parser_consume(p);
        return node_unary(op, parse_unary(p));
    } else {
        return parse_literal(p);
    }
}

struct Node* parse_mul_div(struct Parser *p) {
    struct Node *left = parse_unary(p);

    while (1) {
        struct Token next = parser_peek(p);
        if (next.type != T_STAR && next.type != T_SLASH)
            break;

        struct Token op = parser_consume(p);
        struct Node *right = parse_unary(p);
        left = node_binary(left, op, right);
    }

    return left;
}

struct Node* parse_add_sub(struct Parser *p) {
    struct Node *left = parse_mul_div(p);

    while (1) {
        struct Token next = parser_peek(p);
        if (next.type != T_MINUS && next.type != T_PLUS)
            break;

        struct Token op = parser_consume(p);
        struct Node *right = parse_mul_div(p);
        left = node_binary(left, op, right);
    }

    return left;
}

struct Node* parser_parse(struct Parser *p) {
    return parse_add_sub(p); 
}
/*
 * Lexer
 */

struct Lexer {
    char* code;
    int current;
};

void ta_init(struct TokenArray *ta) {
    ta->tokens = NULL;
    ta->count = 0;
    ta->max_count = 0;
}

void ta_add(struct TokenArray *ta, struct Token t) {
    if (ta->max_count == 0) {
        ta->tokens = realloc(NULL, sizeof(struct Token) * 8); //TODO: Need to check for errors with sys calls
        ta->max_count = 8;
    }else if (ta->count + 1 > ta->max_count) {
        ta->max_count *= 2;
        ta->tokens = realloc(ta->tokens, sizeof(struct Token) * ta->max_count); //TODO: check for errors
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
    struct Parser p;
    parser_init(&p, &ta);
    struct NodeArray na;
    na_init(&na);
    while (parser_peek(&p).type != T_EOF) {
        na_add(&na, parser_parse(&p));
    }

    for (int i = 0; i < na.count; i++) {
        ast_print(na.nodes[i]);
        printf("\n");
    }
    
    //Type check (can skip this until we have ints, floats, and booleans)

    //Compile into bytecode

    //Run bytecode on vm (stack or register based vm???)

    return 0;
}
