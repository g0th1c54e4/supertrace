#ifndef UTIL_H
#define UTIL_H

#include <string>
#include <chrono>
#include <ctime>
#include <utility>
#include <fstream>
#include <filesystem>
#include <windows.h>

#define COMBINE_U32_TO_U64(high, low)  ( ((uint64_t)(high) << 32) | (uint64_t)(low) )
#define COMBINE_U16_TO_U32(high, low) \
    ( ((uint32_t)(high) << 16) | (uint32_t)(low) )
#define COMBINE_U8_TO_U16(high, low) \
    ( ((uint16_t)(high) << 8) | (uint16_t)(low) )

#define SPLIT_U64_HI(v)  ( (uint32_t)((uint64_t)(v) >> 32) )
#define SPLIT_U64_LO(v)  ( (uint32_t)((uint64_t)(v)) )

#define SPLIT_U32_HI(v) ((uint16_t)((uint32_t)(v) >> 16))
#define SPLIT_U32_LO(v) ((uint16_t)((uint32_t)(v)))

#define SPLIT_U16_HI(v) ((uint8_t)((uint16_t)(v) >> 8))
#define SPLIT_U16_LO(v) ((uint8_t)((uint16_t)(v)))

template<class E1, class E2>
E2 enumCast(E1 e) {
    return static_cast<E2>(static_cast<std::underlying_type_t<E1>>(e));
}

std::wstring Utf8ToWstring(const char* utf8Str);
std::string WstringToAnsi(const std::wstring& wideStr);
#define DECODE(str) WstringToAnsi(Utf8ToWstring(str))

long long getCurrentTime();
std::vector<uint8_t> readFileToVector(const std::string& filepath);
bool saveVectorToFile(const std::vector<uint8_t>& data, const std::string& filePath);

#endif