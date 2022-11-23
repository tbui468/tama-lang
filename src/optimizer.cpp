#include "optimizer.hpp"
#include "token.hpp"
#include "utility.hpp"
#include <iostream>
#include <stack>

void Optimizer::fold_constants(std::vector<TacQuad>* quads) {
    for (int i = 0; i < quads->size(); i++) {
        TacQuad q = (*quads)[i];
        if (q.m_opd1 == "" || q.m_opd2 == "") continue;


        if (is_int(q.m_opd1) && is_int(q.m_opd2)) {
            char* p1;
            char* p2;
            int left = (int)strtol(q.m_opd1.c_str(), &p1, 10);
            int right = (int)strtol(q.m_opd2.c_str(), &p2, 10);
            int result;
            switch (q.m_op) {
                case T_PLUS:
                    result = left + right;
                    break;
                case T_MINUS:
                    result = left - right;
                    break;
                case T_STAR:
                    result = left * right;
                    break;
                case T_SLASH:
                    result = left / right;
                    break;
                case T_LESS:
                    result = left < right;
                    break;
                case T_GREATER:
                    result = left > right;
                    break;
                case T_LESS_EQUAL:
                    result = left <= right;
                    break;
                case T_GREATER_EQUAL:
                    result = left >= right;
                    break;
                case T_EQUAL_EQUAL:
                    result = left == right;
                    break;
                case T_NOT_EQUAL:
                    result = left != right;
                    break;
                case T_AND:
                    result = left && right;
                    break;
                case T_OR:
                    result = left || right;
                    break;
                default:
                    continue;
            }
            (*quads)[i] = TacQuad(q.m_target, std::to_string(result), "", T_EQUAL);
        }
    }
}

void Optimizer::merge_adjacent_store_fetch(std::vector<TacQuad>* quads) {
    for (int i = 0; i < quads->size() - 1; i++) {
        TacQuad* q1 = &((*quads)[i]);
        TacQuad* q2 = &((*quads)[i + 1]);

        if (q2->m_op == T_EQUAL && q2->m_opd2 == "" && q2->m_opd1 == q1->m_target) {
            q2->m_opd1 = q1->m_opd1;
            q2->m_opd2 = q1->m_opd2;
            q2->m_op = q1->m_op;
            q1->m_target = "";
            q1->m_opd1 = "";
            q1->m_opd2 = "";
            q1->m_op == T_NIL;
        }

    }
}

void Optimizer::simplify_algebraic_identities(std::vector<TacQuad>* quads) {
    for (int i = 0; i < quads->size() - 1; i++) {
        TacQuad* q = &((*quads)[i]);

        switch (q->m_op) {
            case T_PLUS:
                if (q->m_opd1 == "0") {
                    q->m_opd1 = q->m_opd2;
                    q->m_opd2 = "";
                    q->m_op = T_EQUAL;
                } else if (q->m_opd2 == "0") {
                    q->m_opd2 = "";
                    q->m_op = T_EQUAL;
                }
                break;
           case T_MINUS:
                if (q->m_opd1 == q->m_opd2) {
                    q->m_opd1 == "0";
                    q->m_opd2 == "";
                    q->m_op = T_EQUAL;
                } else if(q->m_opd2 == "0") {
                    q->m_opd2 = "";
                    q->m_op = T_EQUAL;
                }
                break;
            case T_STAR:
                if (q->m_opd1 == "1") {
                    q->m_opd1 = q->m_opd2;
                    q->m_opd2 = "";
                    q->m_op = T_EQUAL;
                } else if (q->m_opd2 == "1") {
                    q->m_opd2 = "";
                    q->m_op = T_EQUAL;
                } else if (q->m_opd1 == "0" || q->m_opd2 == "0") {
                    q->m_opd1 = "0";
                    q->m_opd2 = "";
                    q->m_op = T_EQUAL;
                }
                break;
            case T_SLASH:
                if (q->m_opd2 == "1") {
                    q->m_opd2 = "";
                    q->m_op = T_EQUAL;
                } else if (q->m_opd1 == q->m_opd2) {
                    q->m_opd1 = "1";
                    q->m_opd2 = "";
                    q->m_op = T_EQUAL;
                }
                break;
            case T_AND:
                if (q->m_opd1 == q->m_opd2) {
                    q->m_opd2 = "";
                    q->m_op = T_EQUAL;
                }
                break;
            case T_OR:
                if (q->m_opd1 == q->m_opd2) {
                    q->m_opd2 = "";
                    q->m_op = T_EQUAL;
                }
                break;
        }
    }
}

void Optimizer::mark_from_root_label(ControlFlowGraph* cfg, const std::string& label) {
        std::stack<BasicBlock*> greys;
        BasicBlock* main_block = cfg->get_block(label);
        if (main_block) {
            greys.push(main_block);
        }

        while (greys.size() > 0) {
            BasicBlock* cur = greys.top();
            greys.pop();
            for (const BlockEdge& e: cfg->m_edges) {
                if (e.m_from == cur->m_label) {
                    BasicBlock* dst = cfg->get_block(e.m_to);
                    //@note: dst may be nullptr if function is imported
                    if (dst && dst->m_mark == BasicBlock::Color::Black) {
                        dst->m_mark = BasicBlock::Color::Grey;
                        greys.push(dst);
                    }
                }
            }
            cur->m_mark = BasicBlock::Color::White;
        }
}

void Optimizer::eliminate_dead_code(ControlFlowGraph* cfg, const std::vector<std::string>& labels) {
    
    bool is_executable = nullptr != cfg->get_block("main"); 

    if (is_executable) {
        mark_from_root_label(cfg, "_start");
    } else {
        for (BasicBlock& b: cfg->m_blocks) {
            if (labels[b.m_begin][0] != '_') {
                mark_from_root_label(cfg, labels[b.m_begin]);
            }
        }
    }

    std::vector<BasicBlock> temp;
    for (BasicBlock b: cfg->m_blocks) {
        temp.push_back(b);
    }

    cfg->m_blocks.clear();
    for (BasicBlock b: temp) {
        if (b.m_mark == BasicBlock::Color::White) {
            cfg->m_blocks.push_back(b);
        }
    }

}

void Optimizer::collapse_cond_jumps(std::vector<TacQuad>* quads, std::vector<std::string>* labels) {
    for (int i = 0; i < quads->size() - 1; i++) {
        TacQuad& q = (*quads)[i];
        if (q.m_op == T_CONDJUMP && q.m_opd2 == (*labels)[i + 1]) {
            q.m_opd2 = ""; 
        }
    }
}


