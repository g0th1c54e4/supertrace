#include "plugin.h"
#include <fstream>
#include <filesystem>
#include <windows.h>
#include "trace.h"
#include "blockdef.h"
#include "util.h"

std::string g_traceFilePath = ""; // The tracking file path of the current operation
MetaBlock trace_info{};

std::function cmpMemBytes = [](duint addr, std::vector<uint8_t> bytes) -> bool {
    size_t size = bytes.size();
    if (size == 0) return false;
    try {
        if (size == 1) {
            if (Script::Memory::ReadByte(addr) != *reinterpret_cast<uint8_t*>(bytes.data())) return false;
        }
        if (size == 2) {
            if (Script::Memory::ReadWord(addr) != *reinterpret_cast<uint16_t*>(bytes.data())) return false;
        }
        if (size == 4) {
            if (Script::Memory::ReadDword(addr) != *reinterpret_cast<uint32_t*>(bytes.data())) return false;
        }
        if (size == 8) {
            if (Script::Memory::ReadQword(addr) != *reinterpret_cast<uint64_t*>(bytes.data())) return false;
        }
            
        std::vector<uint8_t> checkBytes(size);
        duint readSize = 0;
        Script::Memory::Read(addr, checkBytes.data(), checkBytes.size(), &readSize);
        if (size != readSize) return false;
        if (std::memcmp(checkBytes.data(), bytes.data(), size) != 0)
            return false;
    }
    catch (std::exception excep) {
        return false;
    }
    return true;
};

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

void set_init_info() {
    trace_info.process.id = DbgGetProcessId();
    trace_info.process.handle = (uint64_t)DbgGetProcessHandle();
    trace_info.process.peb = DbgGetPebAddress(DbgGetProcessId());

    THREADLIST threadlist{};
    DbgGetThreadList(&threadlist);
    for (int i = 0; i < threadlist.count; i++) {
        ThreadInfo threadItem{};
        threadItem.id = threadlist.list[i].BasicInfo.ThreadId;
        threadItem.handle = (uint64_t)threadlist.list[i].BasicInfo.Handle;
        threadItem.teb = threadlist.list[i].BasicInfo.ThreadLocalBase;
        threadItem.entry = threadlist.list[i].BasicInfo.ThreadStartAddress;
        threadItem.cip = threadlist.list[i].ThreadCip;
        threadItem.suspendCount = threadlist.list[i].SuspendCount;
        threadItem.waitReason = (uint32_t)threadlist.list[i].WaitReason;
        threadItem.priority = (uint32_t)threadlist.list[i].Priority;
        threadItem.lastError = threadlist.list[i].LastError;
        threadItem.userTime.lowDateTime = threadlist.list[i].UserTime.dwLowDateTime;
        threadItem.userTime.highDateTime = threadlist.list[i].UserTime.dwHighDateTime;
        threadItem.kernelTime.lowDateTime = threadlist.list[i].KernelTime.dwLowDateTime;
        threadItem.kernelTime.highDateTime = threadlist.list[i].KernelTime.dwHighDateTime;
        threadItem.creationTime.lowDateTime = threadlist.list[i].CreationTime.dwLowDateTime;
        threadItem.creationTime.highDateTime = threadlist.list[i].CreationTime.dwHighDateTime;
        threadItem.cycles = threadlist.list[i].Cycles;
        threadItem.name = threadlist.list[i].BasicInfo.threadName;
        threadItem.isCurrentThread = (threadlist.CurrentThread == i);
        
        threadItem.stack.base = get_stack_base((uint64_t)threadlist.list[i].BasicInfo.ThreadId);
        threadItem.stack.limit = get_stack_limit((uint64_t)threadlist.list[i].BasicInfo.ThreadId);

        trace_info.threads.push_back(threadItem);
    }

    ListInfo symbols;
    Script::Symbol::GetList(&symbols);
    Script::Symbol::SymbolInfo* symbolInfos(static_cast<Script::Symbol::SymbolInfo*>(symbols.data));
    for (int i = 0; i < symbols.count; i++) {
        SymbolInfo symbolItem{};
        symbolItem.mod = symbolInfos[i].mod;
        symbolItem.name = symbolInfos[i].name;
        switch (symbolInfos[i].type) {
        case Script::Symbol::Function:
            symbolItem.type = SymbolType::Function; break;
        case Script::Symbol::Import:
            symbolItem.type = SymbolType::Import; break;
        case Script::Symbol::Export:
            symbolItem.type = SymbolType::Export; break;
        }
        symbolItem.rva = symbolInfos[i].rva;
        duint modbase = Script::Module::BaseFromName(symbolInfos[i].mod);
        if (modbase == 0) { throw std::exception(); }
        symbolItem.va = modbase + symbolInfos[i].rva;

        trace_info.symbols.push_back(symbolItem);
    }
    BridgeFree(symbolInfos);

    MEMMAP map{};
    DbgMemMap(&map);
    for (int i = 0; i < map.count; i++) {
        MemoryMapInfo memMapInfo{};
        memMapInfo.address = (uint64_t)map.page[i].mbi.BaseAddress;
        memMapInfo.size = map.page[i].mbi.RegionSize;
        memMapInfo.protect = map.page[i].mbi.Protect;
        memMapInfo.state = map.page[i].mbi.State;
        memMapInfo.type = map.page[i].mbi.Type;
        memMapInfo.allocation.base = (uint64_t)map.page[i].mbi.AllocationBase;
        memMapInfo.allocation.protect = map.page[i].mbi.AllocationProtect;

        memMapInfo.dataValid = false;
        if (map.page[i].mbi.Protect != 0) {
            memMapInfo.dataValid = true;
            memMapInfo.data.resize(map.page[i].mbi.RegionSize);
            duint sizeRead = 0;
            try {
                Script::Memory::Read((duint)map.page[i].mbi.BaseAddress, memMapInfo.data.data(), memMapInfo.data.size(), &sizeRead);
                if (sizeRead != memMapInfo.data.size())
                    throw std::exception();
            }
            catch (std::exception excep) {
                memMapInfo.dataValid = false;
                memMapInfo.data.clear();
            }
        }

        trace_info.memoryMaps.push_back(memMapInfo);
    }

    ListInfo mods;
    Script::Module::GetList(&mods);
    duint mainModBase = Script::Module::GetMainModuleBase();
    Script::Module::ModuleInfo* modInfos(static_cast<Script::Module::ModuleInfo*>(mods.data));
    for (int i = 0; i < mods.count; i++) {
        std::string decoded_path = DECODE(modInfos[i].path);

        ModuleInfo modItem{};
        modItem.name = modInfos[i].name;
        modItem.path = modInfos[i].path;
        modItem.base = modInfos[i].base;
        modItem.size = modInfos[i].size;
        modItem.entry = modInfos[i].entry;
        modItem.sectionCount = modInfos[i].sectionCount;
        modItem.isMainModule = (modInfos[i].base == mainModBase);

        ListInfo modSecs;
        Script::Module::SectionListFromAddr(modInfos[i].base, &modSecs);
        Script::Module::ModuleSectionInfo* modSecInfos(static_cast<Script::Module::ModuleSectionInfo*>(modSecs.data));
        for (int i = 0; i < modSecs.count; i++) {
            ModuleSectionInfo modSecItem{};
            modSecItem.name = modSecInfos[i].name;
            modSecItem.address = modSecInfos[i].addr;
            modSecItem.size = modSecInfos[i].size;
            modSecItem.entropy = calculateEntropy(modSecInfos[i].addr, modSecInfos[i].size);
            modItem.sections.push_back(modSecItem);
        }
        BridgeFree(modSecInfos);

        trace_info.modules.push_back(modItem);
    }
    BridgeFree(modInfos);

    // Function
    // XREF
    // Handle
    // SEH chain
}

