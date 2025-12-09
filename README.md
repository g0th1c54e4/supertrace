# Introduction

This is an x64dbg plugin aimed at appending additional information to trace files (.trace32 & .trace64). With it, the trace files can include helpful data for reverse engineering research on execution traces, such as symbols, loaded modules, memory maps (including corresponding binary data), thread information, etc.

For details, see **[blockdef.h](https://github.com/g0th1c54e4/supertrace/blob/master/blockdef.h)**

> **It is recommended to use [x64dbg-trace-parser](https://github.com/g0th1c54e4/x64dbg-trace-parser) for parsing**


# Attention
The extra information is appended by introducing user-defined blocks (see [Block Format](https://deepwiki.com/x64dbg/x64dbg/5.1-trace-recording-and-analysis#block-format)). Therefore, it does not interfere with the native parsing behavior of x64dbg TraceFileReader. In other words, opening the trace file in x64dbg and viewing the trace records continues to work normally.

# Usage
Once the plugin is successfully loaded, it collects extra information whenever trace recording starts (**StartTraceRecording** \\ **StartRunTrace**). When tracing stops, the collected information will be written into the trace file.
The serialized information is archived in JSON format, so you can parse and use it anywhere with a JSON interpreter.

# Third-party Library
[cereal](https://github.com/USCiLab/cereal)：A C++11 library for serialization
