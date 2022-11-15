#include "optimizer.hpp"
#include "token.hpp"
#include "utility.hpp"
#include <iostream>

void Optimizer::constant_folding(std::vector<TacQuad>* quads) {
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
