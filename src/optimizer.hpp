#ifndef OPTIMIZER_HPP
#define OPTIMIZER_HPP

#include <vector>
#include "tac.hpp"

class Optimizer {
    public:
        void fold_constants(std::vector<TacQuad>* quads);
        void merge_adjacent_store_fetch(std::vector<TacQuad>* quads);
        void simplify_algebraic_identities(std::vector<TacQuad>* quads);
};


#endif //OPTIMIZER_HPP
