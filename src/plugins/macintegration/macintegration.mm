#include <Cocoa/Cocoa.h>
#include "macintegration_p.h"
#import <objc/runtime.h>

#include <QImage>
#include <QPixmap>

//#include "growl/GrowlApplicationBridge.h"
#include <Growl.h>

#include <utils/log.h>
#include <utils/macwidgets.h>

#include <QDebug>

// helper func

#include <definitions/notificationtypes.h>

static QString resolveGrowlType(const QString & notifyType)
{
	if (notifyType == NNT_CHAT_MESSAGE)
		return "New Message";
	else if (notifyType == NNT_MAIL_NOTIFY)
		return "New E-Mail";
	else if (notifyType == NNT_CONTACT_STATE)
		return "Status Changed";
	else if (notifyType == NNT_CONTACT_MOOD)
		return "Mood Changed";
	else if (notifyType == NNT_BIRTHDAY_REMIND)
		return "Birthday Reminder";
	else if (notifyType == NNT_SUBSCRIPTION)
		return "Subscription Message";
	else
		return "Error";
}

// growl agent

@interface GrowlAgent : NSObject <GrowlApplicationBridgeDelegate>
{
	NSDictionary * registration;
}

- (void) growlNotificationWasClicked:(id)clickContext;
- (void) growlNotificationTimedOut:(id)clickContext;

@end

@implementation GrowlAgent

- (id) init
{
	if ((self = [super init]))
	{
		registration = nil;

		// set self as a growl delegate

		[GrowlApplicationBridge setGrowlDelegate: self];
		NSLog(@"growl agent init... installed: %d running: %d delegate: %@", [GrowlApplicationBridge isGrowlInstalled], [GrowlApplicationBridge isGrowlRunning], [GrowlApplicationBridge growlDelegate]);
	}
	return self;
}

// TODO: make localized
//- (NSDictionary *) registrationDictionaryForGrowl
//{
//	NSLog(@"registrationDictionaryForGrowl");
//	if (!registration)
//	{
//		// init registration dictionary

//		NSString * newMessage = MacIntegrationPrivate::nsStringFromQString(QObject::tr(resolveGrowlType(NNT_CHAT_MESSAGE).toUtf8().constData()));
//		NSString * newEmail = MacIntegrationPrivate::nsStringFromQString(QObject::tr(resolveGrowlType(NNT_MAIL_NOTIFY).toUtf8().constData()));
//		NSString * moodChanged = MacIntegrationPrivate::nsStringFromQString(QObject::tr(resolveGrowlType(NNT_CONTACT_MOOD).toUtf8().constData()));
//		NSString * statusChanged = MacIntegrationPrivate::nsStringFromQString(QObject::tr(resolveGrowlType(NNT_CONTACT_STATE).toUtf8().constData()));
//		NSString * birthdayReminder = MacIntegrationPrivate::nsStringFromQString(QObject::tr(resolveGrowlType(NNT_BIRTHDAY_REMIND).toUtf8().constData()));
//		NSString * error = MacIntegrationPrivate::nsStringFromQString(QObject::tr(resolveGrowlType("Error").toUtf8().constData()));
//		NSString * subscriptionMessage = MacIntegrationPrivate::nsStringFromQString(QObject::tr(resolveGrowlType(NNT_SUBSCRIPTION).toUtf8().constData()));

//		NSLog(@"Init registration dictionary with %@ %@ %@ %@ %@ %@ %@", newMessage, newEmail, moodChanged, statusChanged, birthdayReminder, error, subscriptionMessage);

//		NSArray * allNotifications = [NSArray arrayWithObjects: newMessage, newEmail, moodChanged, statusChanged, birthdayReminder, error, subscriptionMessage, nil];
//		NSArray * defaultNotifications = [NSArray arrayWithObjects: newMessage, newEmail, birthdayReminder, error, subscriptionMessage, nil];

//		[newMessage release];
//		[newEmail release];
//		[moodChanged release];
//		[statusChanged release];
//		[birthdayReminder release];
//		[error release];
//		[subscriptionMessage release];

//		registration = [NSDictionary dictionaryWithObjects: [NSArray arrayWithObjects: allNotifications, defaultNotifications, nil] forKeys: [NSArray arrayWithObjects: GROWL_NOTIFICATIONS_ALL, GROWL_NOTIFICATIONS_DEFAULT, nil]];
//	}
//	return registration;
//}

- (void) growlNotificationWasClicked:(id)clickContext
{
	NSNumber * num = (NSNumber*)clickContext;
	//NSLog(@"Growl notify clicked! id: %@", num);
	MacIntegrationPrivate::instance()->emitGrowlNotifyClick([num intValue]);
	[num release];
}

- (void) growlNotificationTimedOut:(id)clickContext
{
	NSLog(@"Growl notify timed out! id: %@", (NSNumber*)clickContext);
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
	growlAgent = [[GrowlAgent alloc] init];
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
	NSString * nsType = nsStringFromQString(resolveGrowlType(type));
	NSNumber * nsId = [NSNumber numberWithInt: id];
	NSImage * nsIcon = nsImageFromQImage(icon);
	//qDebug() << "Growl notify: " << title << text << type << id;
	//NSLog(@"Growl notify: type: %@ text: %@ title: %@ id: %@", nsType, nsText, nsTitle, nsId);
	[GrowlApplicationBridge notifyWithTitle: nsTitle description: nsText notificationName: nsType iconData: [nsIcon TIFFRepresentation] priority: 0 isSticky: NO clickContext: nsId identifier: [NSString stringWithFormat:@"ID%d", id]];

	[nsTitle release];
	[nsText release];
	[nsType release];
	[nsId release];
	[nsIcon release];
}

