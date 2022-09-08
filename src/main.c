#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdarg.h>
#include <stdint.h>

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
    //@Note: order here is necessary for machine code mapping tables
    T_EAX = 0,
    T_ECX,
    T_EDX,
    T_EBX,
    T_ESP,
    T_EBP,
    T_ESI,
    T_EDI,

    //tokenizing tamarind file
    T_INT,
    T_HEX,
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
    T_WHILE,
    T_L_BRACKET,
    T_R_BRACKET,
    T_COMMA,

    //tokening assembly file
    T_MOV,
    T_PUSH,
    T_POP,
    T_ADD,
    T_SUB,
    T_IMUL,
    T_IDIV,
    T_INTR,
    T_DOLLAR,   //Let's just force user to define labels instead of compiling $$
    T_EQU,       //used to compute sizes (should compute the value and then patch immediately)
    T_ORG,
    T_CDQ
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
    NODE_WHILE,

    //for assembler
    ANODE_LABEL_REF,
    ANODE_LABEL_DEF,
    ANODE_OP,
    ANODE_REG,
    ANODE_IMM
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


struct ANodeLabelDef {
    struct Node n; 
    struct Token t;
};

struct ANodeLabelRef {
    struct Node n; 
    struct Token t;
};

struct ANodeOp {
    struct Node n;
    struct Token operator;
    struct Node *operand1;
    struct Node *operand2;
};

struct ANodeReg {
    struct Node n;
    struct Token t;
};

