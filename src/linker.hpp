#ifndef LINKER_HPP
#define LINKER_HPP

#include <vector>
#include <string>

class Linker {
    private:
        std::vector<uint8_t> m_buf;
        std::vector<std::vector<uint8_t>> m_obj_bufs;
    private:
        std::vector<uint8_t> read_binary(const std::string& input_file);
        void write(const std::string& output_file);
    public:
        void link(const std::vector<std::string>& input_files, const std::string& output_file);
};


#endif //LINKER_HPP