void MacIntegrationPrivate::showGrowlPrefPane()
{
	NSString * growlCommonPath = @"/Library/PreferencePanes/Growl.prefPane";
	NSString * growlPath = [NSHomeDirectory() stringByAppendingPathComponent:growlCommonPath];
	BOOL ok = [[NSWorkspace sharedWorkspace] openURL: [NSURL fileURLWithPath:growlPath]];
	if (!ok)
	{
		NSLog(@"Error opening Growl preference pane at %@. Trying %@...", growlPath, growlCommonPath);
		ok = [[NSWorkspace sharedWorkspace] openURL: [NSURL fileURLWithPath:growlCommonPath]];
		if (!ok)
			NSLog(@"Error opening Growl preference pane at %@. Possibly, Growl isn\' installed.", growlCommonPath);
	}
}

@interface DrawHelper : NSObject
{
	NSColor * color;
}

@property (nonatomic, retain) NSColor * color;

- (float)roundedCornerRadius;
- (void)drawRectOriginal:(NSRect)rect;
- (NSWindow*)window;
- (id)_displayName;

- (void)drawRect:(NSRect)rect;
@end

@implementation DrawHelper

@synthesize color;

- (void)drawRect:(NSRect)rect
{
	// Call original drawing method
	[self drawRectOriginal:rect];

	// Build clipping path : intersection of frame clip (bezier path with rounded corners) and rect argument
	//
	NSRect windowRect = [[self window] frame];
	windowRect.origin = NSMakePoint(0, 0);

	float cornerRadius = [self roundedCornerRadius];
	[[NSBezierPath bezierPathWithRoundedRect:windowRect xRadius:cornerRadius yRadius:cornerRadius] addClip];
	[[NSBezierPath bezierPathWithRect:rect] addClip];

	//
	// Draw background image (extend drawing rect : biggest rect dimension become's rect size)
	//
//	NSRect imageRect = windowRect;
//	if (imageRect.size.width > imageRect.size.height)
//	{
//		imageRect.origin.y = -(imageRect.size.width-imageRect.size.height)/2;
//		imageRect.size.height = imageRect.size.width;
//	}
//	else
//	{
//		imageRect.origin.x = -(imageRect.size.height-imageRect.size.width)/2;
//		imageRect.size.width = imageRect.size.height;
//	}
	//[[NSImage imageNamed:NSImageNameActionTemplate] drawInRect:imageRect fromRect:NSZeroRect operation:NSCompositeSourceAtop fraction:0.15];

	//
	// Draw a background color on top of everything
	//
	CGContextRef context = (CGContextRef)[[NSGraphicsContext currentContext] graphicsPort];
	CGContextSetBlendMode(context, kCGBlendModeSourceIn);

	// background
	[[NSColor colorWithSRGBRed: (65 / 255.0) green: (70 / 255.0) blue: (77 / 255.0) alpha: 1.0] set];
	[[NSBezierPath bezierPathWithRect:rect] fill];

	// text
	[[NSColor colorWithSRGBRed: 1.0 green: 1.0 blue: 1.0 alpha: 1.0] set];
	NSFont *font = [NSFont fontWithName:@"Palatino-Roman" size:14.0];

	NSDictionary *attrsDictionary = [NSDictionary dictionaryWithObject:font forKey:NSFontAttributeName];
	NSLog(@"display name: %@", [self _displayName]);
	[[self _displayName] drawAtPoint: NSMakePoint(0, 0) withAttributes: attrsDictionary];
}

@end

void MacIntegrationPrivate::setCustomBorderColor(QWidget * window, const QColor & color)
{
	id nswindow = [nsViewFromWidget(window) window];
	NSLog(@"setCustomBorderColor: done %@", nswindow);


	// Get window's frame view class
	id _class = [[[nswindow contentView] superview] class];
	NSLog(@"setCustomBorderColor: class=%@", _class);

	static DrawHelper * dhelper = [[DrawHelper alloc] init];
	NSColor * nsColor = [NSColor colorWithSRGBRed: color.redF() green: color.greenF() blue: color.blueF() alpha: color.alphaF()];

	dhelper.color = nsColor;

	// Exchange draw rect
	Method m0 = class_getInstanceMethod([dhelper class], @selector(drawRect:));
	class_addMethod(_class, @selector(drawRectOriginal:), method_getImplementation(m0), method_getTypeEncoding(m0));

	Method m1 = class_getInstanceMethod(_class, @selector(drawRect:));
	Method m2 = class_getInstanceMethod(_class, @selector(drawRectOriginal:));

	method_exchangeImplementations(m1, m2);
}

void MacIntegrationPrivate::setWindowMovableByBackground(QWidget * window, bool movable)
{
	[[nsViewFromWidget(window) window] setMovableByWindowBackground: (movable ? YES : NO)];
}
