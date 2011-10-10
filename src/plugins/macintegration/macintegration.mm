#include <Cocoa/Cocoa.h>
#include "macintegration_p.h"
#import <objc/runtime.h>

#include <QImage>
#include <QPixmap>

#include "growl/GrowlApplicationBridge.h"

#include <utils/log.h>

// growl agent

@interface GrowlAgent : NSObject <GrowlApplicationBridgeDelegate>
{
	MacIntegrationPrivate * obj;
}

@property (nonatomic, assign) MacIntegrationPrivate * object;

- (void) growlIsReady;
- (void) growlNotificationWasClicked:(id)clickContext;
- (void) growlNotificationTimedOut:(id)clickContext;

@end

@implementation GrowlAgent

@synthesize object=obj;

- (id) initWithObject: (MacIntegrationPrivate *) object
{
	if ((self = [super init]))
	{
		self.object = object;
		[GrowlApplicationBridge setGrowlDelegate: self];
	}
	return self;
}

- (void) growlIsReady
{
	NSLog(@"growl is ready!");
}

- (void) growlNotificationWasClicked:(id)clickContext
{
	NSNumber * num = (NSNumber*)clickContext;
	NSLog(@"Growl notify clicked! id: %@", num);
	self.object->emitGrowlNotifyClick([num intValue]);
	[num release];
}

- (void) growlNotificationTimedOut:(id)clickContext
{
	NSLog(@"Growl notify timed out!");
}

@end

// dock click handler

void dockClickHandler(id self, SEL _cmd)
{
	Q_UNUSED(self)
	Q_UNUSED(_cmd)
	MacIntegrationPrivate::instance()->emitClick();
}

// MacIntegrationPrivate

MacIntegrationPrivate * MacIntegrationPrivate::_instance = NULL;

static GrowlAgent * growlAgent = nil;

MacIntegrationPrivate::MacIntegrationPrivate() :
	QObject(NULL)
{
	Class cls = [[[NSApplication sharedApplication] delegate] class];
	if (!class_addMethod(cls, @selector(applicationShouldHandleReopen:hasVisibleWindows:), (IMP) dockClickHandler, "v@:"))
		LogError("MacIntegrationPrivate::MacIntegrationPrivate() : class_addMethod failed!");
	growlAgent = [[GrowlAgent alloc] initWithObject: this];
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

// warning! nsimage isn't released!
NSImage * MacIntegrationPrivate::nsImageFromQImage(const QImage & image)
{
	CGImageRef ref = QPixmap::fromImage(image).toMacCGImageRef();
	NSImage * nsimg = [[NSImage alloc] initWithCGImage: ref size: NSZeroSize];
	return nsimg;
}

QImage MacIntegrationPrivate::qImageFromNSImage(NSImage * image)
{
	CGImageRef ref = [image CGImageForProposedRect:NULL context:nil hints:nil];
	return QPixmap::fromMacCGImageRef(ref).toImage();
}

// warning! nsstring isn't released!
NSString * MacIntegrationPrivate::nsStringFromQString(const QString & s)
{
	const char * utf8String = s.toUtf8().constData();
	return [[NSString alloc] initWithUTF8String: utf8String];
}

void MacIntegrationPrivate::emitClick()
{
	emit dockClicked();
}

void MacIntegrationPrivate::emitGrowlNotifyClick(int id)
{
	emit growlNotifyClicked(id);
}

void MacIntegrationPrivate::setDockBadge(const QString & badgeText)
{
	NSString * badgeString = nsStringFromQString(badgeText);
	[[NSApp dockTile] setBadgeLabel: badgeString];
	[badgeString release];
}

void MacIntegrationPrivate::postGrowlNotify(const QImage & icon, const QString & title, const QString & text, const QString & type, int id)
{
	NSString * nsTitle = nsStringFromQString(title);
	NSString * nsText = nsStringFromQString(text);
	NSString * nsType = nsStringFromQString(type);
	NSNumber * nsId = [NSNumber numberWithInt: id];
	NSImage * nsIcon = nsImageFromQImage(icon);
	[GrowlApplicationBridge notifyWithTitle: nsTitle description: nsText notificationName: nsType iconData: [nsIcon TIFFRepresentation] priority: 0 isSticky: NO clickContext: nsId identifier: [NSString stringWithFormat:@"ID%d", id]];
}
