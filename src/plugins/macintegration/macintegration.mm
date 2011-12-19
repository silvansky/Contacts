#include <Cocoa/Cocoa.h>

#define COCOA_CLASSES_DEFINED
#include "macintegration_p.h"

#import <objc/runtime.h>

#include <QImage>
#include <QPixmap>
#include <QApplication>
#include <QWidget>

//#include "growl/GrowlApplicationBridge.h"
#include <Growl.h>

#include <Sparkle.h>

#include <utils/log.h>
#include <utils/macwidgets.h>

// 24 hours
#define UPDATE_CHECK_INTERVAL (60*60*24)

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
		//NSLog(@"growl agent init... installed: %d running: %d delegate: %@", [GrowlApplicationBridge isGrowlInstalled], [GrowlApplicationBridge isGrowlRunning], [GrowlApplicationBridge growlDelegate]);
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
	Q_UNUSED(clickContext)
	//NSLog(@"Growl notify timed out! id: %@", (NSNumber*)clickContext);
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
static SUUpdater * sparkleUpdater = nil;

MacIntegrationPrivate::MacIntegrationPrivate() :
	QObject(NULL)
{
	// dock click handler
	Class cls = [[[NSApplication sharedApplication] delegate] class];
	if (!class_addMethod(cls, @selector(applicationShouldHandleReopen:hasVisibleWindows:), (IMP) dockClickHandler, "v@:"))
		LogError("MacIntegrationPrivate::MacIntegrationPrivate() : class_addMethod failed!");

	// growl agent
	growlAgent = [[GrowlAgent alloc] init];

	// sparkle updater
	sparkleUpdater = [[SUUpdater alloc] init];
	[sparkleUpdater setAutomaticallyChecksForUpdates: YES];
	[sparkleUpdater setSendsSystemProfile: YES];
	[sparkleUpdater setUpdateCheckInterval: UPDATE_CHECK_INTERVAL];
	[sparkleUpdater setFeedURL: [NSURL URLWithString: @"https://update.rambler.ru/contacts/mac.xml"]];
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
	CGImageRelease(ref);
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
NSLog(@"Notification: %@ | %@", nsTitle, nsText);
	if ([GrowlApplicationBridge isGrowlRunning])
	{
		[GrowlApplicationBridge notifyWithTitle: nsTitle description: nsText notificationName: nsType iconData: [nsIcon TIFFRepresentation] priority: 0 isSticky: NO clickContext: nsId identifier: [NSString stringWithFormat:@"ID%d", id]];
	}
	else
	{
		NSLog(@"Notification: %@ | %@", nsTitle, nsText);
		if ([GrowlApplicationBridge isGrowlInstalled])
			LogError("Growl installed, but not running!");
		else
			LogError("Growl is not installed!");
	}

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
		{
			NSLog(@"Error opening Growl preference pane at %@. Possibly, Growl isn\' installed. Trying Growl 1.3.x bundle...", growlCommonPath);
			ok = [[NSWorkspace sharedWorkspace] openURL: [NSURL fileURLWithPath:@"/Applications/Growl.app"]];
			if (!ok)
				NSLog(@"Error opening Growl.app! Growl isn\'t installed?");
		}
	}
}

static NSColor * gFrameColor = nil;
static NSColor * gTitleColor = nil;

@interface DrawHelper : NSObject
{
}

- (float)roundedCornerRadius;
- (void)drawRectOriginal:(NSRect)rect;
- (void)_drawTitleStringOriginalIn: (NSRect) rect withColor: (NSColor *) color;
- (NSWindow*)window;
- (id)_displayName;
- (NSRect)bounds;

- (void)drawRect:(NSRect)rect;
- (void) _drawTitleStringIn: (NSRect) rect withColor: (NSColor *) color;
@end

@implementation DrawHelper

#define CLIP_METHOD2

