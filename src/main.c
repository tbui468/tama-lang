#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdarg.h>

#define MAX_MSG_LEN 256


size_t allocated;


void* alloc_arr(void *vptr, size_t unit_size, int old_count, int new_count) {
    allocated += unit_size * (new_count - old_count);
    void* ret;
    
    if (!(ret = realloc(vptr, unit_size * new_count))) {
        printf("System Error: realloc failed\n");
        exit(1);
    }

    return ret;
}

void* alloc_unit(size_t unit_size) {
    return alloc_arr(NULL, unit_size, 0, 1);
}

void free_arr(void *vtpr, size_t unit_size, int count) {
    free(vtpr);
    allocated -= unit_size * count;
}

void free_unit(void *vptr, size_t unit_size) {
    free_arr(vptr, unit_size, 1);
}


enum TokenType {
    T_INT,
    T_FLOAT,
    T_PLUS,
    T_MINUS,
    T_STAR,
    T_SLASH,
    T_L_PAREN,
    T_R_PAREN,
    T_NEWLINE,
    T_PRINT,
    T_NIL,
    T_EOF,
    T_COLON,
    T_INT_TYPE,
    T_NIL_TYPE,
    T_EQUAL,
    T_IDENTIFIER,
    T_TRUE,
    T_FALSE,
    T_BOOL_TYPE,
    T_L_BRACE,
    T_R_BRACE,
    T_LESS,
    T_GREATER,
    T_LESS_EQUAL,
    T_GREATER_EQUAL,
    T_EQUAL_EQUAL,
    T_NOT_EQUAL,
    T_NOT,
    T_AND,
    T_OR,
    T_IF,
    T_ELIF,
    T_ELSE,
    T_WHILE
};

struct Token {
    enum TokenType type;
    char *start;
    int len; 
    int line;
};

/*
 * Errors
 */

struct Error {
    char* msg;
    int line;
};


struct ErrorMsgs {
    struct Error *errors;
    int count;
    int max_count;
};

struct ErrorMsgs ems;

void ems_init(struct ErrorMsgs *ems) {
    ems->errors = NULL;
    ems->count = 0;
    ems->max_count = 0;
}

void ems_free(struct ErrorMsgs *ems) {
    for (int i = 0; i < ems->count; i++) {
        free_unit(ems->errors[i].msg, MAX_MSG_LEN);
    }
    free_arr(ems->errors, sizeof(struct Error), ems->max_count);
}

void ems_add(struct ErrorMsgs *ems, int line, char* format, ...) {
    if (ems->count + 1 > ems->max_count) {
        int old_max = ems->max_count;
        if (ems->max_count == 0) {
            ems->max_count = 8;
        } else {
            ems->max_count *= 2;
        }
        ems->errors = alloc_arr(ems->errors, sizeof(struct Error), old_max, ems->max_count);
    }

    va_list ap;
    va_start(ap, format);
    char* s = alloc_unit(MAX_MSG_LEN);
    int written = snprintf(s, MAX_MSG_LEN, "[%d] ", line);
    vsnprintf(s + written, MAX_MSG_LEN - written, format, ap);
    va_end(ap);

    struct Error e;
    e.msg = s;
    e.line = line;

    ems->errors[ems->count++] = e;
}

void ems_sort(struct ErrorMsgs *ems) {
    for (int end = ems->count - 1; end > 0; end--) {
        for (int i = 0; i < end; i++) {
            struct Error left = ems->errors[i];
            struct Error right = ems->errors[i + 1];
            if (left.line > right.line) {
                ems->errors[i] = right;
                ems->errors[i + 1] = left;
            }
        }
    }
}

void ems_print(struct ErrorMsgs *ems) {
    ems_sort(ems);
    for (int i = 0; i < ems->count; i++) {
        printf("%s\n", ems->errors[i].msg); 
    }
}

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
 * AST Nodes
 */

