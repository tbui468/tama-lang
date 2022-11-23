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
        void collapse_cond_jumps(std::vector<TacQuad>* quads, std::vector<std::string>* labels);
        void mark_from_root_label(ControlFlowGraph* cfg, const std::string& label);
        void eliminate_dead_code(ControlFlowGraph* cfg, const std::vector<std::string>& labels);
};


#endif //OPTIMIZER_HPP
