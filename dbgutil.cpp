#include "dbgutil.h"

duint get_stack_base(uint32_t threadID) {
    duint teb_base = DbgGetTebAddress(threadID);
    duint result = Script::Memory::ReadDword(teb_base + (1 * sizeof(duint))); // TEB.NtTib.StackBase
    return result;
}
duint get_stack_limit(uint32_t threadID) {
    duint teb_base = DbgGetTebAddress(threadID);
    duint result = Script::Memory::ReadDword(teb_base + (2 * sizeof(duint))); // TEB.NtTib.StackLimit
    // uint32_t result = Script::Memory::ReadDword(teb_base + 0xE0C); // TEB.NtTib.DeallocationStack
    return result;
}
double calculateEntropy(const duint addr, size_t size) {
    if (!addr || size == 0) {
        return 0.0;
    }

    uint8_t* pcalcdata = new std::uint8_t[size];
    duint sizeread = 0;
    Script::Memory::Read(addr, pcalcdata, size, &sizeread);
    if (size != sizeread) { return 0.0; }

    std::vector<size_t> freq(256, 0);
    for (size_t i = 0; i < size; ++i) {
        freq[pcalcdata[i]]++;
    }

    double entropy = 0.0;
    for (size_t i = 0; i < 256; ++i) {
        if (freq[i] == 0) continue;
        double p = static_cast<double>(freq[i]) / size;
        entropy -= p * std::log2(p);  // Shannon
    }

    delete[] pcalcdata;
    return entropy; // bits/byte
}