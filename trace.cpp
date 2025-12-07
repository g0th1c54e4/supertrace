#include "trace.h"

void insert_userblock_x64dbg_trace(std::string filename, uint8_t block_type, uint32_t block_size, void* block_data) {
    std::fstream f;

    if (!(block_type >= 0x80 && block_type <= 0xFF)) {
        throw std::exception("Block muse be user-defined.");
    }

    f.open(filename, std::ios::in | std::ios::out | std::ios::binary);
    if (!f.is_open()) {
        throw std::exception("Error opening file.");
    }

    // check magic number 'TRAC' (0x43415254)
    uint32_t magic = 0;
    f.read(reinterpret_cast<char*>(&magic), 4);
    if (magic != 0x43415254) {
        throw std::exception("Error, wrong file format.");
    }

    f.seekp(0, std::ios::end);

    f.write(reinterpret_cast<char*>(&block_type), 1);
    if (!f.good()) throw std::exception("Error writing block_type.");

    uint32_t sz = static_cast<uint32_t>(block_size);
    f.write(reinterpret_cast<char*>(&sz), 4);
    if (!f.good()) throw std::exception("Error writing block_size.");

    f.write(reinterpret_cast<const char*>(block_data), block_size);
    if (!f.good()) throw std::exception("Error writing block_data.");

    f.flush();
    f.close();
}