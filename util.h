#ifndef UTIL_H
#define UTIL_H

#include <string>
#include <chrono>
#include <ctime>
#include <utility>
#include <fstream>
#include <filesystem>
#include <windows.h>

std::wstring Utf8ToWstring(const char* utf8Str);
std::string WstringToAnsi(const std::wstring& wideStr);
#define DECODE(str) WstringToAnsi(Utf8ToWstring(str))

std::pair<long long, std::string> getCurrentTime();
std::vector<uint8_t> readFileToVector(const std::string& filepath);
bool saveVectorToFile(const std::vector<uint8_t>& data, const std::string& filePath);

#endif