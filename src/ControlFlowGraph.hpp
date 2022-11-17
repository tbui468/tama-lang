#ifndef CONTROL_FLOW_GRAPH_HPP
#define CONTROL_FLOW_GRAPH_HPP

#include <vector>
#include "tac.hpp"

class BasicBlock {
    public:
        std::string m_label;
        int m_begin;
        int m_end; //index of one quad after end of block
        BasicBlock(const std::string& label, int begin, int end): m_label(label), m_begin(begin), m_end(end) {}
};

class BlockEdge {
    public:
        std::string m_from;
        std::string m_to;
};


class ControlFlowGraph {
    public:
        void create_basic_blocks(const std::vector<TacQuad>& quads, const std::vector<std::string>& labels);
        void generate_graph(const std::vector<TacQuad>& quads);
    private:
        int get_block_index(const std::string& label);
    public:
        std::vector<BasicBlock> m_blocks;
        std::vector<BlockEdge> m_edges;
};


#endif //CONTROL_FLOW_GRAPH_HPP
