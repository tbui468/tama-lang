#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdarg.h>
#include <stdint.h>

#define MAX_MSG_LEN 256

#include "memory.h"
#include "ast.h"
#include "token.h"
#include "byte_array.h"
#include "assembler.h"
#include "error.h"

extern size_t allocated;

/*
 * Tokens
 */

struct TokenArray {
    struct Token *tokens;
    int count;
    int max_count;
};


struct VarData {
    struct Token var;
    struct Token type;
    int bp_offset;
};

struct VarData vd_create(struct Token var, struct Token type) {
    struct VarData vd;
    vd.var = var;
    vd.type = type;
    return vd;
}

struct VarDataArray {
    struct VarData *vds;
    int count;
    int max_count;
    struct VarDataArray* next;
};


struct VarData* vda_get_local(struct VarDataArray *vda, struct Token var);


/*
 * Compiler
 */

void vda_init(struct VarDataArray *vda) {
    vda->vds = NULL;
    vda->count = 0;
    vda->max_count = 0;
    vda->next = NULL;
}

void vda_free(struct VarDataArray *vda) {
    free_arr(vda->vds, sizeof(struct VarData), vda->max_count);
}

void vda_add(struct VarDataArray *vda, struct VarData vd) {
    int old_max = vda->max_count;
    if (vda->max_count == 0) {
        vda->max_count = 8;
    } else if (vda->count + 1 > vda->max_count) {
        vda->max_count *= 2;
    }
    vda->vds = alloc_arr(vda->vds, sizeof(struct VarData), old_max, vda->max_count);

    vda->vds[vda->count] = vd;
    vda->count++;
}

struct VarData* vda_get_local(struct VarDataArray *vda, struct Token var) {
    for (int i = 0; i < vda->count; i++) {
        struct VarData *vd = &(vda->vds[i]);
        if (strncmp(vd->var.start,  var.start, var.len) == 0) {
            return vd;
        }
    }
    
    return NULL;
}

struct Compiler {
    struct ByteArray text;
    struct ByteArray data;
    int data_offset; //in bytes TODO: Not using this, are we?
    struct VarDataArray *head;
    unsigned conditional_label_id; //used to create unique ids for labels in assembly code
};



void compiler_init(struct Compiler *c) {
    ba_init(&c->text);
    ba_init(&c->data);
    c->data_offset = 0;
    struct VarDataArray *head = alloc_unit(sizeof(struct VarDataArray));
    vda_init(head);
    c->head = head;
    c->conditional_label_id = 0;
}

void compiler_free(struct Compiler *c) {
    ba_free(&c->text);
    ba_free(&c->data);
    vda_free(c->head);
    free_unit(c->head, sizeof(struct VarDataArray));
}

void compiler_decl_local(struct Compiler *c, struct Token var, struct Token type) {
    struct VarData vd;
    vd.var = var;
    vd.type = type;
    int offset = 0;

    struct VarDataArray *current = c->head;
    while (current) {
        offset += current->count;
        current = current->next;
    }

    vd.bp_offset = offset;
    vda_add(c->head, vd);
}

struct VarData* compiler_get_local(struct Compiler* c, struct Token var) {
    struct VarData* vd = NULL;

    struct VarDataArray *current = c->head;
    while (current) {
        vd = vda_get_local(current, var);
        if (vd) {
            break;
        } else {
            current = current->next;
        }
    }

    return vd;
}


void compiler_begin_scope(struct Compiler *c) {
    struct VarDataArray *scope = alloc_unit(sizeof(struct VarDataArray));
    vda_init(scope);
    scope->next = c->head;
    c->head = scope;
}

int compiler_end_scope(struct Compiler *c) {
    struct VarDataArray *scope = c->head;
    c->head = scope->next;
    int len = scope->count;
    vda_free(scope);
    free_unit(scope, sizeof(struct VarDataArray));
    return len;
}


