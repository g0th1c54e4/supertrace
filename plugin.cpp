#include "plugin.h"
#include <fstream>
#include <filesystem>
#include <windows.h>
#include "trace.h"
#include "blockdef.h"
#include "util.h"
#include "dbgutil.h"

std::string g_traceFilePath = ""; // The tracking file path of the current operation
MetaBlock traceInfo{};

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

void setInitInfo() {
    traceInfo.process.id = DbgGetProcessId();
    traceInfo.process.handle = reinterpret_cast<uint64_t>(DbgGetProcessHandle());
    traceInfo.process.peb = DbgGetPebAddress(DbgGetProcessId());

    THREADLIST threadlist{};
    DbgGetThreadList(&threadlist);
    for (int i = 0; i < threadlist.count; i++) {
        ThreadInfo threadItem{};
        threadItem.id = threadlist.list[i].BasicInfo.ThreadId;
        threadItem.handle = reinterpret_cast<uint64_t>(threadlist.list[i].BasicInfo.Handle);
        threadItem.teb = threadlist.list[i].BasicInfo.ThreadLocalBase;
        threadItem.entry = threadlist.list[i].BasicInfo.ThreadStartAddress;
        threadItem.cip = threadlist.list[i].ThreadCip;
        threadItem.suspendCount = threadlist.list[i].SuspendCount;
        threadItem.waitReason = enumCast<THREADWAITREASON, ThreadWaitReason>(threadlist.list[i].WaitReason);
        threadItem.priority = enumCast<THREADPRIORITY, ThreadPriority>(threadlist.list[i].Priority);
        threadItem.lastError = threadlist.list[i].LastError;
        threadItem.time.user = COMBINE_U32_TO_U64(threadlist.list[i].UserTime.dwHighDateTime, threadlist.list[i].UserTime.dwLowDateTime);
        threadItem.time.kernel = COMBINE_U32_TO_U64(threadlist.list[i].KernelTime.dwHighDateTime, threadlist.list[i].KernelTime.dwLowDateTime);
        threadItem.time.creation = COMBINE_U32_TO_U64(threadlist.list[i].CreationTime.dwHighDateTime, threadlist.list[i].CreationTime.dwLowDateTime);
        threadItem.cycles = threadlist.list[i].Cycles;
        threadItem.name = threadlist.list[i].BasicInfo.threadName;

        traceInfo.threads.push_back(threadItem);
    }

    ListInfo symbols;
    Script::Symbol::GetList(&symbols);
    Script::Symbol::SymbolInfo* symbolInfos(static_cast<Script::Symbol::SymbolInfo*>(symbols.data));
    for (int i = 0; i < symbols.count; i++) {
        SymbolInfo symbolItem{};
        symbolItem.mod = symbolInfos[i].mod;
        symbolItem.name = symbolInfos[i].name;
        symbolItem.type = enumCast<Script::Symbol::SymbolType, SymbolType>(symbolInfos[i].type);
        symbolItem.rva = symbolInfos[i].rva;
        duint modbase = Script::Module::BaseFromName(symbolInfos[i].mod);
        if (modbase == 0) { throw std::exception(); }
        symbolItem.va = modbase + symbolInfos[i].rva;

        traceInfo.symbols.push_back(symbolItem);
    }
    BridgeFree(symbolInfos);

    MEMMAP map{};
    DbgMemMap(&map);
    for (int i = 0; i < map.count; i++) {
        MemoryMapInfo memMapInfo{};
        memMapInfo.addr = reinterpret_cast<uint64_t>(map.page[i].mbi.BaseAddress);
        memMapInfo.size = map.page[i].mbi.RegionSize;
        memMapInfo.protect = map.page[i].mbi.Protect;
        memMapInfo.state = map.page[i].mbi.State;
        memMapInfo.type = map.page[i].mbi.Type;
        memMapInfo.allocation.base = reinterpret_cast<uint64_t>(map.page[i].mbi.AllocationBase);
        memMapInfo.allocation.protect = map.page[i].mbi.AllocationProtect;

        memMapInfo.dataValid = false;
        if (map.page[i].mbi.Protect != 0) {
            memMapInfo.dataValid = true;
            memMapInfo.data.resize(map.page[i].mbi.RegionSize);
            try {
                duint sizeRead = 0;
                Script::Memory::Read(reinterpret_cast<duint>(map.page[i].mbi.BaseAddress), memMapInfo.data.data(), memMapInfo.data.size(), &sizeRead);
                if (sizeRead != memMapInfo.data.size())
                    throw std::exception();
            }
            catch (std::exception excep) {
                memMapInfo.dataValid = false;
                memMapInfo.data.clear();
            }
        }

        traceInfo.memoryMaps.push_back(memMapInfo);
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
            modSecItem.addr = modSecInfos[i].addr;
            modSecItem.size = modSecInfos[i].size;
            modItem.sections.push_back(modSecItem);
        }
        BridgeFree(modSecInfos);

        traceInfo.modules.push_back(modItem);
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
    traceInfo = {}; // clear traceInfo
    traceInfo.supertrace.version = COMBINE_U16_TO_U32(SUPERTRACE_BLOCK_VERSION_MAJOR, SUPERTRACE_BLOCK_VERSION_MINOR);
    traceInfo.supertrace.createTimeStamp = getCurrentTime();
    std::string exepath;
    exepath.reserve(MAX_PATH);
    if (Script::Module::GetMainModulePath(exepath.data())) {
        dprintf("Main module path: %s\n", exepath.c_str());
        traceInfo.exeBuf = readFileToVector(exepath);
    }
    else {
        dprintf("Get main module path failed.\n");
    }
    
    setInitInfo();
    dprintf("Initial information has been saved.\n");
}
void traceStopCB(CBTYPE cbType, PLUG_CB_STOPTRACE* callbackInfo) {
    dprintf("Result saved: %s\n", g_traceFilePath.c_str());
    auto serialMetaBlock = serializeJson(traceInfo);
    insert_userblock(g_traceFilePath, METABLOCK_TYPE, serialMetaBlock.size(), serialMetaBlock.data());
}

void traceExecuteCB(CBTYPE cbType, PLUG_CB_TRACEEXECUTE* callbackInfo) {
    // See #2837

    /*
    * When you trigger the breakpoint here, you need to manually execute the tracking again
    */
    if (cmpMemBytes(callbackInfo->cip, { 0x0F, 0xA2 })) { // CPUID (0x0FA2)
        duint cpuidNextAddr = callbackInfo->cip + 2;
        cpuidNextAddrSet.insert(cpuidNextAddr);
        Script::Debug::SetBreakpoint(cpuidNextAddr);
    }
}

void breakPointCB(CBTYPE cbType, PLUG_CB_BREAKPOINT* callbackInfo) {
    duint cpuidNextAddr = callbackInfo->breakpoint->addr;
    if (cpuidNextAddrSet.contains(cpuidNextAddr)) {
        dprintf("You have encountered the CPUID. At this point, the debugger should be in the breakpoint hit state. please manually continue to execute the trace.\n");
        Script::Debug::DeleteBreakpoint(cpuidNextAddr);
    }
}

bool pluginInit(PLUG_INITSTRUCT* initStruct) {
    dprintf("Block version: %d.%d\n", SUPERTRACE_BLOCK_VERSION_MAJOR, SUPERTRACE_BLOCK_VERSION_MINOR);
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