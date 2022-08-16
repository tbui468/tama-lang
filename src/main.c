#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

//static char* code = "43.23 + .65*9.0/-7.";
//static char* code = "43 + 65+9-7";
static char* code = "(9+-10) * (2 -- 100)";


/*
 * Tokens
 */
enum TokenType {
    T_INT,
    T_FLOAT,
    T_PLUS,
    T_MINUS,
    T_STAR,
    T_SLASH,
    T_L_PAREN,
    T_R_PAREN,
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
 * Compiler
 */

struct CharArray {
    char* chars;
    int count;
    int max_count;
};

struct Compiler {
    struct CharArray text;
    struct CharArray data;
    int data_offset; //in bytes
};

void ca_init(struct CharArray* ca) {
    ca->chars = NULL;
    ca->count = 0;
    ca->max_count = 0;
}

void ca_append(struct CharArray* ca, char* s, int len) {
    if (ca->max_count == 0) {
        ca->max_count = 8;
        ca->chars = realloc(NULL, sizeof(char) * ca->max_count);
    }

    while (ca->count + len > ca->max_count) {
        ca->max_count *= 2;
        ca->chars = realloc(ca->chars, sizeof(char) * ca->max_count);
    }

    memcpy(&ca->chars[ca->count], s, len);
    ca->count += len;
}

void compiler_init(struct Compiler *c) {
    ca_init(&c->text);
    ca_init(&c->data);
    c->data_offset = 0;
}

//appends to text section (op codes)
void compiler_append_text(struct Compiler *c, char *s, int len) {
    ca_append(&c->text, s, len);
}

//appends to data section
int compiler_append_data(struct Compiler *c, char *s, int len) {
    if (c->data.count != 0) {
        ca_append(&c->data, ", ", 2);
    }
    ca_append(&c->data, s, len);

    int previous_offset = c->data_offset;
    c->data_offset += 8;
    return previous_offset; 
}

//write out assembly file
void compiler_output_assembly(struct Compiler *c) {
    FILE *f = fopen("out.asm", "w");
    char *s = "section     .text\n"
              "global      _start\n"
              "%include \"../../assembly_test/fun.asm\""
              "\n"
              "_start:\n";
    char *e = "\n"
              //"    push    eax\n"
              "    call    _print_int\n"
              "    add     esp, 4\n"
              "\n"
              "    push    0xa\n"
              "    call    _print_char\n"
              "    add     esp, 4\n"
              "\n"
              "    mov     eax, 0x1\n"
              "    xor     ebx, ebx\n"
              "    int     0x80\n"
              "\n"
              "section     .data\n";
    fwrite(s, sizeof(char), strlen(s), f);
    fwrite(c->text.chars, sizeof(char), c->text.count, f);
    fwrite(e, sizeof(char), strlen(e), f);
    fwrite(c->data.chars, sizeof(char), c->data.count, f);
    fclose(f);
}

void compiler_compile(struct Compiler *c, struct Node *n) {
    switch (n->type) {
        case NODE_LITERAL: {
            struct NodeLiteral* l = (struct NodeLiteral*)n;
            char s[64];
            sprintf(s, "    push    %.*s\n", l->value.len, l->value.start);
            compiler_append_text(c, s, strlen(s));
            break;
        }
        case NODE_UNARY: {
            struct NodeUnary *u = (struct NodeUnary*)n;
            compiler_compile(c, u->right);
            char* pop_right = "    pop     eax\n\0";
            compiler_append_text(c, pop_right, strlen(pop_right));
            char* neg_op = "    neg     eax\n\0";
            compiler_append_text(c, neg_op, strlen(neg_op));
            char* push_op = "    push    eax\n\0";
            compiler_append_text(c, push_op, strlen(push_op));
            break;
        }
        case NODE_BINARY: {
            struct NodeBinary* b = (struct NodeBinary*)n;
            compiler_compile(c, b->left);
            compiler_compile(c, b->right);
            char* pop_right = "    pop     ebx\n\0";
            compiler_append_text(c, pop_right, strlen(pop_right));
            char* pop_left = "    pop     eax\n\0";
            compiler_append_text(c, pop_left, strlen(pop_left));
            if (*b->op.start == '+') {
                char* add = "    add     eax, ebx\n\0";
                compiler_append_text(c, add, strlen(add)); 
            } else if (*b->op.start == '-') {
                char* sub = "    sub     eax, ebx\n\0";
                compiler_append_text(c, sub, strlen(sub)); 
            } else if (*b->op.start == '*') {
                char* mul = "    imul    eax, ebx\n\0";
                compiler_append_text(c, mul, strlen(mul)); 
            } else if (*b->op.start == '/') {
                char* cdq = "    cdq\n\0";
                compiler_append_text(c, cdq, strlen(cdq));
                char* div = "    idiv    ebx\n\0";
                compiler_append_text(c, div, strlen(div)); 
            }
            char* push_op = "    push    eax\n\0";
            compiler_append_text(c, push_op, strlen(push_op));
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

struct Node *parse_expr(struct Parser *p);

struct Node *parse_group(struct Parser *p) {
    parser_consume(p); //TODO: left paren - need to make a parser_next() - peek looks, next takes, consume has second argument and expects something
    struct Node* n = parse_expr(p);
    parser_consume(p); //TODO: right paren
    return n;
}

struct Node* parse_literal(struct Parser *p) {
    struct Token next = parser_peek(p);
    if (next.type == T_L_PAREN) {
        return parse_group(p);
    } else {
        return node_literal(parser_consume(p));
    }
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

struct Node* parse_expr(struct Parser *p) {
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

bool is_digit(char c) {
    return '0' <= c && c <= '9';
}

struct Token lexer_read_number(struct Lexer *l) {
    struct Token t;
    t.start = &l->code[l->current];
    t.len = 1;
    bool has_decimal = *t.start == '.';

    while (1) {
        char next = l->code[l->current + t.len];
        if (is_digit(next)) {
            t.len++;
        } else if (next == '.') {
            t.len++;
            if (has_decimal) {
                printf("Parser error!  Too many decimals!\n"); //TODO: Need to have real error message here
                exit(1);    
            } else {
                has_decimal = true;
            }
        } else {
            break;
        }
    }

    t.type = has_decimal ? T_FLOAT : T_INT;
    return t;
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
        default:
            t = lexer_read_number(l);
            break;
    }
    l->current += t.len;
    return t;
}

/*
 * Type Checker
 */

enum TokenType type_check(struct Node* n) {
    enum TokenType t;
    switch(n->type) {
        case NODE_LITERAL: {
            struct NodeLiteral *l = (struct NodeLiteral*)n;
            t = l->value.type;
            break;
        }
        case NODE_UNARY: {
            struct NodeUnary *u = (struct NodeUnary*)n;
            t = type_check(u->right);
            break;
        }
        case NODE_BINARY: {
            struct NodeBinary* b = (struct NodeBinary*)n;
            enum TokenType left_type = type_check(b->left);
            enum TokenType right_type = type_check(b->right);
            if (left_type != right_type) {
                printf("Left and right types don't match!\n");
                exit(0); //TODO: Need to display static type checker error message
            }
            t = left_type;
            break;
        }
        default:
            t = T_EOF;
            break;
    }
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

    /*
    for (int i = 0; i < ta.count; i++) {
        printf("%.*s\n", ta.tokens[i].len, ta.tokens[i].start);
    }*/

    //Parse tokens into AST
    struct Parser p;
    parser_init(&p, &ta);
    struct NodeArray na;
    na_init(&na);
    while (parser_peek(&p).type != T_EOF) {
        na_add(&na, parse_expr(&p));
    }

    /*
    for (int i = 0; i < na.count; i++) {
        ast_print(na.nodes[i]);
        printf("\n");
    }*/
    
    //Static Type checking
    for (int i = 0; i < na.count; i++) {
        type_check(na.nodes[i]);
      //  printf("\n");
    }

    //Compile into x64 assembly
    struct Compiler c;
    compiler_init(&c);
    compiler_compile(&c, na.nodes[0]);
    //compiler_append_text(&c, "mov\n", 4);
    //compiler_append_data(&c, "const\n", 6);
    compiler_output_assembly(&c);


    //Assemble into machine code using nasm (or similar) and gcc

    return 0;
}
