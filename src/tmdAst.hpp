#ifndef TMD_AST_HPP
#define TMD_AST_HPP

#include "ast.hpp"
#include "semant.hpp"


class AstBinary: public Ast {
    public:
        struct Token m_op;
        Ast* m_left;
        Ast* m_right;
        AstBinary(struct Token op, Ast* left, Ast* right): m_op(op), m_left(left), m_right(right) {}
        std::string to_string() {
            return "binary";
        }
        enum TokenType translate(Semant& s) {
            enum TokenType ret_type = T_NIL_TYPE;
            enum TokenType left_type = m_left->translate(s);
            enum TokenType right_type = m_right->translate(s);
            if (left_type != right_type) {
                ems_add(&ems, m_op.line, "Type Error: Left and right types don't match!");
                ret_type = T_NIL_TYPE; //TODO: Should be error type to avoid same error message cascade 
            } else if (m_op.type == T_PLUS || m_op.type == T_MINUS || m_op.type == T_SLASH || m_op.type == T_STAR) {
                ret_type = T_INT_TYPE;
            } else {
                ret_type = T_BOOL_TYPE;
            }

            s.write_op("    pop     %s", "ebx");
            s.write_op("    pop     %s", "eax");

            switch (m_op.type) {
                case T_PLUS:
                    s.write_op("    add     %s, %s", "eax", "ebx");
                    break;
                case T_MINUS:
                    s.write_op("    sub     %s, %s", "eax", "ebx");
                    break;
                case T_STAR:
                    s.write_op("    imul    %s, %s", "eax", "ebx");
                    break;
                case T_SLASH:
                    s.write_op("    cdq"); //sign extend eax into edx
                    s.write_op("    idiv    %s", "ebx");
                    break;
                case T_LESS:
                    s.write_op("    cmp     %s, %s", "eax", "ebx");
                    s.write_op("    setl    %s", "al"); //set byte to 0 or 1
                    s.write_op("    movzx   %s, %s", "eax", "al");
                    break;
                case T_GREATER:
                    s.write_op("    cmp     %s, %s", "eax", "ebx");
                    s.write_op("    setg    %s", "al"); //set byte to 0 or 1
                    s.write_op("    movzx   %s, %s", "eax", "al");
                    break;
                case T_LESS_EQUAL:
                    s.write_op("    cmp     %s, %s", "eax", "ebx");
                    s.write_op("    setle   %s", "al"); //set byte to 0 or 1
                    s.write_op("    movzx   %s, %s", "eax", "al");
                    break;
                case T_GREATER_EQUAL:
                    s.write_op("    cmp     %s, %s", "eax", "ebx");
                    s.write_op("    setge   %s", "al");; //set byte to 0 or 1
                    s.write_op("    movzx   %s, %s", "eax", "al");
                    break;
                case T_EQUAL_EQUAL:
                    s.write_op("    cmp     %s, %s", "eax", "ebx");
                    s.write_op("    sete    %s", "al"); //set byte to 0 or 1
                    s.write_op("    movzx   %s, %s", "eax", "al");
                    break;
                case T_NOT_EQUAL:
                    s.write_op("    cmp     %s, %s", "eax", "ebx");
                    s.write_op("    setne   %s", "al"); //set byte to 0 or 1
                    s.write_op("    movzx   %s, %s", "eax", "al");
                    break;
                case T_AND:
                    s.write_op("    and     %s, %s", "eax", "ebx");
                    break;
                case T_OR:
                    s.write_op("    or      %s, %s", "eax", "ebx");
                    break;
                default:
                    ems_add(&ems, m_op.line, "Translate Error: Binary operator not recognized.");
                    break;
            }

            s.write_op("    push    %s", "eax");
            return ret_type;
        }
};

class AstUnary: public Ast {
    public:
        struct Token m_op;
        Ast* m_right;
        AstUnary(struct Token op, Ast* right): m_op(op), m_right(right) {}
        std::string to_string() {
            return "unary";
        }
        enum TokenType translate(Semant& s) {
            enum TokenType ret_type = m_right->translate(s);

            s.write_op("    pop     %s", "eax");

            if (ret_type == T_INT_TYPE) {
                s.write_op("    neg     %s", "eax");
            } else if (ret_type == T_BOOL_TYPE) {
                s.write_op("    xor     %s, %d", "eax", 1); 
            }

            s.write_op("    push    %s", "eax");
            return ret_type;
        }
};

