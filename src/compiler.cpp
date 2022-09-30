#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

#include "compiler.hpp"
#include "memory.hpp"
#include "error.hpp"

#define MAX_MSG_LEN 256

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
    vda->vds = (struct VarData*)alloc_arr(vda->vds, sizeof(struct VarData), old_max, vda->max_count);

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

void compiler_init(struct Compiler *c) {
    ba_init(&c->text);
    ba_init(&c->data);
    c->data_offset = 0;
    struct VarDataArray *head = (struct VarDataArray*)alloc_unit(sizeof(struct VarDataArray));
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
    struct VarDataArray *scope = (struct VarDataArray*)alloc_unit(sizeof(struct VarDataArray));
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



