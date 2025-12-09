# 介绍

这是一款旨在为跟踪文件(.trace32 & .trace64) 追加额外信息的 x64dbg 插件，使用它即可让跟踪文件包含诸如符号、存储模块列表、内存映射（含对应二进制数据）、线程列表等针对跟踪轨迹的逆向工程研究会有所帮助的信息。

```cpp
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
```

> **推荐使用 [x64dbg-trace-parser](https://github.com/g0th1c54e4/x64dbg-trace-parser) 进行解析工作**

# 请注意
在跟踪文件中追加额外信息的原理是添加了用户自定义块（参见 [Block Format](https://deepwiki.com/x64dbg/x64dbg/5.1-trace-recording-and-analysis#block-format)），因此它不会影响 x64dbg TraceFileReader 的原生解析行为。换句话说，从 x64dbg 打开跟踪文件并查看指令轨迹记录依旧是正常的。

# 使用
当插件被成功加载后，每当你开始进行跟踪记录(**StartTraceRecording** \\ **StartRunTrace**)，插件会在开始记录额外信息。跟踪记录结束后，这些信息将会一并写入至文件中。
序列化后的信息使用JSON格式进行归档，由此你可自由在任何地方使用JSON解释器进行解析利用。

# 第三方库
[cereal](https://github.com/USCiLab/cereal)：A C++11 library for serialization
