#include <fstream>
#include <sstream>
#include <iostream>

#include "semant.hpp"
#include "tmdAst.hpp"
#include "error.hpp"

void Semant::generate_asm(const std::string& input_file, const std::string& output_file) {
    read(input_file);
    lex();
    /*
    for (struct Token t: m_tokens) {
        printf("%.*s\n", t.len, t.start);
    }*/
    parse();
    /*
    for (Ast* n: m_nodes) {
        std::cout << n->to_string() << std::endl;
    }*/
    translate();
    write(output_file);
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

void Semant::translate() {
    std::string start = "org     0x08048000\n"
                        "_start:\n"
                        "    mov     ebp, esp\n"
                        "    call    main\n"
                        "    mov     ebx, 0\n"
                        "    mov     eax, 0x01\n"
                        "    int     0x80\n";
    m_buf.insert(m_buf.end(), (uint8_t*)start.data(), (uint8_t*)start.data() + start.size());

    for (Ast* n: m_nodes) {
        n->translate(*this);
    }

    std::ifstream f("../../assembly_test/fun.asm");
    std::stringstream buffer;
    buffer << f.rdbuf();
    std::string lib = buffer.str();
    m_buf.insert(m_buf.end(), (uint8_t*)lib.data(), (uint8_t*)lib.data() + lib.size());
}

void Semant::write_op(const char* format, ...) {
    va_list ap;
    va_start(ap, format);
    char s[256];
    int n = vsprintf(s, format, ap);
    va_end(ap);
    s[n] = '\n';
    s[n + 1] = '\0';
    std::string str(s);
    m_buf.insert(m_buf.end(), (uint8_t*)str.data(), (uint8_t*)str.data() + str.size());
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
    struct Token l_brace = consume_token(T_L_BRACE);
    std::vector<Ast*> stmts;
    while (peek_one().type != T_R_BRACE && peek_one().type != T_EOF) {
        stmts.push_back(parse_stmt());
    }
    struct Token r_brace = consume_token(T_R_BRACE);
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
        struct Token var = consume_token(T_IDENTIFIER);
        consume_token(T_COLON);
        struct Token type = next_token();
        consume_token(T_EQUAL);
        Ast *expr = parse_expr();
        return new AstDeclSym(var, type, expr);
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
    } else if (next.type == T_FUN) {
        consume_token(T_FUN);
        struct Token symbol = consume_token(T_IDENTIFIER);
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
        return new AstFunDef(symbol, params, ret_type, body);
    } else if (next.type == T_RETURN) {
        struct Token ret = next_token();
        Ast* expr = parse_expr();
        return new AstReturn(ret, expr);
    } else {
        return new AstExprStmt(parse_expr());
    }
}

