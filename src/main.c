#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdarg.h>
#include <stdint.h>

#include "memory.h"
#include "ast.h"
#include "token.h"
#include "byte_array.h"
#include "assembler.h"
#include "error.h"
#include "compiler.h"

#define MAX_MSG_LEN 256

extern size_t allocated;

/*
 * Tokens
 */

struct TokenArray {
    struct Token *tokens;
    int count;
    int max_count;
};


/*
 *  Parser
 */

struct Parser {
    struct TokenArray *ta;
    int current;
};

struct Token parser_peek_two(struct Parser *p) {
    int next = p->current + 1;
    if (next >= p->ta->count)
        next = p->ta->count - 1;
    return p->ta->tokens[next];
}

struct Token parser_peek_one(struct Parser *p) {
    int next = p->current;
    if (next >= p->ta->count)
        next = p->ta->count - 1;
    return p->ta->tokens[next];
}

struct Token parser_next(struct Parser *p) {
    int next = p->current;
    if (next >= p->ta->count)
        next = p->ta->count - 1;
    struct Token ret = p->ta->tokens[next];
    p->current++;
    return ret;
}

struct Token parser_consume(struct Parser *p, enum TokenType tt) {
    struct Token t = parser_next(p);
    if (t.type != tt) {
        ems_add(&ems, t.line, "Unexpected token!");
    }
    return t;
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
    parser_consume(p, T_L_PAREN);
    struct Node* n = parse_expr(p);
    parser_consume(p, T_R_PAREN);
    return n;
}

struct Node* parse_literal(struct Parser *p) {
    struct Token next = parser_peek_one(p);
    if (next.type == T_L_PAREN) {
        return parse_group(p);
    } else if (next.type == T_IDENTIFIER) {
        return node_get_var(parser_next(p));
    } else if (next.type == T_INT) {
        return node_literal(parser_next(p));
    } else if (next.type == T_TRUE) {
        return node_literal(parser_next(p));
    } else if (next.type == T_FALSE) {
        return node_literal(parser_next(p));
    } else {
        ems_add(&ems, next.line, "Parse Error: Unexpected token.");
        return node_literal(parser_next(p));
    }
}

struct Node* parse_unary(struct Parser *p) {
    struct Token next = parser_peek_one(p);
    if (next.type == T_MINUS || next.type == T_NOT) {
        struct Token op = parser_next(p);
        return node_unary(op, parse_unary(p));
    } else {
        return parse_literal(p);
    }
}

struct Node* parse_mul_div(struct Parser *p) {
    struct Node *left = parse_unary(p);

    while (1) {
        struct Token next = parser_peek_one(p);
        if (next.type != T_STAR && next.type != T_SLASH)
            break;

        struct Token op = parser_next(p);
        struct Node *right = parse_unary(p);
        left = node_binary(left, op, right);
    }

    return left;
}

struct Node* parse_add_sub(struct Parser *p) {
    struct Node *left = parse_mul_div(p);

    while (1) {
        struct Token next = parser_peek_one(p);
        if (next.type != T_MINUS && next.type != T_PLUS)
            break;

        struct Token op = parser_next(p);
        struct Node *right = parse_mul_div(p);
        left = node_binary(left, op, right);
    }

    return left;
}

struct Node* parse_inequality(struct Parser *p) {
    struct Node *left = parse_add_sub(p);

    while (1) {
        struct Token next = parser_peek_one(p);
        if (next.type != T_LESS && next.type != T_LESS_EQUAL &&
            next.type != T_GREATER && next.type != T_GREATER_EQUAL) {
            break;
        }

        struct Token op = parser_next(p);
        struct Node *right = parse_add_sub(p);
        left = node_binary(left, op, right);
    }

    return left;
}

struct Node *parse_equality(struct Parser *p) {
    struct Node *left = parse_inequality(p);

    while (1) {
        struct Token next = parser_peek_one(p);
        if (next.type != T_EQUAL_EQUAL && next.type != T_NOT_EQUAL) {
            break;
        }

        struct Token op = parser_next(p);
        struct Node *right = parse_inequality(p);
        left = node_binary(left, op, right);
    }

    return left;
}

struct Node *parse_and(struct Parser *p) {
    struct Node *left = parse_equality(p);

    while (1) {
        struct Token next = parser_peek_one(p);
        if (next.type != T_AND) {
            break;
        }

        struct Token op = parser_next(p);
        struct Node *right = parse_equality(p);
        left = node_binary(left, op, right);
    }

    return left;
}

struct Node *parse_or(struct Parser *p) {
    struct Node *left = parse_and(p);