struct ANodeImm {
    struct Node n;
    struct Token t;
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

struct Node *anode_label_def(struct Token t) {
    struct ANodeLabelDef *node = alloc_unit(sizeof(struct ANodeLabelDef));
    node->n.type = ANODE_LABEL_DEF;
    node->t = t;
    return (struct Node*)node;
}

struct Node *anode_label_ref(struct Token t) {
    struct ANodeLabelRef *node = alloc_unit(sizeof(struct ANodeLabelRef));
    node->n.type = ANODE_LABEL_REF;
    node->t = t;
    return (struct Node*)node;
}

struct Node *anode_op(struct Token operator, struct Node *operand1, struct Node *operand2) {
    struct ANodeOp *node = alloc_unit(sizeof(struct ANodeOp));
    node->n.type = ANODE_OP;
    node->operator = operator;
    node->operand1 = operand1;
    node->operand2 = operand2;
    return (struct Node*)node;
}

struct Node *anode_reg(struct Token t) {
    struct ANodeReg *node = alloc_unit(sizeof(struct ANodeReg));
    node->n.type = ANODE_REG;
    node->t = t;
    return (struct Node*)node;
}

struct Node *anode_imm(struct Token t) {
    struct ANodeImm *node = alloc_unit(sizeof(struct ANodeImm));
    node->n.type = ANODE_IMM;
    node->t = t;
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
        case ANODE_LABEL_DEF: {
            printf("ANODE_LABEL_DEF");
            break;
        }
        case ANODE_LABEL_REF: {
            printf("ANODE_LABEL_REF");
            break;
        }
        case ANODE_OP: {
            printf("ANODE_OP");
            break;
        }
        case ANODE_REG: {
            printf("ANODE_REG");
            break;
        }
        case ANODE_IMM: {
            printf("ANODE_IMM");
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
        case ANODE_LABEL_DEF: {
            struct ANodeLabelDef* l = (struct ANodeLabelDef*)n;
            free_unit(l, sizeof(struct ANodeLabelDef));
            break;
        }
        case ANODE_LABEL_REF: {
            struct ANodeLabelRef* l = (struct ANodeLabelRef*)n;
            free_unit(l, sizeof(struct ANodeLabelRef));
            break;
        }
        case ANODE_OP: {
            struct ANodeOp *o = (struct ANodeOp*)n;
            ast_free(o->operand1);
            ast_free(o->operand2);
            free_unit(o, sizeof(struct ANodeOp));
            break;
        }
        case ANODE_REG: {
            struct ANodeReg* r = (struct ANodeReg*)n;
            free_unit(r, sizeof(struct ANodeReg));
            break;
        }
        case ANODE_IMM: {
            struct ANodeImm* i = (struct ANodeImm*)n;
            free_unit(i, sizeof(struct ANodeImm));
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
    unsigned conditional_label_id; //used to create unique ids for labels in assembly code
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

/*
 * Assembler
 */

struct U32Array {
    uint32_t *elements;
    int count;
    int max_count;
};


void u32a_init(struct U32Array *ua) {
    ua->elements = NULL;
    ua->count = 0;
    ua->max_count = 0;
}

void u32a_free(struct U32Array *ua) {
    free_arr(ua->elements, sizeof(uint32_t), ua->max_count); 
}

void u32a_add(struct U32Array *ua, uint32_t element) {
    int old_max = ua->max_count;
    if (ua->count + 1 > ua->max_count) {
        ua->max_count *= 2;
        if (ua->max_count == 0)
            ua->max_count = 8;
    }

    ua->elements = alloc_arr(ua->elements, sizeof(uint32_t), old_max, ua->max_count);

    ua->elements[ua->count] = element;
    ua->count++;
}

struct ALabel {
    struct Token t;
    uint32_t addr;
    bool defined;
    struct U32Array ref_locs;
};

void al_init(struct ALabel *al, struct Token t, uint32_t addr, bool defined) {
    al->t = t;
    al->addr = addr;
    al->defined = defined;
    u32a_init(&al->ref_locs);
}

struct ALabelArray {
    struct ALabel *elements;
    int count;
    int max_count;
};

void ala_init(struct ALabelArray* ala) {
    ala->elements = NULL;
    ala->count = 0;
    ala->max_count = 0;
}

void ala_free(struct ALabelArray *ala) {
    free_arr(ala->elements, sizeof(struct ALabel), ala->max_count); 
}

void ala_add(struct ALabelArray *ala, struct ALabel element) {
    int old_max = ala->max_count;
    if (ala->count + 1 > ala->max_count) {
        ala->max_count *= 2;
        if (ala->max_count == 0)
            ala->max_count = 8;
    }

    ala->elements = alloc_arr(ala->elements, sizeof(struct ALabel), old_max, ala->max_count);

    ala->elements[ala->count] = element;
    ala->count++;
}

struct ALabel* ala_get_label(struct ALabelArray *ala, struct Token t) {
    for (int i = 0; i < ala->count; i++) {
        struct ALabel* cur = &ala->elements[i];
        if (cur->t.len == t.len && strncmp(cur->t.start, t.start, t.len) == 0) {
            return cur;
        }
    }

    return NULL;
}


struct Assembler {
    struct CharArray buf;
    int program_start_patch;
    int phdr_start_patch;
    int ehdr_size_patch;
    int phdr_size_patch;
    int filesz_patch;
    int memsz_patch;
    int vaddr_patch;
    int paddr_patch;
    uint32_t location;
    uint32_t program_start_loc;
    struct ALabelArray ala;
};

void assembler_init(struct Assembler *a) {
    ca_init(&a->buf);
    a->program_start_patch = 0;
    a->phdr_start_patch = 0;
    a->ehdr_size_patch = 0;
    a->phdr_size_patch = 0;
    a->filesz_patch = 0;
    a->memsz_patch = 0;
    a->vaddr_patch = 0;
    a->paddr_patch = 0;
    a->location = 0;
    a->program_start_loc = 0;
    ala_init(&a->ala);
}

void assembler_free(struct Assembler *a) {
    ca_free(&a->buf);
}

void assembler_append_elf_header(struct Assembler *a) {
    uint8_t magic[8];
    magic[0] = 0x7f;
    magic[1] = 'E';
    magic[2] = 'L';
    magic[3] = 'F';
    magic[4] = 1;
    magic[5] = 1;
    magic[6] = 1;
    magic[7] = 0;
    ca_append(&a->buf, (char*)magic, 8);

    uint8_t zeros[8] = {0};
    ca_append(&a->buf, (char*)zeros, 8);
    uint16_t file_type = 2;
    ca_append(&a->buf, (char*)&file_type, 2);
    uint16_t arch_type = 3;
    ca_append(&a->buf, (char*)&arch_type, 2);
    uint32_t file_version = 1;
    ca_append(&a->buf, (char*)&file_version, 4);

    a->program_start_patch = a->buf.count; //need to add executable offset (0x08048000)
    ca_append(&a->buf, (char*)zeros, 4); //TODO: patch this
    a->phdr_start_patch = a->buf.count;
    ca_append(&a->buf, (char*)zeros, 4); //TODO: patch this
    ca_append(&a->buf, (char*)zeros, 8);
    a->ehdr_size_patch = a->buf.count; //TODO: patch this
    ca_append(&a->buf, (char*)zeros, 2);
    a->phdr_size_patch = a->buf.count; //TODO: patch this 
    ca_append(&a->buf, (char*)zeros, 2);

    uint16_t phdr_entries = 1;
    ca_append(&a->buf, (char*)&phdr_entries, 2);
    ca_append(&a->buf, (char*)zeros, 6);

    //patch elf header size
    uint16_t ehdr_size = a->buf.count;
    memcpy(&a->buf.chars[a->ehdr_size_patch], &ehdr_size, 2);
}

void assembler_append_program_header(struct Assembler *a) {
    //TODO: patch program header size and _start location (offset by org value)
    uint32_t phdr_start = a->buf.count;
    memcpy(&a->buf.chars[a->phdr_start_patch], &phdr_start, 4);

    uint32_t type = 1; //load
    ca_append(&a->buf, (char*)&type, 4);

    uint32_t offset = 0;
    ca_append(&a->buf, (char*)&offset, 4);

    a->vaddr_patch = a->buf.count;
    uint32_t vaddr = 0;
    ca_append(&a->buf, (char*)&vaddr, 4);

    a->paddr_patch = a->buf.count;
    uint32_t paddr = 0;
    ca_append(&a->buf, (char*)&paddr, 4);

    a->filesz_patch = a->buf.count;
    uint32_t filesz = 0;
    ca_append(&a->buf, (char*)&filesz, 4);

    a->memsz_patch = a->buf.count;
    uint32_t memsz = 0;
    ca_append(&a->buf, (char*)&memsz, 4);

    uint32_t flags = 5;
    ca_append(&a->buf, (char*)&flags, 4);

    uint32_t align = 0x1000;
    ca_append(&a->buf, (char*)&align, 4);

    uint16_t phdr_size = a->buf.count - phdr_start;
    memcpy(&a->buf.chars[a->phdr_size_patch], &phdr_size, 2);
}

uint32_t get_double(struct Token t) {
    char* end = t.start + t.len;
    long ret;
    if (t.type == T_INT) {
        ret = strtol(t.start, &end, 10);
    } else if (t.type == T_HEX) {
        ret = strtol(t.start + 2, &end, 16);
    }
    return (int32_t)ret;
}

uint8_t get_byte(struct Token t) {
    char* end = t.start + t.len;
    long ret;
    if (t.type == T_INT) {
        ret = strtol(t.start, &end, 10);
    } else if (t.type == T_HEX) {
        ret = strtol(t.start + 2, &end, 16);
    }
    return (uint8_t)ret;
}

uint8_t reg_reg_code(enum TokenType dst, enum TokenType src) {
    return src * 8 + 0xc0 + dst;
}

uint8_t mov_reg_imm_code(enum TokenType dst) {
    return dst + 0xb8;
}

//first byte
//xxxxxx00
static uint8_t ins_tbl[] = {
    0x00,   //add
    0x88,   //mov
    0x28    //sub
};

enum OpInstruction {
    INS_ADD = 0,
    INS_MOV,
    INS_SUB
};

//000000x0
static uint8_t dir_tbl[] = {
    0x00,   //dst is r/m
    0x02    //dst is r
};

enum OpDirection {
    DIR_RSM = 0,
    DIR_REG
};

//0000000x
static uint8_t siz_tbl[] = {
    0x00,   //8-bit operand
    0x01    //32-bit operand
};

enum OpSize {
    SIZ_08B = 0,
    SIZ_32B
};

//second byte
//xx000000
static uint8_t mod_tbl[] = {
    0x00,
    0x40,
    0x80,
    0xc0    //r/m is register
};

enum OpMod {
    MOD_TEMP1 = 0, //TODO: choose better name than _TEMP
    MOD_TEMP2,
    MOD_TEMP3,
    MOD_REG
};

//registers table indices should match with enum TokenType register values
//00xxx000
static uint8_t reg_tbl[] = {
    0x00,   //eax
    0x08,
    0x10,
    0x18,
    0x20,
    0x28,
    0x30,
    0x38
};

//00000xxx
static uint8_t rsm_tbl[] = {
    0x00,   //eax
    0x01,
    0x02,
    0x03,
    0x04,
    0x05,
    0x06,
    0x07
};

void assemble_node(struct Assembler *a, struct Node *node) {
    switch (node->type) {
        case ANODE_IMM: {
            struct ANodeImm* imm = (struct ANodeImm*)node;
            uint32_t num = get_double(imm->t);
            ca_append(&a->buf, (char*)(&num), 4);
            break;
        }
        case ANODE_LABEL_DEF: {
            struct ALabel *label = NULL;
            struct ANodeLabelDef* ld = (struct ANodeLabelDef*)node;
            if ((label = ala_get_label(&a->ala, ld->t))) {
                if (label->defined) {
                    ems_add(&ems, ld->t.line, "Assembler Error: Labels cannot be defined more than once.");
                } 
                label->t = ld->t;
                label->addr = a->buf.count;
                label->defined = true;
            } else {
                struct ALabel al;
                al_init(&al, ld->t, a->buf.count, true);
                ala_add(&a->ala, al);
            }
            break;
        }
        case ANODE_LABEL_REF: {
            struct ALabel *label = NULL;
            struct ANodeLabelRef* lr = (struct ANodeLabelRef*)node;
            if ((label = ala_get_label(&a->ala, lr->t))) {
                u32a_add(&label->ref_locs, a->buf.count);
            } else {
                struct ALabel al;
                al_init(&al, lr->t, 0, false);
                ala_add(&a->ala, al);
                u32a_add(&al.ref_locs, a->buf.count);
            }
            uint32_t place_holder = 0x0;
            ca_append(&a->buf, (char*)&place_holder, 4);
            break;
        }
        case ANODE_OP: {
            struct ANodeOp* o = (struct ANodeOp*)node;
            struct Token op = o->operator;
            switch (op.type) {
                case T_MOV: {
                    if (o->operand1->type == ANODE_REG && o->operand2->type == ANODE_REG) {
                        uint8_t mov_rr_code = 0x89;
                        ca_append(&a->buf, (char*)&mov_rr_code, 1);
                        struct ANodeReg* dst = (struct ANodeReg*)(o->operand1);
                        struct ANodeReg* src = (struct ANodeReg*)(o->operand2);
                        uint8_t rr_code = reg_reg_code(dst->t.type, src->t.type);
                        ca_append(&a->buf, (char*)&rr_code, 1);
                    } else if (o->operand1->type == ANODE_REG) {
                        struct ANodeReg* reg = (struct ANodeReg*)(o->operand1);
                        uint8_t opcode = mov_reg_imm_code(reg->t.type);
                        ca_append(&a->buf, (char*)&opcode, 1);
                        assemble_node(a, o->operand2);
                    }
                    break;
                }
                case T_ADD: {
                    if (o->operand1->type == ANODE_REG && o->operand2->type == ANODE_REG) {
                        struct ANodeReg* dst = (struct ANodeReg*)(o->operand1);
                        struct ANodeReg* src = (struct ANodeReg*)(o->operand2);

                        uint8_t opc = ins_tbl[INS_ADD] | dir_tbl[DIR_RSM] | siz_tbl[SIZ_32B];
                        ca_append(&a->buf, (char*)&opc, 1);

                        uint8_t opd = mod_tbl[MOD_REG] | reg_tbl[src->t.type] | rsm_tbl[dst->t.type];
                        ca_append(&a->buf, (char*)&opd, 1);
                    } else if (o->operand1->type == ANODE_REG) {
                        struct ANodeReg* dst = (struct ANodeReg*)(o->operand1);
                        if (dst->t.type == T_EAX) {
                            uint8_t add_code = 0x05;
                            ca_append(&a->buf, (char*)&add_code, 1);
                        } else {
                            uint8_t add_code = 0x81;
                            ca_append(&a->buf, (char*)&add_code, 1);
                            uint8_t r_code = dst->t.type + 0xc0;
                            ca_append(&a->buf, (char*)&r_code, 1);
                        }
                        assemble_node(a, o->operand2);
                    }
                    break;
                }
                case T_SUB: {
                    if (o->operand1->type == ANODE_REG && o->operand2->type == ANODE_REG) {
                        uint8_t code = 0x29;
                        ca_append(&a->buf, (char*)&code, 1);

                        struct ANodeReg *dst = (struct ANodeReg*)(o->operand1);
                        struct ANodeReg *src = (struct ANodeReg*)(o->operand2);
                        uint8_t rr_code = reg_reg_code(dst->t.type, src->t.type);
                        ca_append(&a->buf, (char*)&rr_code, 1);
                    } else if (o->operand1->type == ANODE_REG) {
                        struct ANodeReg *reg = (struct ANodeReg*)(o->operand1);
                        if (reg->t.type == T_EAX) {
                            uint8_t code = 0x2d;
                            ca_append(&a->buf, (char*)&code, 1);
                        } else {
                            uint8_t code1 = 0x81;
                            ca_append(&a->buf, (char*)&code1, 1);
                            uint8_t code2 = 0xe8 + reg->t.type;
                            ca_append(&a->buf, (char*)&code2, 1);
                        }
                        assemble_node(a, o->operand2);
                    } 
                    break;
                }
                case T_IMUL: {
                    if (o->operand1->type == ANODE_REG && o->operand2->type == ANODE_REG) {
                        uint16_t code = 0xaf0f;
                        ca_append(&a->buf, (char*)&code, 2);

                        struct ANodeReg *dst = (struct ANodeReg*)(o->operand1);
                        struct ANodeReg *src = (struct ANodeReg*)(o->operand2);

                        uint8_t rrcode = reg_reg_code(src->t.type, dst->t.type);
                        ca_append(&a->buf, (char*)&rrcode, 1);
                    } else if (o->operand1->type == ANODE_REG) {
                        uint8_t code1 = 0x69;
                        ca_append(&a->buf, (char*)&code1, 1);

                        struct ANodeReg *reg = (struct ANodeReg*)(o->operand1);
                        uint8_t code2 = reg->t.type * 9 + 0xc0;
                        ca_append(&a->buf, (char*)&code2, 1);
                        assemble_node(a, o->operand2);
                    }
                    break;
                }
                case T_IDIV: {
                    if (o->operand1->type == ANODE_REG && o->operand2 == NULL) {
                        uint8_t code = 0xf7;
                        ca_append(&a->buf, (char*)&code, 1);
                        struct ANodeReg *reg = (struct ANodeReg*)(o->operand1);
                        uint8_t r_offset = 0xf8 + reg->t.type;
                        ca_append(&a->buf, (char*)&r_offset, 1);
                    } else {
                        //TODO: error message
                    }
                    break;
                }
                case T_PUSH: {
                    if (o->operand1->type == ANODE_IMM || o->operand1->type == ANODE_LABEL_REF) {
                        uint8_t code = 0x68;
                        ca_append(&a->buf, (char*)&code, 1);
                        assemble_node(a, o->operand1);
                    } else if (o->operand1->type == ANODE_REG) {
                        struct ANodeReg *reg = (struct ANodeReg*)(o->operand1);
                        uint8_t code = 0x50 + reg->t.type;
                        ca_append(&a->buf, (char*)&code, 1);
                    }
                    break;
                }
                case T_POP: {
                    struct ANodeReg *reg = (struct ANodeReg*)(o->operand1);
                    uint8_t code = 0x58 + reg->t.type;
                    ca_append(&a->buf, (char*)&code, 1);
                    break;
                }
                case T_INTR: {
                    if (o->operand1->type == ANODE_IMM) {
                        uint8_t opcode = 0xcd;
                        ca_append(&a->buf, (char*)&opcode, 1);
                        struct ANodeImm* imm = (struct ANodeImm*)(o->operand1);
                        uint8_t num = get_byte(imm->t);
                        ca_append(&a->buf, (char*)(&num), 1);
                    } else {
                        printf("Operator not recognized: INTR branch\n");
                    }
                    break;
                }
                case T_ORG: {
                    if (o->operand1->type == ANODE_IMM) {
                        struct ANodeImm* imm = (struct ANodeImm*)(o->operand1);
                        a->location = get_double(imm->t);
                    } else {
                        printf("Operand must be immediate value\n");
                    }
                    break;
                }
                case T_CDQ: {
                    if (o->operand1 == NULL && o->operand2 == NULL) {
                        uint8_t code = 0x99;
                        ca_append(&a->buf, (char*)&code, 1);
                    } else {
                        //TODO: error
                    }
                    break;
                }
                default:
                    printf("Operator not recognized: default case\n");
                    break;
            }
            break;
        }
        default:
            printf("Not supported yet\n");
    }
}



void assembler_append_program(struct Assembler *a, struct NodeArray *na) {
    a->program_start_loc = a->buf.count;

    for (int i = 0; i < na->count; i++) {
        assemble_node(a, na->nodes[i]);
    }

    uint32_t filesz = a->buf.count;
    memcpy(&a->buf.chars[a->filesz_patch], &filesz, 4);

    uint32_t memsz = a->buf.count;
    memcpy(&a->buf.chars[a->memsz_patch], &memsz, 4);
}

void assembler_patch_locations(struct Assembler *a) {
    uint32_t program_start = a->program_start_loc + a->location;
    memcpy(&a->buf.chars[a->program_start_patch], &program_start, 4);

    uint32_t vaddr = a->location;
    memcpy(&a->buf.chars[a->vaddr_patch], &vaddr, 4);

    uint32_t paddr = a->location;
    memcpy(&a->buf.chars[a->paddr_patch], &paddr, 4);
}

void assembler_patch_labels(struct Assembler *a) {
    for (int i = 0; i < a->ala.count; i++) {
        struct ALabel* l = &a->ala.elements[i];
        if (!(l->defined)) {
            ems_add(&ems, l->t.line, "Assembling Error: Label '%.*s' not defined.", l->t.len, l->t.start);
        } else {
            uint32_t final_addr = l->addr + a->location;
            for (int j = 0; j < l->ref_locs.count; j++) {
                uint32_t loc = l->ref_locs.elements[j];
                memcpy(&a->buf.chars[loc], &final_addr, 4);
            }
        }
    }
}


void assembler_write_binary(struct Assembler *a, char* filename) {
    FILE *f = fopen(filename, "wb");
    fwrite(a->buf.chars, sizeof(char), a->buf.count, f);
    fclose(f);
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
    lexer_init(&l, code, ST_ASM);

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


    //parsing and assembling
    struct Parser p;
    parser_init(&p, &ta);
    struct NodeArray na;
    na_init(&na);
    
    while (parser_peek_one(&p).type != T_EOF) {
        na_add(&na, aparse_stmt(&p));
    }

    for (int i = 0; i < na.count; i++) {
        ast_print(na.nodes[i]);
        printf("\n");
    }

    struct Assembler a;
    assembler_init(&a);

    assembler_append_elf_header(&a);
    assembler_append_program_header(&a);  
    assembler_append_program(&a, &na);
    assembler_patch_locations(&a);
    assembler_patch_labels(&a);
    
    assembler_write_binary(&a, "final.bin");

    assembler_free(&a);

    //end of assembling 

    /*
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
    printf("Allocated memory remaining: %ld\n", allocated);*/

    return 0;
}