enum NodeType {
    NODE_BINARY,
    NODE_UNARY,
    NODE_LITERAL,
    NODE_PRINT,
    NODE_EXPR_STMT,
    NODE_DECL_VAR,
    NODE_GET_VAR,
    NODE_SET_VAR,
    NODE_BLOCK,
    NODE_IF,
    NODE_WHILE
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

//TODO: remove after generalizing to NodeCall
struct NodePrint {
    struct Node n;
    struct Node *arg;
};

struct NodeExprStmt {
    struct Node n;
    struct Node *expr;
};

struct NodeDeclVar {
    struct Node n;
    struct Token var;
    struct Token type;
    struct Node *expr;
};

struct NodeGetVar {
    struct Node n;
    struct Token var;
};

struct NodeSetVar {
    struct Node n;
    struct Token var;
    struct Node *expr;
};

struct NodeArray {
    struct Node **nodes;
    int count;
    int max_count;
};

struct NodeBlock {
    struct Node n;
    struct Token l_brace;
    struct Token r_brace;
    struct NodeArray* stmts;
};

struct NodeIf {
    struct Node n;
    struct Token if_token;
    struct Node* condition;
    struct Node* then_block;
    struct Node* else_block;
};

struct NodeWhile {
    struct Node n;
    struct Token while_token;
    struct Node *condition;
    struct Node *while_block;
};

void na_init(struct NodeArray *na) {
    na->nodes = NULL;
    na->count = 0;
    na->max_count = 0;
}

void ast_free(struct Node* n);


void na_free(struct NodeArray *na) {
    for (int i = 0; i < na->count; i++) {
        ast_free(na->nodes[i]); 
    }
    free_arr(na->nodes, sizeof(struct Node*), na->max_count);
}

void na_add(struct NodeArray *na, struct Node* n) {
    int old_max = na->max_count;
    if (na->max_count == 0) {
        na->max_count = 8;
    } else if (na->count + 1 > na->max_count) {
        na->max_count *= 2;
    }
    na->nodes = alloc_arr(na->nodes, sizeof(struct Node*), old_max, na->max_count);

    na->nodes[na->count++] = n;
}

struct Node* node_literal(struct Token value) {
    struct NodeLiteral *node = alloc_unit(sizeof(struct NodeLiteral));
    node->n.type = NODE_LITERAL;
    node->value = value;
    return (struct Node*)node;
}

struct Node* node_unary(struct Token op, struct Node* right) {
    struct NodeUnary *node = alloc_unit(sizeof(struct NodeUnary));
    node->n.type = NODE_UNARY;
    node->op = op;
    node->right = right;
    return (struct Node*)node;
}

struct Node* node_binary(struct Node* left, struct Token op, struct Node* right) {
    struct NodeBinary *node = alloc_unit(sizeof(struct NodeBinary));
    node->n.type = NODE_BINARY;
    node->left = left;
    node->op = op;
    node->right = right;
    return (struct Node*)node;
}

struct Node *node_print(struct Node* arg) {
    struct NodePrint *node = alloc_unit(sizeof(struct NodePrint));
    node->n.type = NODE_PRINT;
    node->arg = arg;
    return (struct Node*)node;
}

struct Node *node_expr_stmt(struct Node *expr) {
    struct NodeExprStmt *node = alloc_unit(sizeof(struct NodeExprStmt));
    node->n.type = NODE_EXPR_STMT;
    node->expr = expr;
    return (struct Node*)node;
}

struct Node *node_decl_var(struct Token var, struct Token type, struct Node *expr) {
    struct NodeDeclVar *node = alloc_unit(sizeof(struct NodeDeclVar));
    node->n.type = NODE_DECL_VAR;
    node->var = var;
    node->type = type;
    node->expr = expr;
    return (struct Node*)node;
}

struct Node *node_get_var(struct Token var) {
    struct NodeGetVar *node = alloc_unit(sizeof(struct NodeGetVar));
    node->n.type = NODE_GET_VAR;
    node->var = var;
    return (struct Node*)node;
}

struct Node *node_set_var(struct Token var, struct Node *expr) {
    struct NodeSetVar *node = alloc_unit(sizeof(struct NodeSetVar));
    node->n.type = NODE_SET_VAR;
    node->var = var;
    node->expr = expr;
    return (struct Node*)node;
}

struct Node *node_block(struct Token l_brace, struct Token r_brace, struct NodeArray* na) {
    struct NodeBlock *node = alloc_unit(sizeof(struct NodeBlock));
    node->n.type = NODE_BLOCK;
    node->l_brace = l_brace;
    node->r_brace = r_brace;
    node->stmts = na;
    return (struct Node*)node;
}

struct Node *node_if(struct Token if_token, struct Node *condition, struct Node *then_block, struct Node *else_block) {
    struct NodeIf *node = alloc_unit(sizeof(struct NodeIf));
    node->n.type = NODE_IF;
    node->if_token = if_token;
    node->condition = condition;
    node->then_block = then_block;
    node->else_block = else_block;
    return (struct Node*)node;
}

struct Node *node_while(struct Token while_token, struct Node *condition, struct Node *while_block) {
    struct NodeWhile *node = alloc_unit(sizeof(struct NodeWhile));
    node->n.type = NODE_WHILE;
    node->while_token = while_token;
    node->condition = condition;
    node->while_block = while_block;
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
        case NODE_PRINT: {
            printf("NODE_PRINT");
            break;
        }
        case NODE_EXPR_STMT: {
            printf("NODE_EXPR_STMT");  
            break;
        }
        case NODE_DECL_VAR: {
            printf("NODE_DECL_VAR");  
            break;
        }
        case NODE_GET_VAR: {
            printf("NODE_GET_VAR");  
            break;
        }
        case NODE_SET_VAR: {
            printf("NODE_SET_VAR");  
            break;
        }
        case NODE_BLOCK: {
            printf("NODE_BLOCK");  
            break;
        }
        case NODE_IF: {
            printf("NODE_IF");
            break;
        }
        case NODE_WHILE: {
            printf("NODE_WHILE");
            break;
        }
        default:
            printf("Node type not recognized\n");
            break;
    }
}