    while (1) {
        struct Token next = parser_peek_one(p);
        if (next.type != T_OR) {
            break;
        }

        struct Token op = parser_next(p);
        struct Node *right = parse_and(p);
        left = node_binary(left, op, right);
    }

    return left;
}

struct Node* parse_assignment(struct Parser *p) {
    if (parser_peek_one(p).type == T_IDENTIFIER && parser_peek_two(p).type == T_EQUAL) {
        struct Token var = parser_consume(p, T_IDENTIFIER);
        parser_consume(p, T_EQUAL);
        struct Node *expr = parse_expr(p);
        return node_set_var(var, expr);
    } else {
        return parse_or(p);
    }
}

struct Node* parse_expr(struct Parser *p) {
    return parse_assignment(p); 
}

struct Node *parse_stmt(struct Parser *p);

struct Node* parse_block(struct Parser *p) {
    struct Token l_brace = parser_consume(p, T_L_BRACE);
    struct NodeArray* na = alloc_unit(sizeof(struct NodeArray));
    na_init(na);
    while (parser_peek_one(p).type != T_R_BRACE && parser_peek_one(p).type != T_EOF) {
        struct Node* stmt = parse_stmt(p);
        na_add(na, stmt);
    }
    struct Token r_brace = parser_consume(p, T_R_BRACE);
    return node_block(l_brace, r_brace, na);
}

struct Node *parse_stmt(struct Parser *p) {
    struct Token next = parser_peek_one(p);
    if (next.type == T_PRINT) {
        parser_next(p);
        parser_consume(p, T_L_PAREN);
        struct Node* arg = parse_expr(p);
        parser_consume(p, T_R_PAREN);
        return node_print(arg);
    } else if (next.type == T_IDENTIFIER && parser_peek_two(p).type == T_COLON) {
        struct Token var = parser_consume(p, T_IDENTIFIER);
        parser_consume(p, T_COLON);
        struct Token type = parser_next(p);
        parser_consume(p, T_EQUAL);
        struct Node *expr = parse_expr(p);
        return node_decl_var(var, type, expr);
    } else if (next.type == T_L_BRACE) {
        return parse_block(p);
    } else if (next.type == T_IF) {
        struct Token if_token = parser_next(p);
        struct Node* condition = parse_expr(p);
        struct Node* then_block = parse_block(p);
        struct Node *else_block = NULL;
        if (parser_peek_one(p).type == T_ELSE) {
            parser_next(p);
            else_block = parse_block(p);
        }
        return node_if(if_token, condition, then_block, else_block);
    } else if (next.type == T_WHILE) {
        struct Token while_token = parser_next(p);
        struct Node* condition = parse_expr(p);
        struct Node* while_block = parse_block(p);
        return node_while(while_token, condition, while_block);
    } else {
        return node_expr_stmt(parse_expr(p));
    }
}

//struct Node *aparse_deref(struct Parser *p) {
//  NOTE: should reach this point if T_L_BRACKET is encountered
//}

struct Node *aparse_literal(struct Parser *p) {
    struct Token next = parser_next(p);
    switch (next.type) {
        case T_INT:
        case T_HEX:
            return anode_imm(next);
        case T_EAX:
        case T_ECX:
        case T_EDX:
        case T_EBX:
        case T_ESP:
        case T_EBP:
        case T_ESI:
        case T_EDI:
            return anode_reg(next);
        case T_IDENTIFIER:
            return anode_label_ref(next);
        default:
            ems_add(&ems, next.line, "Parse Error: Unrecognized token!");
    }
    return NULL;
}

struct Node *aparse_expr(struct Parser *p) {
    return aparse_literal(p);
}

struct Node *aparse_stmt(struct Parser *p) {
    struct Token next = parser_peek_one(p);
    //if identifer followed by a colon, then it's a label
    if (next.type == T_IDENTIFIER && parser_peek_two(p).type == T_COLON) {
        struct Token id = parser_next(p);
        parser_consume(p, T_COLON);
        return anode_label_def(id);
    } else {
        struct Token op =  parser_next(p);
        struct Node *left = NULL;
        struct Node *right = NULL;
        switch (next.type) {
            //two operands
            case T_MOV:
            case T_ADD:
            case T_SUB:
            case T_IMUL:
            case T_XOR:
                left = aparse_expr(p);
                parser_consume(p, T_COMMA);
                right = aparse_expr(p);
                break;
            //single operand
            case T_POP:
            case T_PUSH:
            case T_INTR:
            case T_ORG:
            case T_IDIV:
                left = aparse_expr(p);
                break;
            //no operands
            case T_CDQ:
                break;
            default:
                ems_add(&ems, next.line, "AParse Error: Invalid token type!");
        }
        return anode_op(op, left, right);
    }
}

