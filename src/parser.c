#include <stddef.h>
#include <stdbool.h>

#include "parser.h"
#include "error.h"
#include "ast.h"
#include "memory.h"


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
            case T_CALL:
                left = aparse_expr(p);
                break;
            //no operands
            case T_CDQ:
            case T_RET:
                break;
            default:
                ems_add(&ems, next.line, "AParse Error: Invalid token type!");
        }
        return anode_op(op, left, right);
    }
}
