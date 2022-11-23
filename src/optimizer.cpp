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
                case TacT::Plus:
                    result = left + right;
                    break;
                case TacT::Minus:
                    result = left - right;
                    break;
                case TacT::Star:
                    result = left * right;
                    break;
                case TacT::Slash:
                    result = left / right;
                    break;
                case TacT::Less:
                    result = left < right;
                    break;
                case TacT::EqualEqual:
                    result = left == right;
                    break;
                case TacT::And:
                    result = left && right;
                    break;
                case TacT::Or:
                    result = left || right;
                    break;
                    /*
                case T_GREATER:
                    result = left > right;
                    break;
                case T_LESS_EQUAL:
                    result = left <= right;
                    break;
                case T_GREATER_EQUAL:
                    result = left >= right;
                    break;
                case T_NOT_EQUAL:
                    result = left != right;
                    break;*/
                default:
                    continue;
            }
            (*quads)[i] = TacQuad(q.m_target, std::to_string(result), "", TacT::Assign);
        }
    }
}

void Optimizer::merge_adjacent_store_fetch(std::vector<TacQuad>* quads) {
    for (int i = 0; i < quads->size() - 1; i++) {
        TacQuad* q1 = &((*quads)[i]);
        TacQuad* q2 = &((*quads)[i + 1]);

        if (q2->m_op == TacT::Assign && q2->m_opd1 == q1->m_target) {
            q2->m_opd1 = q1->m_opd1;
            q2->m_opd2 = q1->m_opd2;
            q2->m_op = q1->m_op;
            q1->m_target = "";
            q1->m_opd1 = "";
            q1->m_opd2 = "";
            q1->m_op = TacT::EmptyQuad;
        }

    }
}

void Optimizer::simplify_algebraic_identities(std::vector<TacQuad>* quads) {
    for (int i = 0; i < quads->size() - 1; i++) {
        TacQuad* q = &((*quads)[i]);

        switch (q->m_op) {
            case TacT::Plus:
                if (q->m_opd1 == "0") {
                    q->m_opd1 = q->m_opd2;
                    q->m_opd2 = "";
                    q->m_op = TacT::Assign;
                } else if (q->m_opd2 == "0") {
                    q->m_opd2 = "";
                    q->m_op = TacT::Assign;
                }
                break;
            case TacT::Minus:
                if (q->m_opd1 == q->m_opd2) {
                    q->m_opd1 == "0";
                    q->m_opd2 == "";
                    q->m_op = TacT::Assign;
                } else if(q->m_opd2 == "0") {
                    q->m_opd2 = "";
                    q->m_op = TacT::Assign;
                }
                break;
            case TacT::Star:
                if (q->m_opd1 == "1") {
                    q->m_opd1 = q->m_opd2;
                    q->m_opd2 = "";
                    q->m_op = TacT::Assign;
                } else if (q->m_opd2 == "1") {
                    q->m_opd2 = "";
                    q->m_op = TacT::Assign;
                } else if (q->m_opd1 == "0" || q->m_opd2 == "0") {
                    q->m_opd1 = "0";
                    q->m_opd2 = "";
                    q->m_op = TacT::Assign;
                }
                break;
            case TacT::Slash:
                if (q->m_opd2 == "1") {
                    q->m_opd2 = "";
                    q->m_op = TacT::Assign;
                } else if (q->m_opd1 == q->m_opd2) {
                    q->m_opd1 = "1";
                    q->m_opd2 = "";
                    q->m_op = TacT::Assign;
                }
                break;
            case TacT::And:
                if (q->m_opd1 == q->m_opd2) {
                    q->m_opd2 = "";
                    q->m_op = TacT::Assign;
                }
                break;
            case TacT::Or:
                if (q->m_opd1 == q->m_opd2) {
                    q->m_opd2 = "";
                    q->m_op = TacT::Assign;
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
        if (q.m_op == TacT::CondGoto && q.m_opd2 == (*labels)[i + 1]) {
            q.m_opd2 = ""; 
        }
    }
}


