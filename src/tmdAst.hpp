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
        Type translate(Semant& s) {
            Type ret_type = Type(T_NIL_TYPE);
            Type left_type = m_left->translate(s);
            Type right_type = m_right->translate(s);
            if (!left_type.is_of_type(right_type)) {
                ems_add(&ems, m_op.line, "Type Error: Left and right types don't match!");
            } else if (m_op.type == T_PLUS || m_op.type == T_MINUS || m_op.type == T_SLASH || m_op.type == T_STAR) {
                ret_type = Type(T_INT_TYPE);
            } else {
                ret_type = Type(T_BOOL_TYPE);
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
        Type translate(Semant& s) {
            Type ret_type = m_right->translate(s);

            s.write_op("    pop     %s", "eax");

            if (ret_type.m_dtype == T_INT_TYPE) {
                s.write_op("    neg     %s", "eax");
            } else if (ret_type.m_dtype == T_BOOL_TYPE) {
                s.write_op("    xor     %s, %d", "eax", 1); 
            } else {
                ems_add(&ems, m_op.line, "Unary expression does not support that operator.");
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
        Type translate(Semant& s) {
            if (m_lexeme.type == T_INT) {
                s.write_op("    push    %.*s", m_lexeme.len, m_lexeme.start);
            } else if (m_lexeme.type == T_TRUE) {
                s.write_op("    push    %d", 1);
            } else if (m_lexeme.type == T_FALSE) {
                s.write_op("    push    %d", 0);
            } else if (m_lexeme.type == T_NIL) {
                s.write_op("    push    %d", 0);
            } else {
                ems_add(&ems, m_lexeme.line, "Synax Error: Invalid literal");
            }

            switch (m_lexeme.type) {
                case T_INT:
                    return Type(T_INT_TYPE);
                case T_TRUE:
                case T_FALSE:
                    return Type(T_BOOL_TYPE);
                default:
                    return Type(T_NIL_TYPE);
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
        Type translate(Semant& s) {
            Type arg_type = m_arg->translate(s);

            if (arg_type.m_dtype == T_INT_TYPE) {
                s.write_op("    call    %s", "_print_int");
            } else if (arg_type.m_dtype == T_BOOL_TYPE) {
                s.write_op("    call    %s", "_print_bool");
            }
            s.write_op("    add     %s, %d", "esp", 4);

            s.write_op("    push    %s", "0xa");
            s.write_op("    call    %s", "_print_char");
            s.write_op("    add     %s, %d", "esp", 4);
            return Type(T_NIL_TYPE);
        }
};

class AstExprStmt: public Ast {
    public:
        Ast* m_expr;
        AstExprStmt(Ast* expr): m_expr(expr) {}
        std::string to_string() {
            return "exprstmt";
        }
        Type translate(Semant& s) {
            Type expr_type = m_expr->translate(s);

            s.write_op("    pop     %s", "ebx");
            return Type(T_NIL_TYPE);
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
        Type translate(Semant& s) {
            Type right_type = m_value->translate(s);
            if (right_type.m_dtype != m_type.type) {
                ems_add(&ems, m_symbol.line, "Type Error: Declaration type and assigned value type don't match!");
            }

            if (s.m_env.declared_in_scope(m_symbol)) {
                ems_add(&ems, m_symbol.line, "Syntax Error: Local symbol already declared in this scope!");
            }

            s.m_env.add_symbol(m_symbol, Type(m_type.type));

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
        Type translate(Semant& s) {
            Symbol* sym = s.m_env.get_symbol(m_symbol);

            Type ret_type = Type(T_NIL_TYPE); //TODO: Should be error type to avoid multiple error messages

            if (!sym) {
                ems_add(&ems, m_symbol.line, "Syntax Error: Variable not declared!");
            } else {
                ret_type = sym->m_type;
            }

            s.write_op("    mov     %s, [%s - %d]", "eax", "ebp", 4 * (sym->m_bp_offset + 1));
            s.write_op("    push    %s", "eax");
            
            return ret_type;
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
        Type translate(Semant& s) {
            Symbol *sym = s.m_env.get_symbol(m_symbol);
            Type ret_type = Type(T_NIL_TYPE); //TODO: should have special error type to avoid repeated error messages

            if (!sym) {
                ems_add(&ems, m_symbol.line, "Syntax Error: Variable not declared!");
            } else {
                Type assigned_type = m_value->translate(s);
                if (!sym->m_type.is_of_type(assigned_type)) {
                    ems_add(&ems, m_symbol.line, "Type Error: Declaration type and assigned value type don't match!");
                } else {
                    ret_type = assigned_type;
                }
            }

            s.write_op("    pop     %s", "eax");
            s.write_op("    mov     [%s - %d], %s", "ebp", 4 * (sym->m_bp_offset + 1), "eax");
            s.write_op("    push    %s", "eax");
            return ret_type;
        }
};

class AstFunDef: public Ast {
    public:
        struct Token m_symbol;
        std::vector<Ast*> m_params;
        struct Token m_ret_type;
        Ast* m_body;
    public:
        AstFunDef(struct Token symbol, std::vector<Ast*> params, struct Token ret_type, Ast* body):
            m_symbol(symbol), m_params(params), m_ret_type(ret_type), m_body(body) {}
        std::string to_string() {
            return "param";
        }
        Type translate(Semant& s) {
            s.write_op("%.*s:", m_symbol.len, m_symbol.start);
            s.write_op("    push    %s", "ebp");
            s.write_op("    mov     %s, %s", "ebp", "esp");

            //TODO: need to compile parameters

            s.m_compiling_fun = this;
            m_body->translate(s);
            s.m_compiling_fun = nullptr;

            s.write_op("__%.*s_ret:", m_symbol.len, m_symbol.start);
            s.write_op("    pop     %s", "ebp");
            s.write_op("    ret");
            return Type(T_NIL_TYPE);
        }
};

class AstBlock: public Ast {
    public:
        std::vector<Ast*> m_stmts;
        AstBlock(std::vector<Ast*> stmts): m_stmts(stmts) {}
        std::string to_string() {
            return "block";
        }
        Type translate(Semant& s) {
            s.m_env.begin_scope();
            for (Ast* n: m_stmts) {
                Type type = n->translate(s);
                if (type.m_dtype == T_RET_TYPE) {
                    AstFunDef* n = dynamic_cast<AstFunDef*>(s.m_compiling_fun);
                    int sym_count = s.m_env.symbol_count(); 
                    s.write_op("    add     %s, %d", "esp", 4 * sym_count);
                    s.write_op("    jmp     __%.*s_ret", n->m_symbol.len, n->m_symbol.start);
                    break;
                }
            }
            int pop_count = s.m_env.end_scope();
            s.write_op("    add     %s, %d", "esp", 4 * pop_count);
            return Type(T_NIL_TYPE);
        }
};

class AstIf: public Ast {
    public:
        struct Token m_t;
        Ast* m_condition;
        Ast* m_then_block;
        Ast* m_else_block;
        AstIf(struct Token t, Ast* condition, Ast* then_block, Ast* else_block): m_t(t), m_condition(condition), m_then_block(then_block), m_else_block(else_block) {}
        std::string to_string() {
            return "if";
        }
        Type translate(Semant& s) {
            Type condition_type = m_condition->translate(s);
            if (condition_type.m_dtype != T_BOOL_TYPE) {
                ems_add(&ems, m_t.line, "Syntax Error: 'if' keyword must be followed by boolean expression.");
            }

            int id = s.generate_label_id();

            s.write_op("    pop     %s", "eax");
            s.write_op("    cmp     %s, %d", "eax", 0);
            s.write_op("    je      else_block%d", id);

            m_then_block->translate(s);
            s.write_op("    jmp     if_end%d", id);
            s.write_op("else_block%d:", id);

            if (m_else_block) {
                m_else_block->translate(s);
            }

            s.write_op("if_end%d:", id);

            return Type(T_NIL_TYPE);
        }
};

class AstWhile: public Ast {
    public:
        struct Token m_t;
        Ast* m_condition;
        Ast* m_while_block;
    public:
        AstWhile(struct Token t, Ast* condition, Ast* while_block): m_t(t), m_condition(condition), m_while_block(while_block) {}
        std::string to_string() {
            return "while";
        }
        Type translate(Semant& s) {
            int id = s.generate_label_id();

            s.write_op("    jmp     %s%d", "while_condition", id);
            s.write_op("%s%d:", "while_block", id);
            m_while_block->translate(s);
            s.write_op("%s%d:", "while_condition", id);
            Type condition_type = m_condition->translate(s);
            if (condition_type.m_dtype != T_BOOL_TYPE) {
                ems_add(&ems, m_t.line, "Type Error: 'while' keyword must be followed by boolean expression.");
            }
            s.write_op("    pop     %s", "eax");
            s.write_op("    cmp     %s, %d", "eax", 1);
            s.write_op("    je      while_block%d", id);

            return Type(T_NIL_TYPE);
        }
};


class AstCall: public Ast {
    public:
        struct Token m_symbol;
        std::vector<Ast*> m_args;
    public:
        AstCall(struct Token symbol, std::vector<Ast*> args): m_symbol(symbol), m_args(args) {}
        std::string to_string() {
            return "call";
        }

        Type translate(Semant& s) {
            //TODO: look up function in environment (to get return type, parameter types, etc)
            //evaluate arguments in reverse order and type check
            //call symbol
            //offset stack pointer to remove arguments
            //push eax onto stack (return value)
            //return ret_type
            return Type(T_NIL_TYPE);
        }
};

class AstParam: public Ast {
    public:
        struct Token m_symbol;
        struct Token m_dtype;
    public:
        AstParam(struct Token symbol, struct Token dtype): m_symbol(symbol), m_dtype(dtype) {}
        std::string to_string() {
            return "param";
        }
        Type translate(Semant& s) {
            //TODO: how do I want to do this?
            //Maybe nothing...
            //Really only need to restrict function body from declaring variable with same symbol as formal parameter
            //and check parameters before environment symbols when referencing/reassigning 
            return Type(T_NIL_TYPE);
        }
};

class AstReturn: public Ast {
    public:
        struct Token m_return;
        Ast* m_expr;
    public:
        AstReturn(struct Token ret, Ast* expr): m_return(ret), m_expr(expr) {}
        std::string to_string() {
            return "return";
        }
        Type translate(Semant& s) {
            if (!s.m_compiling_fun) {
                ems_add(&ems, m_return.line, "Synax Error: 'return' can only be used inside a function definition.");
                return Type(T_NIL_TYPE);
            }

            Type ret_type = m_expr->translate(s);
            AstFunDef* n = dynamic_cast<AstFunDef*>(s.m_compiling_fun);
            if (ret_type.m_dtype != n->m_ret_type.type) {
                ems_add(&ems, m_return.line, "Synax Error: return data type does not match function return type.");
            }

            s.write_op("    pop     %s", "eax");
            
            return Type(T_RET_TYPE);
        }
};

#endif //TMD_AST_HPP