// -----------------------
std::unordered_set<duint> cpuidNextAddrSet;
void traceStartCB(CBTYPE cbType, PLUG_CB_STARTTRACE* callbackInfo) {
    cpuidNextAddrSet.clear();
    g_traceFilePath = callbackInfo->traceFilePath;
    trace_info = {};
    trace_info.trace.createTimeStamp = getCurrentTime().first;
    std::string exepath;
    exepath.reserve(MAX_PATH);
    if (Script::Module::GetMainModulePath(exepath.data())) {
        dprintf("Main module path: %s\n", exepath.c_str());
        trace_info.trace.exeBuf = readFileToVector(exepath);
    }
    else {
        dprintf("Get main module path failed.\n");
    }
    
    set_init_info();
    dprintf("Initial information has been saved.\n");
}
void traceStopCB(CBTYPE cbType, PLUG_CB_STOPTRACE* callbackInfo) {
    dprintf("Result saved: %s\n", g_traceFilePath.c_str());
    auto serialMetaBlock = serializeJson(trace_info);
    insert_userblock(g_traceFilePath, METABLOCK_TYPE, serialMetaBlock.size(), serialMetaBlock.data());
}

void traceExecuteCB(CBTYPE cbType, PLUG_CB_TRACEEXECUTE* callbackInfo) {
    // See #2837
    if (cmpMemBytes(callbackInfo->cip, { 0x0F, 0xA2 })) { // Check 'CPUID' (0x0FA2)
        duint cpuidNextAddr = callbackInfo->cip + 2;
        cpuidNextAddrSet.insert(cpuidNextAddr);
        Script::Debug::SetBreakpoint(cpuidNextAddr);
    }
}

void breakPointCB(CBTYPE cbType, PLUG_CB_BREAKPOINT* callbackInfo) {
    duint cpuidNextAddr = callbackInfo->breakpoint->addr;
    if (cpuidNextAddrSet.contains(cpuidNextAddr)) {
        Script::Debug::DeleteBreakpoint(cpuidNextAddr);
    }
}

bool pluginInit(PLUG_INITSTRUCT* initStruct) {
    return true;
}

void pluginStop() {
    _plugin_unregistercallback(pluginHandle, CB_STARTTRACE);
    _plugin_unregistercallback(pluginHandle, CB_STOPTRACE);
    _plugin_unregistercallback(pluginHandle, CB_TRACEEXECUTE);
    _plugin_unregistercallback(pluginHandle, CB_BREAKPOINT);
}

void pluginSetup() {
    _plugin_registercallback(pluginHandle, CB_STARTTRACE, reinterpret_cast<CBPLUGIN>(traceStartCB));
    _plugin_registercallback(pluginHandle, CB_STOPTRACE, reinterpret_cast<CBPLUGIN>(traceStopCB));
    _plugin_registercallback(pluginHandle, CB_TRACEEXECUTE, reinterpret_cast<CBPLUGIN>(traceExecuteCB));
    _plugin_registercallback(pluginHandle, CB_BREAKPOINT, reinterpret_cast<CBPLUGIN>(breakPointCB));
}