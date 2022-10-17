#ifndef SEMANT_HPP
#define SEMANT_HPP

#include <string>
#include <vector>
#include <unordered_map>
#include <stdarg.h>

#include "reserved_word.hpp"
#include "token.hpp"
#include "lexer.hpp"
#include "ast.hpp"
#include "parser_new.hpp"
#include "error.hpp"

class Semant {

    class Symbol {
        std::string m_symbol;
        enum TokenType m_dtype;
    };
    class Environment {
        Environment* m_next;
        std::unordered_map<std::string, Symbol> m_symbols;
    };

    class TmdParser: public ParserNew {
        public:
            class AstBinary: public Ast {
                public:
                    struct Token m_op;
                    Ast* m_left;
                    Ast* m_right;
                    AstBinary(struct Token op, Ast* left, Ast* right): m_op(op), m_left(left), m_right(right) {}
                    std::string to_string() {
                        return "binary";
                    }
                    enum TokenType translate(std::vector<uint8_t>& buf) {
                        enum TokenType ret_type = T_NIL_TYPE;
                        enum TokenType left_type = m_left->translate(buf);
                        enum TokenType right_type = m_right->translate(buf);
                        if (left_type != right_type) {
                            ems_add(&ems, m_op.line, "Type Error: Left and right types don't match!");
                            ret_type = T_NIL_TYPE; //TODO: Should be error type to avoid same error message cascade 
                        } else if (m_op.type == T_PLUS || m_op.type == T_MINUS || m_op.type == T_SLASH || m_op.type == T_STAR) {
                            ret_type = T_INT_TYPE;
                        } else {
                            ret_type = T_BOOL_TYPE;
                        }

                        Semant::write_op(buf, "    pop     %s", "ebx");
                        Semant::write_op(buf, "    pop     %s", "eax");

                        switch (m_op.type) {
                            case T_PLUS:
                                Semant::write_op(buf, "    add     %s, %s", "eax", "ebx");
                                break;
                            case T_MINUS:
                                Semant::write_op(buf, "    sub     %s, %s", "eax", "ebx");
                                break;
                            case T_STAR:
                                Semant::write_op(buf, "    imul    %s, %s", "eax", "ebx");
                                break;
                            case T_SLASH:
                                Semant::write_op(buf, "    cdq"); //sign extend eax into edx
                                Semant::write_op(buf, "    idiv    %s", "ebx");
                                break;
                            case T_LESS:
                                Semant::write_op(buf, "    cmp     %s, %s", "eax", "ebx");
                                Semant::write_op(buf, "    setl    %s", "al"); //set byte to 0 or 1
                                Semant::write_op(buf, "    movzx   %s, %s", "eax", "al");
                                break;
                            case T_GREATER:
                                Semant::write_op(buf, "    cmp     %s, %s", "eax", "ebx");
                                Semant::write_op(buf, "    setg    %s", "al"); //set byte to 0 or 1
                                Semant::write_op(buf, "    movzx   %s, %s", "eax", "al");
                                break;
                            case T_LESS_EQUAL:
                                Semant::write_op(buf, "    cmp     %s, %s", "eax", "ebx");
                                Semant::write_op(buf, "    setle   %s", "al"); //set byte to 0 or 1
                                Semant::write_op(buf, "    movzx   %s, %s", "eax", "al");
                                break;
                            case T_GREATER_EQUAL:
                                Semant::write_op(buf, "    cmp     %s, %s", "eax", "ebx");
                                Semant::write_op(buf, "    setge   %s", "al");; //set byte to 0 or 1
                                Semant::write_op(buf, "    movzx   %s, %s", "eax", "al");
                                break;
                            case T_EQUAL_EQUAL:
                                Semant::write_op(buf, "    cmp     %s, %s", "eax", "ebx");
                                Semant::write_op(buf, "    sete    %s", "al"); //set byte to 0 or 1
                                Semant::write_op(buf, "    movzx   %s, %s", "eax", "al");
                                break;
                            case T_NOT_EQUAL:
                                Semant::write_op(buf, "    cmp     %s, %s", "eax", "ebx");
                                Semant::write_op(buf, "    setne   %s", "al"); //set byte to 0 or 1
                                Semant::write_op(buf, "    movzx   %s, %s", "eax", "al");
                                break;
                            case T_AND:
                                Semant::write_op(buf, "    and     %s, %s", "eax", "ebx");
                                break;
                            case T_OR:
                                Semant::write_op(buf, "    or      %s, %s", "eax", "ebx");
                                break;
                            default:
                                ems_add(&ems, m_op.line, "Translate Error: Binary operator not recognized.");
                                break;
                        }

                        Semant::write_op(buf, "    push    %s", "eax");
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
                    enum TokenType translate(std::vector<uint8_t>& buf) {
                        enum TokenType ret_type = m_right->translate(buf);

