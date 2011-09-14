#include <Cocoa/Cocoa.h>
#include "macintegration_p.h"
#import <objc/runtime.h>

#include <QDebug>
#include <utils/log.h>

void dockClickHandler(id self, SEL _cmd)
{
	MacIntegrationPrivate::instance()->emitClick();
}

MacIntegrationPrivate * MacIntegrationPrivate::_instance = NULL;

MacIntegrationPrivate::MacIntegrationPrivate() :
	QObject(NULL)
{
	Class cls = [[[NSApplication sharedApplication] delegate] class];
	if (!class_addMethod(cls, @selector(applicationShouldHandleReopen:hasVisibleWindows:), (IMP) dockClickHandler, "v@:"))
		LogError("MacIntegrationPrivate::MacIntegrationPrivate() : class_addMethod failed!");
}

MacIntegrationPrivate::~MacIntegrationPrivate()
{

}

MacIntegrationPrivate * MacIntegrationPrivate::instance()
{
	if (!_instance)
		_instance = new MacIntegrationPrivate;
	return _instance;
}

void MacIntegrationPrivate::emitClick()
{
	emit dockClicked();
}
