#ifndef OPTIMIZER_HPP
#define OPTIMIZER_HPP

#include <vector>
#include "tac.hpp"

class Optimizer {
    public:
        void constant_folding(std::vector<TacQuad>* quads);
};


#endif //OPTIMIZER_HPP
