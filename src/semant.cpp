#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>

#include "semant.hpp"
#include "tmdAst.hpp"
#include "error.hpp"


void Semant::add_tac_label(const std::string& label) {
    m_tac_labels.resize(m_quads.size(), "");
    m_tac_labels.push_back(label);
}


X86Frame* Semant::get_compiling_frame() {
    AstFunDef *f = dynamic_cast<AstFunDef*>(m_compiling_fun);
    std::string fun_name = std::string(f->m_symbol.start, f->m_symbol.len);
    std::unordered_map<std::string, X86Frame>::iterator it = m_frames->find(fun_name);
    return &(it->second);
}


void Semant::generate_ir(const std::string& input_file, const std::string& output_file) {
    read(input_file);
    lex();
    parse();

    m_quads.push_back(TacQuad("", "entry", "<alignment>", T_NIL));

    translate_to_ir();

    m_tac_labels.resize(m_quads.size(), "");

    for (int i = 0; i < m_quads.size(); i++) {
        const TacQuad& q = m_quads[i];
        const std::string& s = m_tac_labels[i];
        if (s != "") {
            write_ir("%s:", s.c_str());
        }
        write_ir("    %s", q.to_string().c_str()); 
    }

    std::ofstream f(output_file, std::ios::out | std::ios::binary);
    f.write((const char*)m_irbuf.data(), m_irbuf.size());
}

void Semant::read(const std::string& input_file) {
    std::ifstream f(input_file);
    std::stringstream buffer;
    buffer << f.rdbuf();
    m_code = buffer.str();
}

void Semant::lex() {
    m_tokens = m_lexer.lex(m_code, m_reserved_words);
}

void Semant::parse() {
    m_nodes = m_parser.parse_tokens(m_tokens);
}

void Semant::translate_to_ir() {
    for (Ast* n: m_nodes) {
        n->emit_ir(*this);
    }
}

void Semant::write_ir(const char* format, ...) {
    va_list ap;
    va_start(ap, format);
    char s[256];
    int n = vsprintf(s, format, ap);
    va_end(ap);
    s[n] = '\n';
    s[n + 1] = '\0';
    std::string str(s);
    m_irbuf.insert(m_irbuf.end(), (uint8_t*)str.data(), (uint8_t*)str.data() + str.size());
}

int Semant::generate_label_id() {
    int ret = m_label_id_counter;
    m_label_id_counter++;
    return ret;
}

void Semant::write(const std::string& output_file) {
    std::ofstream f(output_file, std::ios::out | std::ios::binary);
    f.write((const char*)m_buf.data(), m_buf.size());
}

/*
 * Tamarind Parser
 */

Ast* Semant::TmdParser::parse_group() {
    consume_token(T_L_PAREN);
    Ast* n = parse_expr();
    consume_token(T_R_PAREN);
    return n;
}

Ast* Semant::TmdParser::parse_literal() {
    struct Token next = peek_one();
    if (next.type == T_L_PAREN) {
        return parse_group();
    } else if (next.type == T_IDENTIFIER && peek_two().type == T_L_PAREN) {
        struct Token sym = next_token();
        consume_token(T_L_PAREN);
        std::vector<Ast*> params = std::vector<Ast*>();
        while (peek_one().type != T_R_PAREN) {
            params.push_back(parse_expr());
            if (peek_one().type == T_COMMA)
               consume_token(T_COMMA); 
        }
        consume_token(T_R_PAREN);
        return new AstCall(sym, params);
    } else if (next.type == T_IDENTIFIER) {
        return new AstGetSym(next_token());
    } else if (next.type == T_INT) {
        return new AstLiteral(next_token());
    } else if (next.type == T_TRUE) {
        return new AstLiteral(next_token());
    } else if (next.type == T_FALSE) {
        return new AstLiteral(next_token());
    } else if (next.type == T_NIL) {
        return new AstLiteral(next_token());
    } else {
        ems_add(&ems, next.line, "Parse Error: Unexpected token.");
        return new AstLiteral(next_token());
    }
}

Ast* Semant::TmdParser::parse_unary() {
    struct Token next = peek_one();
    if (next.type == T_MINUS || next.type == T_NOT) {
        struct Token op = next_token();
        return new AstUnary(op, parse_unary());
    } else {
        return parse_literal();
    }
}

Ast* Semant::TmdParser::parse_factor() {
    Ast *left = parse_unary();

    while (1) {
        struct Token next = peek_one();
        if (next.type != T_STAR && next.type != T_SLASH)
            break;

        struct Token op = next_token();
        Ast *right = parse_unary();
        left = new AstBinary(op, left, right);
    }

    return left;
}

Ast* Semant::TmdParser::parse_term() {
    Ast *left = parse_factor();

    while (1) {
        struct Token next = peek_one();
        if (next.type != T_MINUS && next.type != T_PLUS)
            break;

        struct Token op = next_token();
        Ast *right = parse_factor();
        left = new AstBinary(op, left, right);
    }

    return left;
}

Ast* Semant::TmdParser::parse_inequality() {
    Ast *left = parse_term();

    while (1) {
        struct Token next = peek_one();
        if (next.type != T_LESS && next.type != T_LESS_EQUAL &&
            next.type != T_GREATER && next.type != T_GREATER_EQUAL) {
            break;
        }

        struct Token op = next_token();
        Ast *right = parse_term();
        left = new AstBinary(op, left, right);
    }

    return left;
}

