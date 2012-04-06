#ifndef PJSIPCALLBACKS_H
#define PJSIPCALLBACKS_H

#include <pjsua.h>

void registerModuleCallbacks(pjsip_module & module);
void registerCallbacks(pjsua_callback & cb);
void registerFrameCallbacks(pjmedia_vid_dev_myframe & myframe);

#endif // PJSIPCALLBACKS_H
