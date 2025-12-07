#ifndef SERIALIZE_H
#define SERIALIZE_H

#include <cereal/archives/binary.hpp>
#include <sstream>
#include <vector>

template <typename T>
std::vector<uint8_t> serialize(const T& obj) {
    std::stringstream ss(std::ios::binary | std::ios::in | std::ios::out);
    {
        cereal::BinaryOutputArchive archive(ss);
        archive(obj);
    }
    const std::string& s = ss.str();
    return std::vector<uint8_t>(s.begin(), s.end());
}

template <typename T>
T deserialize(const std::vector<uint8_t>& data) {
    std::string s(data.begin(), data.end());
    std::stringstream ss(s, std::ios::binary | std::ios::in | std::ios::out);

    T obj;
    {
        cereal::BinaryInputArchive archive(ss);
        archive(obj);
    }
    return obj;
}

#endif