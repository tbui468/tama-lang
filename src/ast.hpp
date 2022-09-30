#ifndef TMD_AST_H
#define TMD_AST_H

#include "token.hpp"

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
    ANODE_IMM32,
    ANODE_DEREF,
    ANODE_IMM8 //TODO: not implemented yet
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


/*
 * Assembler
 */

struct ANodeStmt {
    struct Node n;
    struct Token op;
    struct Node *operand1;
    struct Node *operand2;
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
    struct Token op;
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

struct ANodeDeref {
    struct Node n;
    struct Node *reg;
    struct Token op;
    struct Node *imm;
};

void na_init(struct NodeArray *na);
void ast_free(struct Node* n);
void na_free(struct NodeArray *na);
void na_add(struct NodeArray *na, struct Node* n);

struct Node* node_literal(struct Token value);
struct Node* node_unary(struct Token op, struct Node* right);
struct Node* node_binary(struct Node* left, struct Token op, struct Node* right);
struct Node *node_print(struct Node* arg);
struct Node *node_expr_stmt(struct Node *expr);
struct Node *node_decl_var(struct Token var, struct Token type, struct Node *expr);
struct Node *node_get_var(struct Token var);
struct Node *node_set_var(struct Token var, struct Node *expr);
struct Node *node_block(struct Token l_brace, struct Token r_brace, struct NodeArray* na);
struct Node *node_if(struct Token if_token, struct Node *condition, struct Node *then_block, struct Node *else_block);
struct Node *node_while(struct Token while_token, struct Node *condition, struct Node *while_block);

struct Node *anode_label_def(struct Token t);
struct Node *anode_label_ref(struct Token t);
struct Node *anode_op(struct Token op, struct Node *operand1, struct Node *operand2);
struct Node *anode_reg(struct Token t);
struct Node *anode_imm(struct Token t);
struct Node *anode_deref(struct Node *reg, struct Token t, struct Node *imm);

void ast_print(struct Node* n);
void ast_free(struct Node* n);

#endif //TMD_AST_H
