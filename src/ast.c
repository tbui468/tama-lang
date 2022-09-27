
#include <stdio.h>
#include "ast.h"
#include "memory.h"


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
    node->n.type = ANODE_IMM32;
    node->t = t;
    return (struct Node*)node;
}

struct Node *anode_deref(struct Node *reg, struct Token op, struct Node *imm) {
    struct ANodeDeref *node = alloc_unit(sizeof(struct ANodeDeref));
    node->n.type = ANODE_DEREF;
    node->reg = reg;
    node->op = op;
    node->imm = imm;
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
        case ANODE_IMM32: {
            printf("ANODE_IMM32");
            break;
        }
        case ANODE_DEREF: {
            printf("ANODE_DEREF");
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
        case ANODE_IMM32: {
            struct ANodeImm* i = (struct ANodeImm*)n;
            free_unit(i, sizeof(struct ANodeImm));
            break;
        }
        case ANODE_DEREF: {
            struct ANodeDeref* d = (struct ANodeDeref*)n;
            ast_free(d->reg);
            ast_free(d->imm);
            free_unit(d, sizeof(struct ANodeDeref));
            break;
        }
        default:
            printf("Node type not recognized\n");
            break;
    }
}

