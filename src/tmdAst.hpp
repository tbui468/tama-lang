#ifndef TMD_AST_HPP
#define TMD_AST_HPP

#include <cstring>

#include "ast.hpp"
#include "semant.hpp"
#include "x86_frame.hpp"
#include "symbol.hpp"

class AstBinary: public Ast {
    public:
        struct Token m_op;
        Ast* m_left;
        Ast* m_right;
        AstBinary(struct Token op, Ast* left, Ast* right): m_op(op), m_left(left), m_right(right) {}
        std::string to_string() {
            return "binary";
        }
        EmitTacResult emit_ir(Semant& s) {
            Type ret_type = Type(T_NIL_TYPE);
            EmitTacResult left_result = m_left->emit_ir(s);
            EmitTacResult right_result = m_right->emit_ir(s);

            if (!left_result.m_type.is_of_type(right_result.m_type)) {
                ems_add(&ems, m_op.line, "Type Error: Left and right types don't match!");
            } else if (m_op.type == T_PLUS || m_op.type == T_MINUS || m_op.type == T_SLASH || m_op.type == T_STAR) {
                ret_type = Type(T_INT_TYPE);
            } else {
                ret_type = Type(T_BOOL_TYPE);
            }

            std::string t = s.get_compiling_frame()->add_temp(ret_type);
            switch (m_op.type) {
                case T_PLUS:
                    s.m_quads.push_back(TacQuad(t, left_result.m_temp, right_result.m_temp, TacT::Plus));
                    break;
                case T_MINUS:
                    s.m_quads.push_back(TacQuad(t, left_result.m_temp, right_result.m_temp, TacT::Minus));
                    break;
                case T_STAR:
                    s.m_quads.push_back(TacQuad(t, left_result.m_temp, right_result.m_temp, TacT::Star));
                    break;
                case T_SLASH:
                    s.m_quads.push_back(TacQuad(t, left_result.m_temp, right_result.m_temp, TacT::Slash));
                    break;
                case T_LESS:
                    s.m_quads.push_back(TacQuad(t, left_result.m_temp, right_result.m_temp, TacT::Less));
                    break;
                case T_EQUAL_EQUAL:
                    s.m_quads.push_back(TacQuad(t, left_result.m_temp, right_result.m_temp, TacT::EqualEqual));
                    break;
                case T_AND:
                    s.m_quads.push_back(TacQuad(t, left_result.m_temp, right_result.m_temp, TacT::And));
                    break;
                case T_OR:
                    s.m_quads.push_back(TacQuad(t, left_result.m_temp, right_result.m_temp, TacT::Or));
                    break;
                //synthesize these other operators
                case T_GREATER: {
                    s.m_quads.push_back(TacQuad(t, right_result.m_temp, left_result.m_temp, TacT::Less));
                    break;
                }
                case T_LESS_EQUAL: {
                    std::string tl = s.get_compiling_frame()->add_temp(ret_type);
                    s.m_quads.push_back(TacQuad(tl, left_result.m_temp, right_result.m_temp, TacT::Less));
                    std::string te = s.get_compiling_frame()->add_temp(ret_type);
                    s.m_quads.push_back(TacQuad(te, left_result.m_temp, right_result.m_temp, TacT::EqualEqual));
                    s.m_quads.push_back(TacQuad(t, tl, te, TacT::Or));
                    break;
                }
                case T_GREATER_EQUAL: {
                    std::string tg = s.get_compiling_frame()->add_temp(ret_type);
                    s.m_quads.push_back(TacQuad(tg, right_result.m_temp, left_result.m_temp, TacT::Less));
                    std::string te = s.get_compiling_frame()->add_temp(ret_type);
                    s.m_quads.push_back(TacQuad(te, left_result.m_temp, right_result.m_temp, TacT::EqualEqual));
                    s.m_quads.push_back(TacQuad(t, tg, te, TacT::Or));
                    break;
                }
                case T_NOT_EQUAL: {
                    std::string tl = s.get_compiling_frame()->add_temp(ret_type);
                    s.m_quads.push_back(TacQuad(tl, left_result.m_temp, right_result.m_temp, TacT::Less));
                    std::string tg = s.get_compiling_frame()->add_temp(ret_type);
                    s.m_quads.push_back(TacQuad(tg, right_result.m_temp, left_result.m_temp, TacT::Less));
                    s.m_quads.push_back(TacQuad(t, tl, tg, TacT::Or));
                    break;
                }
                default:
                    ems_add(&ems, m_op.line, "Translate Error: Binary operator not recognized.");
                    break;
            }

            return {t, ret_type};
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
        EmitTacResult emit_ir(Semant& s) {
            EmitTacResult r = m_right->emit_ir(s);

            std::string t = s.get_compiling_frame()->add_temp(r.m_type);
            if (r.m_type.m_dtype == T_INT_TYPE) {
                s.m_quads.push_back(TacQuad(t, "0", r.m_temp, TacT::Minus));
            } else if (r.m_type.m_dtype == T_BOOL_TYPE) {
                std::string tl = s.get_compiling_frame()->add_temp(r.m_type);
                s.m_quads.push_back(TacQuad(tl, r.m_temp, "1", TacT::Less));
                std::string tg = s.get_compiling_frame()->add_temp(r.m_type);
                s.m_quads.push_back(TacQuad(tg, "1", r.m_temp, TacT::Less));
                s.m_quads.push_back(TacQuad(t, tl, tg, TacT::Or));
            } else {
                ems_add(&ems, m_op.line, "Unary expression does not support that operator.");
            }
            return {t, r.m_type};
        }
};

class AstLiteral: public Ast {
    public:
        struct Token m_lexeme;
        AstLiteral(struct Token lexeme): m_lexeme(lexeme) {}
        std::string to_string() {
            return "literal";
        }
        EmitTacResult emit_ir(Semant& s) {
            std::string t;
            if (m_lexeme.type == T_INT) {
                t = std::string(m_lexeme.start, m_lexeme.len);
            } else if (m_lexeme.type == T_TRUE) {
                t = "1";
            } else if (m_lexeme.type == T_FALSE) {
                t = "0";
            } else if (m_lexeme.type == T_NIL) {
                t = "0";
            } else {
                ems_add(&ems, m_lexeme.line, "Synax Error: Invalid literal");
            }

            Type type = Type(T_NIL_TYPE);
            switch (m_lexeme.type) {
                case T_INT:
                    type =  Type(T_INT_TYPE);
                    break;
                case T_TRUE:
                case T_FALSE:
                    type = Type(T_BOOL_TYPE);
                    break;
                default:
                    break;
            }

            return {t, type};
        }
};

//TODO: Need line info for AstPrint (need the token)
//TODO: should generalize to use AstCall
class AstPrint: public Ast {
    public:
        Ast* m_arg;
        AstPrint(Ast* arg): m_arg(arg) {}
        std::string to_string() {
            return "print";
        }
        EmitTacResult emit_ir(Semant& s) {
            EmitTacResult r = m_arg->emit_ir(s);

            s.m_quads.push_back(TacQuad("", "push_arg", r.m_temp, TacT::PushArg)); 
            if (r.m_type.m_dtype == T_INT_TYPE) {
                s.m_quads.push_back(TacQuad("", "call", "_print_int", TacT::CallNil)); 
            } else if (r.m_type.m_dtype == T_BOOL_TYPE) {
                s.m_quads.push_back(TacQuad("", "call", "_print_bool", TacT::CallNil)); 
            } else {
                //TODO: error message with line info goes here
            }
            s.m_quads.push_back(TacQuad("", "pop_args", std::to_string(4), TacT::PopArgs)); 

            return {"", Type(T_NIL_TYPE)};
        }
};

class AstExprStmt: public Ast {
    public:
        Ast* m_expr;
        AstExprStmt(Ast* expr): m_expr(expr) {}
        std::string to_string() {
            return "exprstmt";
        }
        EmitTacResult emit_ir(Semant& s) {
            EmitTacResult r = m_expr->emit_ir(s);
            return {"", Type(T_NIL_TYPE)};
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
        EmitTacResult emit_ir(Semant& s) {
            return {"", Type(m_dtype.type)};
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
            return "function definition";
        }
        EmitTacResult emit_ir(Semant& s) {
            std::string fun_name = std::string(m_symbol.start, m_symbol.len);
            s.m_frames->insert({fun_name, X86Frame()});

            std::unordered_map<std::string, X86Frame>::iterator it = s.m_frames->find(fun_name);
            std::vector<Type> ptypes = std::vector<Type>();
            int ord_num = 0;

            for (Ast* n: m_params) {
                EmitTacResult r = n->emit_ir(s);
                ptypes.push_back(r.m_type); //NOTE: parameters should NOT emit any code
                AstParam* param = (AstParam*)n;
                it->second.add_parameter_to_frame(std::string(param->m_symbol.start, param->m_symbol.len), r.m_type, ord_num);
                ord_num++;
            }
            
            if (s.m_globals.m_symbols.find(fun_name) != s.m_globals.m_symbols.end()) {
                ems_add(&ems, m_symbol.line, "Syntax Error: Function with name already declared in global scope.");
            } else {
                s.m_globals.m_symbols.insert({fun_name, Symbol(fun_name, "", Type(T_FUN_TYPE, m_ret_type.type, ptypes), 0)});
            }

            s.add_tac_label(fun_name);
            int offset = s.m_quads.size();
            s.m_quads.push_back(TacQuad("", "begin_fun", "", TacT::FunBegin));
            int start_temps = X86Frame::s_temp_counter;

            s.m_compiling_fun = this;
            m_body->emit_ir(s);
            s.m_compiling_fun = nullptr;

            int reserved_stack_variables = X86Frame::s_temp_counter - start_temps;
            s.m_quads[offset].m_opd2 = std::to_string((reserved_stack_variables) * 4);
            s.m_quads.push_back(TacQuad("", "end_fun", "", TacT::FunEnd));


            return {"", Type(T_NIL_TYPE)};
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
        EmitTacResult emit_ir(Semant& s) {
            AstFunDef *f = dynamic_cast<AstFunDef*>(s.m_compiling_fun);
            for (Ast* n: f->m_params) {
                AstParam* p = dynamic_cast<AstParam*>(n);
                if (strncmp(p->m_symbol.start, m_symbol.start, m_symbol.len) == 0) {
                    ems_add(&ems, m_symbol.line, "Syntax Error: Formal parameter already declared using symbol");
                    break;
                }
            }

            EmitTacResult r = m_value->emit_ir(s);

            if (r.m_type.m_dtype != m_type.type) {
                ems_add(&ems, m_symbol.line, "Type Error: Declaration type and assigned value type don't match!");
            }

            if(s.get_compiling_frame()->symbol_defined_in_current_scope(std::string(m_symbol.start, m_symbol.len))) {
                ems_add(&ems, m_symbol.line, "Syntax Error: Local symbol already declared in this scope!");
            }

            std::string local_temp = s.get_compiling_frame()->add_local(std::string(m_symbol.start, m_symbol.len), r.m_type);

            s.m_quads.push_back(TacQuad(local_temp, r.m_temp, "", TacT::Assign));

            return {"", Type(T_NIL_TYPE)};
        }
};

class AstGetSym: public Ast {
    public:
        struct Token m_symbol;
        AstGetSym(struct Token symbol): m_symbol(symbol) {}
        std::string to_string() {
            return "getvar";
        }
        EmitTacResult emit_ir(Semant& s) {
            int arg_offset = -1;
            AstFunDef *f = dynamic_cast<AstFunDef*>(s.m_compiling_fun);
            for (int i = 0; i < int(f->m_params.size()); i++) {
                Ast* n = f->m_params[i];
                AstParam* p = dynamic_cast<AstParam*>(n);
                if (strncmp(p->m_symbol.start, m_symbol.start, m_symbol.len) == 0) {
                    arg_offset = i;
                    break;
                }
            }

            if (arg_offset == -1) { //symbol is local
                Symbol* sym = s.get_compiling_frame()->get_symbol_from_scopes(std::string(m_symbol.start, m_symbol.len));

                if (!sym) {
                    ems_add(&ems, m_symbol.line, "Syntax Error: Variable not declared!");
                    return {"", Type(T_NIL_TYPE)};
                }
                
                return {sym->m_tac_name, sym->m_type};
            } else { //symbol is formal parameter

                Ast* n = f->m_params[arg_offset];
                AstParam* p = dynamic_cast<AstParam*>(n);
                
                return {std::string(p->m_symbol.start, p->m_symbol.len), p->m_dtype.type};
            }
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
        EmitTacResult emit_ir(Semant& s) {
            int arg_offset = -1;
            AstFunDef *f = dynamic_cast<AstFunDef*>(s.m_compiling_fun);
            for (int i = 0; i < int(f->m_params.size()); i++) {
                Ast* n = f->m_params[i];
                AstParam* p = dynamic_cast<AstParam*>(n);
                if (strncmp(p->m_symbol.start, m_symbol.start, m_symbol.len) == 0) {
                    arg_offset = i;
                    break;
                }
            }

            if (arg_offset == -1) { //symbol is a local
                Symbol* sym = s.get_compiling_frame()->get_symbol_from_scopes(std::string(m_symbol.start, m_symbol.len));

                if (!sym) {
                    ems_add(&ems, m_symbol.line, "Syntax Error: Variable not declared!");
                    return {"", Type(T_NIL_TYPE)};
                }

                EmitTacResult r = m_value->emit_ir(s);
                if (!sym->m_type.is_of_type(r.m_type)) {
                    ems_add(&ems, m_symbol.line, "Type Error: Declaration type and assigned value type don't match!");
                    return {"", Type(T_NIL_TYPE)};
                }

                s.m_quads.push_back(TacQuad(sym->m_tac_name, r.m_temp, "", TacT::Assign));

                return {sym->m_tac_name, r.m_type};
            } else { //symbol is a formal parameter
                EmitTacResult r = m_value->emit_ir(s);
                Symbol* sym = s.get_compiling_frame()->get_symbol_from_frame(std::string(m_symbol.start, m_symbol.len));

                Type type = sym->m_type.m_ptypes[arg_offset];
                if (!type.is_of_type(r.m_type)) {
                    ems_add(&ems, m_symbol.line, "Type Error: Formal parameter type and assigned value type don't match!");
                    return {"", Type(T_NIL_TYPE)};
                }

                s.m_quads.push_back(TacQuad(sym->m_name, r.m_temp, "", TacT::Assign));
                return {sym->m_name, r.m_type};
            }
        }
};

class AstBlock: public Ast {
    public:
        std::vector<Ast*> m_stmts;
        AstBlock(std::vector<Ast*> stmts): m_stmts(stmts) {}
        std::string to_string() {
            return "block";
        }
        EmitTacResult emit_ir(Semant& s) {
            s.get_compiling_frame()->begin_scope();

            for (Ast* n: m_stmts) {
                EmitTacResult r = n->emit_ir(s);
            }

            s.get_compiling_frame()->end_scope();

            return {"", Type(T_NIL_TYPE)};
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
        EmitTacResult emit_ir(Semant& s) {
            EmitTacResult cond_r = m_condition->emit_ir(s);
            if (cond_r.m_type.m_dtype != T_BOOL_TYPE) {
                ems_add(&ems, m_t.line, "Syntax Error: 'if' keyword must be followed by boolean expression.");
            }

            std::string true_label = TacQuad::new_label();
            std::string false_label;
            std::string end_label = TacQuad::new_label();

            if (m_else_block) {
                false_label = TacQuad::new_label();
                s.m_quads.push_back(TacQuad(cond_r.m_temp, false_label, true_label, TacT::CondGoto));
            } else {
                s.m_quads.push_back(TacQuad(cond_r.m_temp, end_label, true_label, TacT::CondGoto));
            }

            s.add_tac_label(true_label);
            EmitTacResult then_r = m_then_block->emit_ir(s);
            if (m_else_block) {
                s.m_quads.push_back(TacQuad("", "goto", end_label, TacT::Goto));
            }

            if (m_else_block) {
                s.add_tac_label(false_label);
                EmitTacResult then_r = m_else_block->emit_ir(s);
                s.m_quads.push_back(TacQuad("", "goto", end_label, TacT::Goto));
            }
            s.add_tac_label(end_label);

            return {"", Type(T_NIL_TYPE)};
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
        EmitTacResult emit_ir(Semant& s) {
            std::string cond_l = TacQuad::new_label();
            s.m_quads.push_back(TacQuad("", "goto", cond_l, TacT::Goto));
            s.add_tac_label(cond_l);

            EmitTacResult cond_r = m_condition->emit_ir(s);
            if (cond_r.m_type.m_dtype != T_BOOL_TYPE) {
                ems_add(&ems, m_t.line, "Type Error: 'while' keyword must be followed by boolean expression.");
            }

            std::string body_l = TacQuad::new_label();
            std::string end_l = TacQuad::new_label();
            s.m_quads.push_back(TacQuad(cond_r.m_temp, end_l, body_l, TacT::CondGoto));

            s.add_tac_label(body_l);
            EmitTacResult while_r = m_while_block->emit_ir(s);

            s.m_quads.push_back(TacQuad("", "goto", cond_l, TacT::Goto));
            s.add_tac_label(end_l);

            return {"", Type(T_NIL_TYPE)};
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

        EmitTacResult emit_ir(Semant& s) {
            Symbol *sym = nullptr;

            //type-check if defined in current translation unit
            if (!(sym = s.m_globals.get_symbol(std::string(m_symbol.start, m_symbol.len)))) {
                //check if symbol defined in imports
                for (Semant* import_s: s.m_imports) {
                    if ((sym = import_s->m_globals.get_symbol(std::string(m_symbol.start, m_symbol.len))))
                        break;
                }

                if (!sym) {
                    ems_add(&ems, m_symbol.line, "Syntax Error: Function '%.*s' not defined.", m_symbol.len, m_symbol.start);
                    return {"", Type(T_NIL_TYPE)};
                }
            }

            if (m_args.size() != sym->m_type.m_ptypes.size()) {
                ems_add(&ems, m_symbol.line, "Type Error: Argument count does not match formal parameter count.");
                return {"", Type(T_NIL_TYPE)};
            }

            for (int i = m_args.size() - 1; i >= 0; i--) {
                EmitTacResult r = m_args[i]->emit_ir(s);
                if (!r.m_type.is_of_type(sym->m_type.m_ptypes[i])) {
                    ems_add(&ems, m_symbol.line, "Type Error: Argument type doesn't match formal parameter type.");
                }
                s.m_quads.push_back(TacQuad("", "push_arg", r.m_temp, TacT::PushArg));
            }

            std::string t;
            if (sym->m_type.m_rtype != T_NIL_TYPE) {
                t = s.get_compiling_frame()->add_temp(Type(sym->m_type.m_rtype));
                s.m_quads.push_back(TacQuad(t, "call", std::string(m_symbol.start, m_symbol.len), TacT::CallResult));
            } else {
                t = "";
                s.m_quads.push_back(TacQuad(t, "call", std::string(m_symbol.start, m_symbol.len), TacT::CallNil));
            }

            s.m_quads.push_back(TacQuad("", "pop_args", std::to_string(m_args.size() * 4), TacT::PopArgs));

            return {t, Type(sym->m_type.m_rtype)};

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
        EmitTacResult emit_ir(Semant& s) {
            if (!s.m_compiling_fun) {
                ems_add(&ems, m_return.line, "Synax Error: 'return' can only be used inside a function definition.");
                return {"", Type(T_NIL_TYPE)};
            }

            EmitTacResult r = m_expr->emit_ir(s);
            AstFunDef* n = dynamic_cast<AstFunDef*>(s.m_compiling_fun);
            if (r.m_type.m_dtype != n->m_ret_type.type) {
                ems_add(&ems, m_return.line, "Synax Error: return data type does not match function return type.");
            }

            s.m_quads.push_back(TacQuad("", "return", r.m_temp, TacT::Return));
            return {"", Type(T_NIL_TYPE)};
        }
};

class AstImport: public Ast {
    public: 
        struct Token m_symbol;
    public:
        AstImport(struct Token sym): m_symbol(sym) {}
        std::string to_string() {
            return "import";
        }
        EmitTacResult emit_ir(Semant& s) {
            Semant *new_s = new Semant(nullptr);
            new_s->extract_global_declarations(std::string(m_symbol.start, m_symbol.len) + ".tmd");
            s.m_imports.push_back(new_s);
            return {"", Type(T_NIL_TYPE)};
        }
};

#endif //TMD_AST_HPP
