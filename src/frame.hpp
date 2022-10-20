#ifndef FRAME_HPP
#define FRAME_HPP

class Frame {
    public:
        struct Token m_symbol;
        struct Token m_ret_type;
        std::vector<Ast*> m_params;
    public:
        Frame(struct Token symbol, struct Token ret_type, const std::vector<Ast*>& params):
            m_symbol(symbol), m_ret_type(ret_type), m_params(params) {}
};


#endif //FRAME_HPP
