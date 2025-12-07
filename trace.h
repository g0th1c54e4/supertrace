#ifndef TRACE_H
#define TRACE_H

#include <string>
#include <fstream>
#include <filesystem>

#include "blockdef.h"
#include "serialize.h"

void insert_userblock_x64dbg_trace(std::string filename, uint8_t block_type, uint32_t block_size, void* block_data);

#endif