void ast_free(struct Node* n) {
    if (!n) return;
    switch (n->type) {
        case NODE_LITERAL: {
            struct NodeLiteral* l = (struct NodeLiteral*)n;
            free_unit(l, sizeof(struct NodeLiteral));
            break;
        }
        case NODE_UNARY: {
            struct NodeUnary *u = (struct NodeUnary*)n;
            ast_free(u->right);
            free_unit(u, sizeof(struct NodeUnary));
            break;
        }
        case NODE_BINARY: {
            struct NodeBinary* b = (struct NodeBinary*)n;
            ast_free(b->left);
            ast_free(b->right);
            free_unit(b, sizeof(struct NodeBinary));
            break;
        }
        case NODE_PRINT: {
            struct NodePrint* np = (struct NodePrint*)n;
            ast_free(np->arg);
            free_unit(np, sizeof(struct NodePrint));
            break;
        }
        case NODE_EXPR_STMT: {
            struct NodeExprStmt* es = (struct NodeExprStmt*)n;
            ast_free(es->expr);
            free_unit(es, sizeof(struct NodeExprStmt));
            break;
        }
        case NODE_DECL_VAR: {
            struct NodeDeclVar *dv = (struct NodeDeclVar*)n;
            ast_free(dv->expr);
            free_unit(dv, sizeof(struct NodeDeclVar));
            break;
        }
        case NODE_GET_VAR: {
            struct NodeGetVar *gv = (struct NodeGetVar*)n;
            free_unit(gv, sizeof(struct NodeGetVar));
            break;
        }
        case NODE_SET_VAR: {
            struct NodeSetVar *sv = (struct NodeSetVar*)n;
            ast_free(sv->expr);
            free_unit(sv, sizeof(struct NodeSetVar));
            break;
        }
        case NODE_BLOCK: {
            struct NodeBlock *b = (struct NodeBlock*)n;
            na_free(b->stmts);
            free_unit(b->stmts, sizeof(struct NodeArray));
            free_unit(b, sizeof(struct NodeBlock));
            break;
        }
        case NODE_IF: {
            struct NodeIf *i = (struct NodeIf*)n;
            ast_free(i->condition);
            ast_free(i->then_block);
            ast_free(i->else_block);
            free_unit(i, sizeof(struct NodeIf));
            break;
        }
        case NODE_WHILE: {
            struct NodeWhile *w = (struct NodeWhile*)n;
            ast_free(w->condition);
            ast_free(w->while_block);
            free_unit(w, sizeof(struct NodeWhile));
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

struct CharArray {
    char* chars;
    int count;
    int max_count;
};

struct Compiler {
    struct CharArray text;
    struct CharArray data;
    int data_offset; //in bytes TODO: Not using this, are we?
    struct VarDataArray *head;
    unsigned conditional_label_id;
};

void ca_init(struct CharArray* ca) {
    ca->chars = NULL;
    ca->count = 0;
    ca->max_count = 0;
}

void ca_free(struct CharArray* ca) {
    free_arr(ca->chars, sizeof(char), ca->max_count);
}

void ca_append(struct CharArray* ca, char* s, int len) {
    int old_max = ca->max_count;
    if (ca->max_count == 0) {
        ca->max_count = 8;
        ca->chars = alloc_arr(ca->chars, sizeof(char), old_max, ca->max_count);
    }

    while (ca->count + len > ca->max_count) {
        old_max = ca->max_count;
        ca->max_count *= 2;
        ca->chars = alloc_arr(ca->chars, sizeof(char), old_max, ca->max_count);
    }

    memcpy(&ca->chars[ca->count], s, len);
    ca->count += len;
}

void compiler_init(struct Compiler *c, struct VarDataArray *head) {
    ca_init(&c->text);
    ca_init(&c->data);
    c->data_offset = 0;
    c->head = head;
    c->conditional_label_id = 0;
}

void compiler_free(struct Compiler *c) {
    ca_free(&c->text);
    ca_free(&c->data);
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
              "_start:\n"
              "    mov     ebp, esp\n";
    char *e = "\n"
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

void write_op(struct Compiler *c, const char* format, ...) {
    va_list ap;
    va_start(ap, format);
    char s[MAX_MSG_LEN];
    int n = vsprintf(s, format, ap);
    va_end(ap);
    s[n] = '\n';
    s[n + 1] = '\0';
    compiler_append_text(c, s, strlen(s));
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
                write_op(c, "    cdq");
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

/*
 * Lexer
 */

struct Lexer {
    char* code;
    int current;
    int line;
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

void lexer_init(struct Lexer *l, char* code) {
    l->code = code;
    l->current = 0;
    l->line = 1;
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

    //TODO: make this a switch to go a lot faster
    //      will need to rework how this checks strings
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
    return t;
}

struct Token lexer_read_number(struct Lexer *l) {
    struct Token t;
    t.start = &l->code[l->current];
    t.len = 1;
    t.line = l->line;
    bool has_decimal = *t.start == '.';

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

    t.type = has_decimal ? T_FLOAT : T_INT;
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

    allocated = 0;
    ems_init(&ems);

    if (argc < 2) {
        printf("Usage: tama <filename>\n");
        exit(1);
    }
    char *code = load_code(argv[1]);

    //Tokenize source code
    struct Lexer l;
    lexer_init(&l, code);

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
//        printf("%.*s\n", ta.tokens[i].len, ta.tokens[i].start);
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

    
    //Could optimize AST at this point?

    //Compile into IA32 (Intel syntax)
    struct VarDataArray vda;
    vda_init(&vda);
    struct Compiler c;
    compiler_init(&c, &vda);

    compiler_begin_scope(&c);
    for (int i = 0; i < na.count; i++) {
        compiler_compile(&c, na.nodes[i]);
    }
    compiler_end_scope(&c);

    if (ems.count <= 0) {
        compiler_output_assembly(&c);
    }


    //Could assemble to object files here?
    
    //Could link here and create executable?
   
    ems_print(&ems);
    
    //cleanup
    printf("Memory allocated: %ld\n", allocated);
    compiler_free(&c);
    vda_free(&vda);
    na_free(&na);
    ta_free(&ta);
    ems_free(&ems);
    printf("Allocated memory remaining: %ld\n", allocated);

    return 0;
}
