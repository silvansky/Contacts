#ifndef APPDELEGATEHELPER_H
#define APPDELEGATEHELPER_H

#import <Cocoa/Cocoa.h>
#import <interfaces/ipluginmanager.h>

@interface AppDelegateHelper : NSObject
{
}

// methods to exchange with NSApplicationDelegate subclass
- (NSApplicationTerminateReply)applicationShouldTerminate:(NSApplication *)sender;

// renamed original methods of NSApplicationDelegate subclass
- (NSApplicationTerminateReply)applicationShouldTerminateOriginal:(NSApplication *)sender;

@end

void setPluginManager(IPluginManager *manager);

#endif // APPDELEGATEHELPER_H