//write out assembly file
void compiler_output_assembly(struct Compiler *c) {
    FILE *f = fopen("out.asm", "w");
    char *s = //"section     .text\n"
              //"global      _start\n"
              //"%include \"../../assembly_test/fun.asm\""
              //"\n"
              "org     0x08048000\n"
              "_start:\n";
              //"    mov     ebp, esp\n"; //need to set the frame pointer before doing anything else
    char *e = "\n"
              //"    xor     ebx, ebx\n" //ebx holds the return value of exit() system call 0x1
              "    pop     ebx\n"       //temporary popping of stack into ebx to test (since printing won't work right now)
              "    mov     eax, 0x01\n" //assembler doesn't recognize hex right now
              "    int     0x80\n"
              "\n";
  //            "section     .data\n";
    fwrite(s, sizeof(uint8_t), strlen(s), f);
    fwrite(c->text.bytes, sizeof(uint8_t), c->text.count, f);
    fwrite(e, sizeof(uint8_t), strlen(e), f);
    fwrite(c->data.bytes, sizeof(uint8_t), c->data.count, f);
    fclose(f);
}

void write_op(struct Compiler *c, const char* format, ...) {
    va_list ap;
    va_start(ap, format);
    char s[MAX_MSG_LEN];
    int n = vsprintf(s, format, ap);
    va_end(ap);
    s[n] = '\n';
    s[n + 1] = '\0';
    ba_append(&c->text, (uint8_t*)s, strlen(s));
}

