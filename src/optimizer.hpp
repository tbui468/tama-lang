#ifndef OPTIMIZER_HPP
#define OPTIMIZER_HPP

#include <vector>
#include "tac.hpp"
#include "ControlFlowGraph.hpp"

class Optimizer {
    public:
        void fold_constants(std::vector<TacQuad>* quads);
        void merge_adjacent_store_fetch(std::vector<TacQuad>* quads);
        void simplify_algebraic_identities(std::vector<TacQuad>* quads);
        void eliminate_dead_code(ControlFlowGraph* cfg);
};


#endif //OPTIMIZER_HPP
