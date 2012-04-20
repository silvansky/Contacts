#ifndef PJSIPCALLBACKS_H
#define PJSIPCALLBACKS_H

#include <pjsua.h>

class PJCallbacks
{
public:
	static void registerModuleCallbacks(pjsip_module & module);
	static void registerCallbacks(pjsua_callback & cb);
	static void registerFrameCallbacks(pjmedia_vid_dev_myframe & myframe);
};

#endif // PJSIPCALLBACKS_H