/*
 * Lexer
 */

enum SyntaxType {
    ST_TMD,
    ST_ASM
};

struct Lexer {
    char* code;
    int current;
    int line;
    enum SyntaxType st;
};

void ta_init(struct TokenArray *ta) {
    ta->tokens = NULL;
    ta->count = 0;
    ta->max_count = 0;
}

void ta_free(struct TokenArray *ta) {
    free_arr(ta->tokens, sizeof(struct Token), ta->max_count); 
}

void ta_add(struct TokenArray *ta, struct Token t) {
    int old_max = ta->max_count;
    if (ta->max_count == 0) {
        ta->max_count = 8;
    }else if (ta->count + 1 > ta->max_count) {
        ta->max_count *= 2;
    }
    ta->tokens = alloc_arr(ta->tokens, sizeof(struct Token), old_max, ta->max_count);

    ta->tokens[ta->count] = t;
    ta->count++;
}

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

char *load_code(char* filename) {
    FILE *f = fopen(filename, "r");
    fseek(f, 0, SEEK_END);
    long fsize = ftell(f);
    fseek(f, 0, SEEK_SET); //rewind(f);
    char *string = malloc(fsize + 1);
    fread(string, fsize, 1, f);
    fclose(f);
    string[fsize] = 0;
    return string;
}



int main (int argc, char **argv) {

    ems_init(&ems);

    if (argc < 2) {
        printf("Usage: tama <filename>\n");
        exit(1);
    }
    char *code = load_code(argv[1]);

    //Tokenize source code
    struct Lexer l;
    lexer_init(&l, code, ST_TMD); //ST_TMD or ST_ASM

    struct TokenArray ta;
    ta_init(&ta);

    while (l.current < (int)strlen(l.code)) {
        struct Token t = lexer_next_token(&l);
        if (t.type != T_NEWLINE)
            ta_add(&ta, t);
        else
            l.line++;
    }

    struct Token t;
    t.type = T_EOF;
    t.start = NULL;
    t.len = 0;
    t.line = l.line;
    ta_add(&ta, t);

    for (int i = 0; i < ta.count; i++) {
//        printf("[%d] %.*s\n", i, ta.tokens[i].len, ta.tokens[i].start);
    }


    //Parse tokens into AST
    struct Parser p;
    parser_init(&p, &ta);
    struct NodeArray na;
    na_init(&na);
    
    while (parser_peek_one(&p).type != T_EOF) {
        na_add(&na, parse_stmt(&p));
    }


    for (int i = 0; i < na.count; i++) {
//        ast_print(na.nodes[i]);
//        printf("\n");
    }

    
    struct Compiler c;
    compiler_init(&c);

    compiler_begin_scope(&c);
    for (int i = 0; i < na.count; i++) {
        compiler_compile(&c, na.nodes[i]);
    }
    compiler_end_scope(&c);

    if (ems.count <= 0) {
        compiler_output_assembly(&c);
    }


    //Tokenize assembly
    char *acode = load_code("out.asm");
    struct Lexer al;
    lexer_init(&al, acode, ST_ASM); //ST_TMD or ST_ASM

    struct TokenArray ata;
    ta_init(&ata);

    while (al.current < (int)strlen(al.code)) {
        struct Token t = lexer_next_token(&al);
        if (t.type != T_NEWLINE)
            ta_add(&ata, t);
        else
            al.line++;
    }

    struct Token at;
    at.type = T_EOF;
    at.start = NULL;
    at.len = 0;
    at.line = al.line;
    ta_add(&ata, at);

    //parse assembly
    struct Parser ap;
    parser_init(&ap, &ata);
    struct NodeArray ana;
    na_init(&ana);
    
    while (parser_peek_one(&ap).type != T_EOF) {
        na_add(&ana, aparse_stmt(&ap));
    }

    for (int i = 0; i < ana.count; i++) {
        //ast_print(ana.nodes[i]);
        //printf("\n");
    }

    //assemble
    struct Assembler a;
    assembler_init(&a);

    assembler_append_elf_header(&a);
    assembler_append_program_header(&a);  
    assembler_append_program(&a, &ana);
    assembler_patch_locations(&a);
    assembler_patch_labels(&a);
    
    assembler_write_binary(&a, "out.bin");


   
    ems_print(&ems);
    
    //cleanup
    printf("Memory allocated: %ld\n", allocated);
    compiler_free(&c);
    na_free(&na);
    ta_free(&ta);
    na_free(&ana);
    ta_free(&ata);
    assembler_free(&a);
    ems_free(&ems);
    printf("Allocated memory remaining: %ld\n", allocated);

    return 0;
}