- (void)drawRect:(NSRect)rect
{
	// Call original drawing method
	[self drawRectOriginal:rect];
	//[self _setTextShadow:NO];

	NSRect titleRect;

	// Build clipping path : intersection of frame clip (bezier path with rounded corners) and rect argument
#ifndef CLIP_METHOD2
	NSRect windowRect = [[self window] frame];
	windowRect.origin = NSMakePoint(0, 0);

	float cornerRadius = [self roundedCornerRadius];
	[[NSBezierPath bezierPathWithRoundedRect:windowRect xRadius:cornerRadius yRadius:cornerRadius] addClip];
	[[NSBezierPath bezierPathWithRect:rect] addClip];
	titleRect = NSMakeRect(0, 0, windowRect.size.width, windowRect.size.height);
#else
	NSRect brect = [self bounds];
//	[[NSColor clearColor] set];
//	NSRectFill(brect);
//	NSRectFill(rect);

	float radius = [self roundedCornerRadius];
	NSBezierPath *path = [[NSBezierPath alloc] init];
	NSPoint topMid = NSMakePoint(NSMidX(brect), NSMaxY(brect));
	NSPoint topLeft = NSMakePoint(NSMinX(brect), NSMaxY(brect));
	NSPoint topRight = NSMakePoint(NSMaxX(brect), NSMaxY(brect));
	NSPoint bottomRight = NSMakePoint(NSMaxX(brect), NSMinY(brect));

	[path moveToPoint: topMid];
	[path appendBezierPathWithArcFromPoint: topRight
		toPoint: bottomRight
		radius: radius];
	[path appendBezierPathWithArcFromPoint: bottomRight
		toPoint: brect.origin
		radius: radius];
	[path appendBezierPathWithArcFromPoint: brect.origin
		toPoint: topLeft
		radius: radius];
	[path appendBezierPathWithArcFromPoint: topLeft
		toPoint: topRight
		radius: radius];
	[path closePath];

	[path addClip];
	[path release];
	titleRect = NSMakeRect(0, 0, brect.size.width, brect.size.height);
#endif

	CGContextRef context = (CGContextRef)[[NSGraphicsContext currentContext] graphicsPort];
	CGContextSetBlendMode(context, kCGBlendModeMultiply);

	// background
	if (gFrameColor == nil)
	{
		NSLog(@"frame color is nil, setting default");
		gFrameColor = [[NSColor colorWithCalibratedRed: (65 / 255.0) green: (70 / 255.0) blue: (77 / 255.0) alpha: 1.0] retain];
	}

	[gFrameColor set];

#ifndef CLIP_METHOD2
	[[NSBezierPath bezierPathWithRect:rect] fill];
#else
	[[NSBezierPath bezierPathWithRect:rect] fill];
#endif

	CGContextSetBlendMode(context, kCGBlendModeCopy);
	// draw title text
	[self _drawTitleStringIn: titleRect withColor: nil];
}

- (void)_drawTitleStringIn: (NSRect) rect withColor: (NSColor *) color
{
	Q_UNUSED(color)
	if (!gTitleColor)
		gTitleColor = [[NSColor colorWithCalibratedRed: .6 green: .6 blue: .6 alpha: 1.0] retain];
	[self _drawTitleStringOriginalIn: rect withColor: gTitleColor];
}

@end

@class NSThemeFrame;

void MacIntegrationPrivate::installCustomFrame()
{
	id _class = [NSThemeFrame class];

	// Exchange drawRect:
	Method m0 = class_getInstanceMethod([DrawHelper class], @selector(drawRect:));
	class_addMethod(_class, @selector(drawRectOriginal:), method_getImplementation(m0), method_getTypeEncoding(m0));

	Method m1 = class_getInstanceMethod(_class, @selector(drawRect:));
	Method m2 = class_getInstanceMethod(_class, @selector(drawRectOriginal:));

	method_exchangeImplementations(m1, m2);

	// Exchange _drawTitleStringIn:withColor:
	Method m3 = class_getInstanceMethod([DrawHelper class], @selector(_drawTitleStringIn:withColor:));
	class_addMethod(_class, @selector(_drawTitleStringOriginalIn:withColor:), method_getImplementation(m3), method_getTypeEncoding(m3));

	Method m4 = class_getInstanceMethod(_class, @selector(_drawTitleStringIn:withColor:));
	Method m5 = class_getInstanceMethod(_class, @selector(_drawTitleStringOriginalIn:withColor:));

	method_exchangeImplementations(m4, m5);
}

void MacIntegrationPrivate::setCustomBorderColor(const QColor & frameColor)
{
	if (gFrameColor)
		[gFrameColor release];
	gFrameColor = [[NSColor colorWithCalibratedRed: frameColor.redF() green: frameColor.greenF() blue: frameColor.blueF() alpha: frameColor.alphaF()] retain];
	foreach (QWidget * w, QApplication::topLevelWidgets())
		w->update();
}

void MacIntegrationPrivate::setCustomTitleColor(const QColor & titleColor)
{
	if (gTitleColor)
		[gTitleColor release];
	gTitleColor = [[NSColor colorWithCalibratedRed: titleColor.redF() green: titleColor.greenF() blue: titleColor.blueF() alpha: titleColor.alphaF()] retain];
	foreach (QWidget * w, QApplication::topLevelWidgets())
		w->update();
}

void MacIntegrationPrivate::setWindowMovableByBackground(QWidget * window, bool movable)
{
	[[nsViewFromWidget(window) window] setMovableByWindowBackground: (movable ? YES : NO)];
}

void MacIntegrationPrivate::requestAttention()
{
	[NSApp requestUserAttention: NSInformationalRequest];
}
