#ifndef CONTROL_FLOW_GRAPH_HPP
#define CONTROL_FLOW_GRAPH_HPP

#include <vector>
#include "tac.hpp"

class BasicBlock {
    public:
        enum class Color {
            Black,
            Grey,
            White
        };
    public:
        std::string m_label;
        int m_begin;
        int m_end; //index of one quad after end of block
        Color m_mark = Color::Black;
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
        BasicBlock* get_block(const std::string& name);
    private:
        void generate_inter_block_edges(const std::vector<TacQuad>& quads);
        void generate_inter_procedural_edges(const std::vector<TacQuad>& quads);
    public:
        std::vector<BasicBlock> m_blocks;
        std::vector<BlockEdge> m_edges;
};


#endif //CONTROL_FLOW_GRAPH_HPP