Ast* Semant::TmdParser::parse_equality() {
    Ast *left = parse_inequality();

    while (1) {
        struct Token next = peek_one();
        if (next.type != T_EQUAL_EQUAL && next.type != T_NOT_EQUAL) {
            break;
        }

        struct Token op = next_token();
        Ast *right = parse_inequality();
        left = new AstBinary(op, left, right);
    }

    return left;
}

Ast* Semant::TmdParser::parse_and() {
    Ast *left = parse_equality();

    while (1) {
        struct Token next = peek_one();
        if (next.type != T_AND) {
            break;
        }

        struct Token op = next_token();
        Ast *right = parse_equality();
        left = new AstBinary(op, left, right);
    }

    return left;
}

Ast* Semant::TmdParser::parse_or() {
    Ast *left = parse_and();

    while (1) {
        struct Token next = peek_one();
        if (next.type != T_OR) {
            break;
        }

        struct Token op = next_token();
        Ast *right = parse_and();
        left = new AstBinary(op, left, right);
    }

    return left;
}

Ast* Semant::TmdParser::parse_assignment() {
    if (peek_one().type == T_IDENTIFIER && peek_two().type == T_EQUAL) {
        struct Token var = consume_token(T_IDENTIFIER);
        consume_token(T_EQUAL);
        Ast *expr = parse_expr();
        return new AstSetSym(var, expr);
    } else {
        return parse_or();
    }
}

Ast* Semant::TmdParser::parse_expr() {
    return parse_assignment(); 
}

Ast* Semant::TmdParser::parse_block() {
    consume_token(T_L_BRACE);
    std::vector<Ast*> stmts;
    while (peek_one().type != T_R_BRACE && peek_one().type != T_EOF) {
        stmts.push_back(parse_stmt());
    }
    consume_token(T_R_BRACE);
    return new AstBlock(stmts);
}


Ast* Semant::TmdParser::parse_stmt() {
    struct Token next = peek_one();
    if (next.type == T_PRINT) {
        next_token();
        consume_token(T_L_PAREN);
        Ast* arg = parse_expr();
        consume_token(T_R_PAREN);
        return new AstPrint(arg);
    } else if (next.type == T_IDENTIFIER && peek_two().type == T_COLON) {
        struct Token sym = consume_token(T_IDENTIFIER);
        consume_token(T_COLON);

        if (peek_one().type == T_L_PAREN) {
            consume_token(T_L_PAREN);
            std::vector<Ast*> params;
            while (peek_one().type != T_R_PAREN) {
                struct Token sym = consume_token(T_IDENTIFIER);
                consume_token(T_COLON);
                struct Token type = next_token();
                params.push_back(new AstParam(sym, type));
                if (peek_one().type == T_COMMA) {
                    consume_token(T_COMMA);
                }
            } 
            consume_token(T_R_PAREN);
            consume_token(T_MINUS);
            consume_token(T_GREATER);

            struct Token ret_type = next_token();
            if (ret_type.type == T_NIL) ret_type.type = T_NIL_TYPE;

            Ast* body = parse_block();
            return new AstFunDef(sym, params, ret_type, body);
        } else {
            struct Token type = next_token();
            consume_token(T_EQUAL);
            Ast *expr = parse_expr();
            return new AstDeclSym(sym, type, expr);
        }
    } else if (next.type == T_L_BRACE) {
        return parse_block();
    } else if (next.type == T_IF) {
        struct Token if_token = next_token();
        Ast* condition = parse_expr();
        Ast* then_block = parse_block();
        Ast* else_block = NULL;
        if (peek_one().type == T_ELSE) {
            next_token();
            else_block = parse_block();
        }
        return new AstIf(if_token, condition, then_block, else_block);
    } else if (next.type == T_WHILE) {
        struct Token while_token = next_token();
        Ast* condition = parse_expr();
        Ast* while_block = parse_block();
        return new AstWhile(while_token, condition, while_block);
    } else if (next.type == T_IMPORT) {
        next_token();
        struct Token sym = consume_token(T_IDENTIFIER);
        return new AstImport(sym);
    } else if (next.type == T_RETURN) {
        struct Token ret = next_token();
        Ast* expr = parse_expr();
        return new AstReturn(ret, expr);
    } else {
        return new AstExprStmt(parse_expr());
    }
}

void Semant::extract_global_declarations(const std::string& module_file) {
    read(module_file);
    lex();
    parse();
    for (Ast* n: m_nodes) {
        AstFunDef* f = dynamic_cast<AstFunDef*>(n);
        if (f) {
            //if function definition, grab type info...
            std::string fun_name = std::string(f->m_symbol.start, f->m_symbol.len);
            std::vector<Type> ptypes = std::vector<Type>();

            for (Ast* n: f->m_params) {
                EmitTacResult r = n->emit_ir(*this); //NOTE: this only returns type and does not actually add quads
                ptypes.push_back(r.m_type); //NOTE: parameters should NOT emit any code
            }
            
            if (m_globals.m_symbols.find(fun_name) != m_globals.m_symbols.end()) {
                ems_add(&ems, f->m_symbol.line, "Syntax Error: Function with name already declared in global scope.");
            } else {
                m_globals.m_symbols.insert({fun_name, Symbol(fun_name, "", Type(T_FUN_TYPE, f->m_ret_type.type, ptypes), 0)});
            }
        }
    }
}