                        write_op(buf, "    pop     %s", "eax");

                        if (ret_type == T_INT_TYPE) {
                            write_op(buf, "    neg     %s", "eax");
                        } else if (ret_type == T_BOOL_TYPE) {
                            write_op(buf, "    xor     %s, %d", "eax", 1); 
                        }

                        write_op(buf, "    push    %s", "eax");
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
                    enum TokenType translate(std::vector<uint8_t>& buf) {
                        if (m_lexeme.type == T_INT) {
                            Semant::write_op(buf, "    push    %.*s", m_lexeme.len, m_lexeme.start);
                        } else if (m_lexeme.type == T_TRUE) {
                            Semant::write_op(buf, "    push    %d", 1);
                        } else if (m_lexeme.type == T_FALSE) {
                            Semant::write_op(buf, "    push    %d", 0);
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
                    enum TokenType translate(std::vector<uint8_t>& buf) {
                        enum TokenType arg_type = m_arg->translate(buf);

                        if (arg_type == T_INT_TYPE) {
                            Semant::write_op(buf, "    call    %s", "_print_int");
                        } else if (arg_type == T_BOOL_TYPE) {
                            Semant::write_op(buf, "    call    %s", "_print_bool");
                        }
                        Semant::write_op(buf, "    add     %s, %d", "esp", 4);

                        Semant::write_op(buf, "    push    %s", "0xa");
                        Semant::write_op(buf, "    call    %s", "_print_char");
                        Semant::write_op(buf, "    add     %s, %d", "esp", 4);
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
                    enum TokenType translate(std::vector<uint8_t>& buf) {
                        return T_INT_TYPE;
                    }
            };

            class AstDeclVar: public Ast {
                public:
                    struct Token m_symbol;
                    struct Token m_type;
                    Ast* m_value;
                    AstDeclVar(struct Token symbol, struct Token type, Ast* value): m_symbol(symbol), m_type(type), m_value(value) {}
                    std::string to_string() {
                        return "declvar";
                    }
                    enum TokenType translate(std::vector<uint8_t>& buf) {
                        return T_INT_TYPE;
                    }
            };

            class AstGetVar: public Ast {
                public:
                    struct Token m_symbol;
                    AstGetVar(struct Token symbol): m_symbol(symbol) {}
                    std::string to_string() {
                        return "getvar";
                    }
                    enum TokenType translate(std::vector<uint8_t>& buf) {
                        return T_INT_TYPE;
                    }
            };

            class AstSetVar: public Ast {
                public:
                    struct Token m_symbol;
                    Ast* m_value;
                    AstSetVar(struct Token symbol, Ast* value): m_symbol(symbol), m_value(value) {}
                    std::string to_string() {
                        return "setvar";
                    }
                    enum TokenType translate(std::vector<uint8_t>& buf) {
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
                    enum TokenType translate(std::vector<uint8_t>& buf) {
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
                    enum TokenType translate(std::vector<uint8_t>& buf) {
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
                    enum TokenType translate(std::vector<uint8_t>& buf) {
                        return T_INT_TYPE;
                    }
            };

        public:
            std::vector<Ast*> parse_tokens(const std::vector<struct Token>& tokens) override {
                m_tokens = tokens;
                m_current = 0;

                while (!end_of_tokens()) {
                    m_nodes.push_back(parse_stmt());
                }
                return m_nodes;
            }
        private:
            Ast *parse_group();
            Ast *parse_literal();
            Ast *parse_unary();
            Ast *parse_factor();
            Ast *parse_term();
            Ast *parse_inequality();
            Ast *parse_equality();
            Ast *parse_and();
            Ast *parse_or();
            Ast *parse_assignment();
            Ast *parse_expr();
            Ast *parse_block();
            Ast *parse_stmt();
    };

    public:
        inline static std::vector<struct ReservedWordNew> m_reserved_words {{
            {"print", T_PRINT},
            {"int", T_INT_TYPE},
            {"bool", T_BOOL_TYPE},
            {"true", T_TRUE},
            {"false", T_FALSE},
            {"and", T_AND},
            {"or", T_OR},
            {"if", T_IF},
            {"elif", T_ELIF},
            {"else", T_ELSE},
            {"while", T_WHILE}
        }};
    public:
        std::string m_code = "";
        std::vector<struct Token> m_tokens = std::vector<struct Token>();
        int m_current = 0;
        std::vector<Ast*> m_nodes = std::vector<Ast*>();
        Lexer m_lexer;
        TmdParser m_parser;
        std::vector<uint8_t> m_buf;
    public:
        void generate_asm(const std::string& input_file, const std::string& output_file);
    private: 
        void read(const std::string& input_file);
        void lex();
        void parse();
        void translate();
        static void write_op(std::vector<uint8_t>& buf, const char* format, ...);
        void write(const std::string& output_file);
};

#endif //SEMANT_HPP
