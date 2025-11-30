#include "plugin.h"

bool pluginInit(PLUG_INITSTRUCT* initStruct) {
    dprintf("Hello World\n");
    return true;
}
void pluginStop() {
    
}

PLUG_EXPORT void CBMENUENTRY(CBTYPE cbType, PLUG_CB_MENUENTRY* info) {

}

void pluginSetup() {

}
