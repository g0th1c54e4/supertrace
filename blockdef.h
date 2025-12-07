#ifndef BLOCKDEF_H
#define BLOCKDEF_H

// recorder host: x64dbg

#include <cereal/types/string.hpp>
#include <cereal/types/vector.hpp>
#include <fstream>

enum SymbolType {
    Function, //user-defined function
    Import, //IAT entry
    Export //export
};

struct FileTime {
    uint32_t lowDateTime;
    uint32_t highDateTime;

    template <class Archive>
    void serialize(Archive& ar) {
        ar(lowDateTime, highDateTime);
    }
};

struct ThreadInfo {
    uint32_t id;
    uint64_t handle;
    uint64_t teb;
    uint64_t entry;
    uint64_t cip;
    uint32_t suspendCount;
    THREADWAITREASON waitReason;
    THREADPRIORITY priority;
    uint32_t lastError;
    FileTime userTime;
    FileTime kernelTime;
    FileTime creationTime;
    uint64_t cycles;
    std::string name;
    bool isCurrentThread;
    struct {
        uint64_t base;
        uint64_t limit;
    } stack;

    template <class Archive>
    void serialize(Archive& ar) {
        ar(id, handle, teb, entry, cip, suspendCount, waitReason,
            priority, lastError, userTime, kernelTime, creationTime, cycles, name, isCurrentThread, stack.base, stack.limit);
    }
};

struct SymbolInfo {
    std::string mod;
    std::string name;
    SymbolType type;
    uint64_t rva;
    uint64_t va;

    template <class Archive>
    void serialize(Archive& ar) {
        ar(mod, name, type, rva, va);
    }
};

struct MemoryMapInfo {
    uint64_t address;
    uint64_t size;
    uint32_t protect;
    uint32_t state;
    uint32_t type;
    struct {
        uint64_t base;
        uint32_t protect;
    } allocation;

    bool dataValid;
    std::vector<uint8_t> data;

    template <class Archive>
    void serialize(Archive& ar) {
        ar(address, size, protect, state, type, allocation.base, allocation.protect, dataValid, data);
    }
};

struct ModuleSectionInfo {
    std::string name;
    uint64_t address;
    uint64_t size;
    double entropy;

    template <class Archive>
    void serialize(Archive& ar) {
        ar(name, address, size, entropy);
    }
};

struct ModuleInfo {
    std::string name;
    std::string path;
    uint64_t base;
    uint64_t size;
    uint64_t entry;
    uint32_t sectionCount;
    std::vector<ModuleSectionInfo> sections;
    bool isMainModule;

    template <class Archive>
    void serialize(Archive& ar) {
        ar(name, path, base, size, entry, sectionCount, sections, isMainModule);
    }
};

#define METABLOCK_TYPE 0x80
struct MetaBlock {
    struct {
        uint64_t createTimeStamp;
        std::vector<uint8_t> exeBuf;
    } trace;

    struct {
        uint32_t id;
        uint64_t handle;
        uint64_t peb;
    } process;
    std::vector<ThreadInfo> threads;
    std::vector<SymbolInfo> symbols;
    std::vector<MemoryMapInfo> memoryMaps;
    std::vector<ModuleInfo> modules;

    template <class Archive>
    void serialize(Archive& ar) {
        ar(trace.createTimeStamp, trace.exeBuf, process.id, process.handle, process.peb,
            threads, symbols, memoryMaps, modules);
    }
};

#endif