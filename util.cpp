#include "util.h"

std::wstring Utf8ToWstring(const char* utf8Str) {
    if (!utf8Str) return L"";
    int wideLen = MultiByteToWideChar(CP_UTF8, 0, utf8Str, -1, nullptr, 0);
    if (wideLen == 0) return L"";
    std::wstring wideStr(wideLen - 1, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, utf8Str, -1, &wideStr[0], wideLen);
    return wideStr;
}
std::string WstringToAnsi(const std::wstring& wideStr) {
    if (wideStr.empty()) return "";
    int ansiLen = WideCharToMultiByte(CP_ACP, 0,
        wideStr.c_str(), -1,
        nullptr, 0,
        nullptr, nullptr);
    if (ansiLen == 0) return "";

    std::string ansiStr(ansiLen - 1, '\0');
    WideCharToMultiByte(CP_ACP, 0,
        wideStr.c_str(), -1,
        &ansiStr[0], ansiLen,
        nullptr, nullptr);

    return ansiStr;
}
#define DECODE(str) WstringToAnsi(Utf8ToWstring(str))

std::pair<long long, std::string> getCurrentTime() {
    auto now = std::chrono::system_clock::now();
    auto timestamp = std::chrono::duration_cast<std::chrono::seconds>(
        now.time_since_epoch()
    ).count();
    std::time_t t = std::chrono::system_clock::to_time_t(now);
    std::tm tm{};
    localtime_s(&tm, &t); // Windows
    char buffer[64];
    std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &tm);
    return { timestamp, buffer };
}

std::vector<uint8_t> readFileToVector(const std::string& filepath) {
    std::ifstream file(filepath, std::ios::binary | std::ios::ate);

    if (!file.is_open()) {
        throw std::runtime_error("open file failed: " + filepath);
    }

    std::streamsize fileSize = file.tellg();

    if (fileSize == -1) {
        throw std::runtime_error("get file size failed: " + filepath);
    }

    file.seekg(0, std::ios::beg);

    std::vector<uint8_t> buffer(fileSize);

    if (!file.read(reinterpret_cast<char*>(buffer.data()), fileSize)) {
        throw std::runtime_error("read file failed: " + filepath);
    }

    return buffer;
}

bool saveVectorToFile(const std::vector<uint8_t>& data, const std::string& filePath) {
    std::ofstream outFile(filePath, std::ios::binary);
    if (!outFile.is_open()) { return false; }

    outFile.write(reinterpret_cast<const char*>(data.data()), data.size());
    outFile.close();

    return true;
}