class AstLiteral: public Ast {
    public:
        struct Token m_lexeme;
        AstLiteral(struct Token lexeme): m_lexeme(lexeme) {}
        std::string to_string() {
            return "literal";
        }
        enum TokenType translate(Semant& s) {
            if (m_lexeme.type == T_INT) {
                s.write_op("    push    %.*s", m_lexeme.len, m_lexeme.start);
            } else if (m_lexeme.type == T_TRUE) {
                s.write_op("    push    %d", 1);
            } else if (m_lexeme.type == T_FALSE) {
                s.write_op("    push    %d", 0);
            }

            switch (m_lexeme.type) {
                case T_INT:
                    return T_INT_TYPE;
                case T_TRUE:
                case T_FALSE:
                    return T_BOOL_TYPE;
                default:
                    return T_NIL_TYPE;
            }
        }
};

class AstPrint: public Ast {
    public:
        Ast* m_arg;
        AstPrint(Ast* arg): m_arg(arg) {}
        std::string to_string() {
            return "print";
        }
        enum TokenType translate(Semant& s) {
            enum TokenType arg_type = m_arg->translate(s);

            if (arg_type == T_INT_TYPE) {
                s.write_op("    call    %s", "_print_int");
            } else if (arg_type == T_BOOL_TYPE) {
                s.write_op("    call    %s", "_print_bool");
            }
            s.write_op("    add     %s, %d", "esp", 4);

            s.write_op("    push    %s", "0xa");
            s.write_op("    call    %s", "_print_char");
            s.write_op("    add     %s, %d", "esp", 4);
            return T_NIL_TYPE;
        }
};

class AstExprStmt: public Ast {
    public:
        Ast* m_expr;
        AstExprStmt(Ast* expr): m_expr(expr) {}
        std::string to_string() {
            return "exprstmt";
        }
        enum TokenType translate(Semant& s) {
            enum TokenType expr_type = m_expr->translate(s);

            s.write_op("    pop     %s", "ebx");
            return expr_type;
        }
};

class AstDeclSym: public Ast {
    public:
        struct Token m_symbol;
        struct Token m_type;
        Ast* m_value;
        AstDeclSym(struct Token symbol, struct Token type, Ast* value): m_symbol(symbol), m_type(type), m_value(value) {}
        std::string to_string() {
            return "declvar";
        }
        enum TokenType translate(Semant& s) {
            enum TokenType right_type = m_value->translate(s);
            /*
            if (right_type != m_type.type) {
                ems_add(&ems, m_symbol.line, "Type Error: Declaration type and assigned value type don't match!");
            }

            if (m_envs->declared_in_scope(m_symbol)) {
                ems_add(&ems, m_symbol.line, "Semantic Error: Local symbol already declared in this scope!");
            }

            m_envs->add_symbol(m_symbol, m_type);*/

            //local variable is on stack at this point
            return right_type;
        }
};

class AstGetSym: public Ast {
    public:
        struct Token m_symbol;
        AstGetSym(struct Token symbol): m_symbol(symbol) {}
        std::string to_string() {
            return "getvar";
        }
        enum TokenType translate(Semant& s) {
            return T_INT_TYPE;
        }
};

class AstSetSym: public Ast {
    public:
        struct Token m_symbol;
        Ast* m_value;
        AstSetSym(struct Token symbol, Ast* value): m_symbol(symbol), m_value(value) {}
        std::string to_string() {
            return "setvar";
        }
        enum TokenType translate(Semant& s) {
            return T_INT_TYPE;
        }
};

class AstBlock: public Ast {
    public:
        std::vector<Ast*> m_stmts;
        AstBlock(std::vector<Ast*> stmts): m_stmts(stmts) {}
        std::string to_string() {
            return "block";
        }
        enum TokenType translate(Semant& s) {
            return T_INT_TYPE;
        }
};

class AstIf: public Ast {
    public:
        Ast* m_condition;
        Ast* m_then_block;
        Ast* m_else_block;
        AstIf(Ast* condition, Ast* then_block, Ast* else_block): m_condition(condition), m_then_block(then_block), m_else_block(else_block) {}
        std::string to_string() {
            return "if";
        }
        enum TokenType translate(Semant& s) {
            return T_INT_TYPE;
        }
};

class AstWhile: public Ast {
    public:
        Ast* m_condition;
        Ast* m_while_block;
        AstWhile(Ast* condition, Ast* while_block): m_condition(condition), m_while_block(while_block) {}
        std::string to_string() {
            return "while";
        }
        enum TokenType translate(Semant& s) {
            return T_INT_TYPE;
        }
};



#endif //TMD_AST_HPP