enum TokenType compiler_compile(struct Compiler *c, struct Node *n) {
    enum TokenType ret_type;
    switch (n->type) {
        case NODE_LITERAL: {
            struct NodeLiteral* l = (struct NodeLiteral*)n;

            switch (l->value.type) {
                case T_INT:
                    ret_type = T_INT_TYPE;
                    break;
                case T_TRUE:
                case T_FALSE:
                    ret_type = T_BOOL_TYPE;
                    break;
                default:
                    ret_type = T_NIL_TYPE;
                    break;
            }

            if (ret_type == T_INT_TYPE) {
                write_op(c, "    push    %.*s", l->value.len, l->value.start);
            } else if (l->value.type == T_TRUE) {
                write_op(c, "    push    %d", 1);
            } else if (l->value.type == T_FALSE) {
                write_op(c, "    push    %d", 0);
            }
            break;
        }
        case NODE_UNARY: {
            struct NodeUnary *u = (struct NodeUnary*)n;

            ret_type = compiler_compile(c, u->right);

            write_op(c, "    pop     %s", "eax");

            if (ret_type == T_INT_TYPE) {
                write_op(c, "    neg     %s", "eax");
            } else if (ret_type == T_BOOL_TYPE) {
                write_op(c, "    xor     %s, %d", "eax", 1); 
            }

            write_op(c, "    push    %s", "eax");
            break;
        }
        case NODE_BINARY: {
            struct NodeBinary* b = (struct NodeBinary*)n;

            enum TokenType left_type = compiler_compile(c, b->left);
            enum TokenType right_type = compiler_compile(c, b->right);
            if (left_type != right_type) {
                ems_add(&ems, b->op.line, "Type Error: Left and right types don't match!");
                ret_type = T_NIL_TYPE; //TODO: Should be error type to avoid same error message cascade 
            } else if (b->op.type == T_PLUS || b->op.type == T_MINUS || b->op.type == T_SLASH || b->op.type == T_STAR) {
                ret_type = T_INT_TYPE;
            } else {
                ret_type = T_BOOL_TYPE;
            }

            write_op(c, "    pop     %s", "ebx");
            write_op(c, "    pop     %s", "eax");

            //TODO: use switch statement on b->op.type instead of a bunch of elifs
            if (*b->op.start == '+') {
                write_op(c, "    add     %s, %s", "eax", "ebx");
            } else if (*b->op.start == '-') {
                write_op(c, "    sub     %s, %s", "eax", "ebx");
            } else if (*b->op.start == '*') {
                write_op(c, "    imul    %s, %s", "eax", "ebx");
            } else if (*b->op.start == '/') {
                write_op(c, "    cdq"); //sign extend eax into edx
                write_op(c, "    idiv    %s", "ebx");
            } else if (b->op.type == T_LESS) {
                write_op(c, "    cmp     %s, %s", "eax", "ebx");
                write_op(c, "    setl    %s", "al"); //set byte to 0 or 1
                write_op(c, "    movzx   %s, %s", "eax", "al");
            } else if (b->op.type == T_GREATER) {
                write_op(c, "    cmp     %s, %s", "eax", "ebx");
                write_op(c, "    setg    %s", "al"); //set byte to 0 or 1
                write_op(c, "    movzx   %s, %s", "eax", "al");
            } else if (b->op.type == T_LESS_EQUAL) {
                write_op(c, "    cmp     %s, %s", "eax", "ebx");
                write_op(c, "    setle   %s", "al"); //set byte to 0 or 1
                write_op(c, "    movzx   %s, %s", "eax", "al");
            } else if (b->op.type == T_GREATER_EQUAL) {
                write_op(c, "    cmp     %s, %s", "eax", "ebx");
                write_op(c, "    setge   %s", "al");; //set byte to 0 or 1
                write_op(c, "    movzx   %s, %s", "eax", "al");
            } else if (b->op.type == T_EQUAL_EQUAL) {
                write_op(c, "    cmp     %s, %s", "eax", "ebx");
                write_op(c, "    sete    %s", "al"); //set byte to 0 or 1
                write_op(c, "    movzx   %s, %s", "eax", "al");
            } else if (b->op.type == T_NOT_EQUAL) {
                write_op(c, "    cmp     %s, %s", "eax", "ebx");
                write_op(c, "    setne   %s", "al"); //set byte to 0 or 1
                write_op(c, "    movzx   %s, %s", "eax", "al");
            } else if (b->op.type == T_AND) {
                write_op(c, "    and     %s, %s", "eax", "ebx");
            } else if (b->op.type == T_OR) {
                write_op(c, "    or      %s, %s", "eax", "ebx");
            }
            write_op(c, "    push    %s", "eax");
            break;
        }
        case NODE_EXPR_STMT: {
            struct NodeExprStmt *es = (struct NodeExprStmt*)n;

            enum TokenType expr_type = compiler_compile(c, es->expr);
            expr_type = expr_type; //silencing warning of unused expr_type
            ret_type = T_NIL_TYPE;

            write_op(c, "    pop     %s", "ebx");
            break;
        }
        case NODE_PRINT: {
            struct NodePrint *np = (struct NodePrint*)n;

            enum TokenType arg_type = compiler_compile(c, np->arg);
            ret_type = T_NIL_TYPE;

            if (arg_type == T_INT_TYPE) {
                write_op(c, "    call    %s", "_print_int");
            } else if (arg_type == T_BOOL_TYPE) {
                write_op(c, "    call    %s", "_print_bool");
            }
            write_op(c, "    add     %s, %d", "esp", 4);

            write_op(c, "    push    %s", "0xa");
            write_op(c, "    call    %s", "_print_char");
            write_op(c, "    add     %s, %d", "esp", 4);
            break;
        }
        case NODE_DECL_VAR: {
            struct NodeDeclVar *dv = (struct NodeDeclVar*)n;

            enum TokenType right_type = compiler_compile(c, dv->expr);
            if (right_type != dv->type.type) {
                ems_add(&ems, dv->var.line, "Type Error: Declaration type and assigned value type don't match!");
            }
            if (vda_get_local(c->head, dv->var)) {
                ems_add(&ems, dv->var.line, "Type Error: Variable already declared!");
            }
            compiler_decl_local(c, dv->var, dv->type);
            ret_type = right_type;

            //local variable is on stack at this point
            break;
        }
        case NODE_GET_VAR: {
            struct NodeGetVar *gv = (struct NodeGetVar*)n;

            struct VarData* vd = compiler_get_local(c, gv->var);
            if (!vd) {
                ems_add(&ems, gv->var.line, "Type Error: Variable not declared!");
                ret_type = T_NIL_TYPE; //TODO: Should be error type to avoid multiple error messages
            } else {
                ret_type = vd->type.type;
            }

            write_op(c, "    mov     %s, [%s - %d]", "eax", "ebp", 4 * (vd->bp_offset + 1));
            write_op(c, "    push    %s", "eax");
            break;
        }
        case NODE_SET_VAR: {
            struct NodeSetVar *sv = (struct NodeSetVar*)n;

            struct VarData* vd = compiler_get_local(c, sv->var);
            if (!vd) {
                ems_add(&ems, sv->var.line, "Type Error: Variable not declared!");
                ret_type = T_NIL_TYPE; //TODO: should have special error type to avoid repeated error messages
            } else {
                enum TokenType assigned_type = compiler_compile(c, sv->expr);
                if (vd->type.type != assigned_type) {
                    ems_add(&ems, sv->var.line, "Type Error: Declaration type and assigned value type don't match!");
                    ret_type = T_NIL_TYPE; //TODO: should have special error type to avoid repeated type error messages
                } else {
                    ret_type = assigned_type;
                }
            }

            write_op(c, "    pop     %s", "eax");
            write_op(c, "    mov     [%s - %d], %s", "ebp", 4 * (vd->bp_offset + 1), "eax");
            write_op(c, "    push    %s", "eax");
            break;
        }
        case NODE_BLOCK: {
            struct NodeBlock *b = (struct NodeBlock*)n;
            compiler_begin_scope(c);
            for (int i = 0; i < b->stmts->count; i++) {
                compiler_compile(c, b->stmts->nodes[i]);
            }
            int pop_count = compiler_end_scope(c);
            for (int i = 0; i < pop_count; i++) {
                write_op(c, "    pop     %s", "eax");
            }
            ret_type = T_NIL_TYPE;
            break;
        }
        case NODE_IF: {
            struct NodeIf *i = (struct NodeIf*)n;
            enum TokenType condition_type = compiler_compile(c, i->condition);
            if (condition_type != T_BOOL_TYPE) {
                ems_add(&ems, i->if_token.line, "Type Error: 'if' keyword must be followed by boolean expression.");
            }

            write_op(c, "    pop     %s", "eax");
            write_op(c, "    cmp     %s, %d", "eax", 0);
            write_op(c, "    je      else_block%d", c->conditional_label_id);

            compiler_compile(c, i->then_block);
            write_op(c, "    jmp     if_end%d", c->conditional_label_id);

            write_op(c, "else_block%d:", c->conditional_label_id);
            if (i->else_block) {
                compiler_compile(c, i->else_block);
            }
            write_op(c, "if_end%d:", c->conditional_label_id);

            c->conditional_label_id++;
            ret_type = T_NIL_TYPE;
            break;
        }
        case NODE_WHILE: {
            struct NodeWhile *w = (struct NodeWhile*)n;
            write_op(c, "    jmp     %s%d", "while_condition", c->conditional_label_id);
            write_op(c, "%s%d:", "while_block", c->conditional_label_id);
            compiler_compile(c, w->while_block);
            write_op(c, "%s%d:", "while_condition", c->conditional_label_id);
            enum TokenType condition_type = compiler_compile(c, w->condition);
            if (condition_type != T_BOOL_TYPE) {
                ems_add(&ems, w->while_token.line, "Type Error: 'while' keyword must be followed by boolean expression.");
            }
            write_op(c, "    pop     %s", "eax");
            write_op(c, "    cmp     %s, %d", "eax", 1);
            write_op(c, "    je      while_block%d", c->conditional_label_id);

            c->conditional_label_id++;
            ret_type = T_NIL_TYPE;
            break;
        }
        default:
            printf("Node type not recognized\n");
            break;
    }
    
    return ret_type;
}



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
