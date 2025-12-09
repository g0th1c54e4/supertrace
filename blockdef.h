#ifndef BLOCKDEF_H
#define BLOCKDEF_H

constexpr uint16_t SUPERTRACE_BLOCK_VERSION_MAJOR = 1;
constexpr uint16_t SUPERTRACE_BLOCK_VERSION_MINOR = 0;

#include <cereal/types/string.hpp>
#include <cereal/types/vector.hpp>
#include <fstream>

enum class ThreadWaitReason {
    _Executive = 0,
    _FreePage = 1,
    _PageIn = 2,
    _PoolAllocation = 3,
    _DelayExecution = 4,
    _Suspended = 5,
    _UserRequest = 6,
    _WrExecutive = 7,
    _WrFreePage = 8,
    _WrPageIn = 9,
    _WrPoolAllocation = 10,
    _WrDelayExecution = 11,
    _WrSuspended = 12,
    _WrUserRequest = 13,
    _WrEventPair = 14,
    _WrQueue = 15,
    _WrLpcReceive = 16,
    _WrLpcReply = 17,
    _WrVirtualMemory = 18,
    _WrPageOut = 19,
    _WrRendezvous = 20,
    _Spare2 = 21,
    _Spare3 = 22,
    _Spare4 = 23,
    _Spare5 = 24,
    _WrCalloutStack = 25,
    _WrKernel = 26,
    _WrResource = 27,
    _WrPushLock = 28,
    _WrMutex = 29,
    _WrQuantumEnd = 30,
    _WrDispatchInt = 31,
    _WrPreempted = 32,
    _WrYieldExecution = 33,
    _WrFastMutex = 34,
    _WrGuardedMutex = 35,
    _WrRundown = 36,
};

enum class ThreadPriority {
    _PriorityIdle = -15,
    _PriorityAboveNormal = 1,
    _PriorityBelowNormal = -1,
    _PriorityHighest = 2,
    _PriorityLowest = -2,
    _PriorityNormal = 0,
    _PriorityTimeCritical = 15,
    _PriorityUnknown = 0x7FFFFFFF
};

enum class SymbolType {
    Function, //user-defined function
    Import, //IAT entry
    Export //export
};

struct ThreadInfo {
    uint32_t id;
    uint64_t handle;
    uint64_t teb;
    uint64_t entry;
    uint64_t cip;
    uint32_t suspendCount;
    ThreadWaitReason waitReason;
    ThreadPriority priority;
    uint32_t lastError;
    struct {
        uint64_t user;
        uint64_t kernel;
        uint64_t creation;
    } time;
    uint64_t cycles;
    std::string name;

    template <class Archive>
    void serialize(Archive& ar) {
        ar(CEREAL_NVP(id), CEREAL_NVP(handle), CEREAL_NVP(teb), CEREAL_NVP(entry), CEREAL_NVP(cip),
            CEREAL_NVP(suspendCount), CEREAL_NVP(waitReason), CEREAL_NVP(priority),
            CEREAL_NVP(lastError), CEREAL_NVP(time.user), CEREAL_NVP(time.kernel),
            CEREAL_NVP(time.creation), CEREAL_NVP(cycles), CEREAL_NVP(name));
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
        ar(CEREAL_NVP(mod), CEREAL_NVP(name), CEREAL_NVP(type), CEREAL_NVP(rva), CEREAL_NVP(va));
    }
};

struct MemoryMapInfo {
    uint64_t addr;
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
        ar(CEREAL_NVP(addr), CEREAL_NVP(size), CEREAL_NVP(protect), CEREAL_NVP(state), CEREAL_NVP(type),
            CEREAL_NVP(allocation.base), CEREAL_NVP(allocation.protect), CEREAL_NVP(dataValid),
            CEREAL_NVP(data));
    }
};

struct ModuleSectionInfo {
    std::string name;
    uint64_t addr;
    uint64_t size;

    template <class Archive>
    void serialize(Archive& ar) {
        ar(CEREAL_NVP(name), CEREAL_NVP(addr), CEREAL_NVP(size));
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
        ar(CEREAL_NVP(name), CEREAL_NVP(path), CEREAL_NVP(base), CEREAL_NVP(size), CEREAL_NVP(entry),
            CEREAL_NVP(sectionCount), CEREAL_NVP(sections), CEREAL_NVP(isMainModule));
    }
};

#define METABLOCK_TYPE 0x80
struct MetaBlock {
    struct {
        uint32_t version; // (SUPERTRACE_BLOCK_VERSION_MAJOR, SUPERTRACE_BLOCK_VERSION_MINOR)
        uint64_t createTimeStamp;
    } supertrace;

    std::vector<uint8_t> exeBuf;
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
        ar(CEREAL_NVP(supertrace.createTimeStamp), CEREAL_NVP(supertrace.version), CEREAL_NVP(exeBuf), 
            CEREAL_NVP(process.id), CEREAL_NVP(process.handle), CEREAL_NVP(process.peb), CEREAL_NVP(threads),
            CEREAL_NVP(symbols), CEREAL_NVP(memoryMaps), CEREAL_NVP(modules));
    }
};

#endif