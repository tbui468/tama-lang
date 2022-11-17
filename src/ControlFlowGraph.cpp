#include "ControlFlowGraph.hpp"

void ControlFlowGraph::create_basic_blocks(const std::vector<TacQuad>& quads, const std::vector<std::string>& labels) {
    std::string block_label = "";
    int begin_idx;
    for (int i = 0; i < quads.size(); i++) {
        if (block_label != "") {
            std::string l = labels[i];
            if (l == "") continue;

            m_blocks.push_back({block_label, begin_idx, i});
            begin_idx = i;
            block_label = l;

        } else {
            std::string l = labels[i];
            if (l == "") continue;

            begin_idx = i;
            block_label = l;
        }

    }

    m_blocks.push_back({block_label, begin_idx, quads.size()});

    std::cout << "quads: " << quads.size() << std::endl;
    for (BasicBlock b: m_blocks) {
        std::cout << b.m_label << ": " << b.m_begin << "->" << b.m_end << std::endl;
    }
}

void ControlFlowGraph::generate_graph(const std::vector<TacQuad>& quads) {
    for (int i = 0; i < m_blocks.size(); i++) {
        BasicBlock* b = &m_blocks[i];
        for (int j = b->m_begin; j < b->m_end; j++) {
            const TacQuad* q = &quads[j];
            if (q->m_opd1 == "goto") {
                m_edges.push_back({b->m_label, q->m_opd2});
            } else if (q->m_op == T_CONDJUMP) {
                m_edges.push_back({b->m_label, q->m_opd1});
                m_edges.push_back({b->m_label, q->m_opd2});
            }
        }
    }

    std::cout << "Edges" << std::endl;
    for (BlockEdge e: m_edges) {
        std::cout << e.m_from << "->" << e.m_to << std::endl;
    }
}
