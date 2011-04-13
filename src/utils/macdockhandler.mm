#include <Cocoa/Cocoa.h>
#include "macdockhandler.h"
#import <objc/runtime.h>

#include <QDebug>

void dockClickHandler(id self, SEL _cmd)
{
	emit MacDockHandler::instance()->emitClick();
}

MacDockHandler * MacDockHandler::_instance = NULL;

MacDockHandler::MacDockHandler() :
	QObject(NULL)
{
	Class cls = [[[NSApplication sharedApplication] delegate] class];
	if (class_addMethod(cls, @selector(applicationShouldHandleReopen:hasVisibleWindows:), (IMP) dockClickHandler, "v@:"))
		qDebug() << "class_addMethod ok";
	else
		qDebug() << "class_addMethod failed!";
}

MacDockHandler * MacDockHandler::instance()
{
	if (!_instance)
		_instance = new MacDockHandler;
	return _instance;
}

void MacDockHandler::emitClick()
{
	emit dockIconClicked();
}
