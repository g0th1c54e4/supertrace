#ifndef PLUGIN_H
#define PLUGIN_H

#include "pluginmain.h"

//functions
bool pluginInit(PLUG_INITSTRUCT* initStruct);
void pluginStop();
void pluginSetup();

#endif