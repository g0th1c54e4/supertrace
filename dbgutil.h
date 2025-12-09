#ifndef DBGUTIL_H
#define DBGUTIL_H

#include "plugin.h"

duint get_stack_base(uint32_t threadID);
duint get_stack_limit(uint32_t threadID);
double calculateEntropy(const duint addr, size_t size);

#endif