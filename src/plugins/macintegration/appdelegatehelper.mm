#import "appdelegatehelper.h"
#include <QTimer>

static IPluginManager *pluginManager = NULL;
static bool shutdownRequested = false;

@implementation AppDelegateHelper

// impl of methods to exchange with NSApplicationDelegate subclass
- (NSApplicationTerminateReply)applicationShouldTerminate:(NSApplication *)sender
{
	NSLog(@"applicationShouldTerminate: called!");
	if (shutdownRequested || !pluginManager)
	{
		return [self applicationShouldTerminateOriginal:sender];
	}
	else
	{
		QTimer::singleShot(1, pluginManager->instance(), SLOT(quit()));
		shutdownRequested = true;
		return NSTerminateCancel;
	}
}

// dummy impl of renamed original methods of NSApplicationDelegate subclass
- (NSApplicationTerminateReply)applicationShouldTerminateOriginal:(NSApplication *)sender
{
	Q_UNUSED(sender)
	return NSTerminateNow;
}

@end

void setPluginManager(IPluginManager *manager)
{
	pluginManager = manager;
}
