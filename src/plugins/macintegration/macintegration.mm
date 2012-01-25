#import <Cocoa/Cocoa.h>
#import <objc/runtime.h>

#include <Growl.h>
#include <Sparkle.h>

#define COCOA_CLASSES_DEFINED

#include <QVariant>
#include <QImage>
#include <QPixmap>
#include <QApplication>
#include <QWidget>
#include <QPainter>
#include <QTimer>

#include <utils/log.h>
#include <utils/macwidgets.h>
#include <utils/imagemanager.h>

#include "macintegration_p.h"

#import "drawhelper.h"
#import "dockoverlayhelper.h"

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
static DockOverlayHelper * dockOverlay = nil;

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

	// dock overlay
	dockOverlay = [[DockOverlayHelper alloc] init];

	updateTimer = new QTimer(this);
	updateTimer->setInterval(200);
	updateTimer->setProperty("angle", 0);
	connect(updateTimer, SIGNAL(timeout()), SLOT(onUpdateTimer()));
}

MacIntegrationPrivate::~MacIntegrationPrivate()
{
	[growlAgent release];
	[sparkleUpdater release];
	[dockOverlay release];
}

MacIntegrationPrivate * MacIntegrationPrivate::instance()
{
	if (!_instance)
		_instance = new MacIntegrationPrivate;
	return _instance;
}

void MacIntegrationPrivate::release()
{
	if (_instance)
	{
		delete _instance;
		_instance = NULL;
	}
}

// warning! nsimage isn't released!
NSImage * MacIntegrationPrivate::nsImageFromQImage(const QImage & image)
{
	if (!image.isNull())
	{
		CGImageRef ref = QPixmap::fromImage(image).toMacCGImageRef();
		NSImage * nsimg = [[NSImage alloc] initWithCGImage: ref size: NSZeroSize];
		CGImageRelease(ref);
		return nsimg;
	}
	else
		return nil;
}

QImage MacIntegrationPrivate::qImageFromNSImage(NSImage * image)
{
	if (image)
	{
		CGImageRef ref = [image CGImageForProposedRect:NULL context:nil hints:nil];
		QImage result = QPixmap::fromMacCGImageRef(ref).toImage();
		return result;
	}
	return QImage();
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

void MacIntegrationPrivate::setDockOverlay(const QImage & overlay, Qt::Alignment align, bool showAppIcon)
{
	if (!overlay.isNull())
	{
		NSLog(@"setting overlay...");
		NSImage * nsOverlay = nsImageFromQImage(overlay);
		dockOverlay.overlayImage = nsOverlay;
		dockOverlay.alignment = (int)align;
		dockOverlay.drawAppIcon = showAppIcon ? YES : NO;
		[nsOverlay release];
		[dockOverlay set];
	}
	else
	{
		NSLog(@"unsetting overlay...");
		[dockOverlay unset];
	}
}

void MacIntegrationPrivate::postGrowlNotify(const QImage & icon, const QString & title, const QString & text, const QString & type, int id)
{
	NSString * nsTitle = nsStringFromQString(title);
	NSString * nsText = nsStringFromQString(text);
	NSString * nsType = nsStringFromQString(resolveGrowlType(type));
	NSNumber * nsId = [NSNumber numberWithInt: id];
	NSImage * nsIcon = nsImageFromQImage(icon);
	if (isGrowlRunning())
	{
		[GrowlApplicationBridge notifyWithTitle: nsTitle description: nsText notificationName: nsType iconData: [nsIcon TIFFRepresentation] priority: 0 isSticky: NO clickContext: nsId identifier: [NSString stringWithFormat:@"ID%d", id]];
	}
	else
	{
		NSLog(@"Notification: %@ | %@", nsTitle, nsText);
		if (isGrowlInstalled())
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

bool MacIntegrationPrivate::isGrowlInstalled()
{
	return [GrowlApplicationBridge isGrowlInstalled];
}

bool MacIntegrationPrivate::isGrowlRunning()
{
	return [GrowlApplicationBridge isGrowlRunning];
}

void MacIntegrationPrivate::installCustomFrame()
{
	id frameClass = [NSThemeFrame class];

	// Exchange drawRect:
	Method m0 = class_getInstanceMethod([DrawHelper class], @selector(drawRect:));
	class_addMethod(frameClass, @selector(drawRectOriginal:), method_getImplementation(m0), method_getTypeEncoding(m0));

	Method m1 = class_getInstanceMethod(frameClass, @selector(drawRect:));
	Method m2 = class_getInstanceMethod(frameClass, @selector(drawRectOriginal:));

	method_exchangeImplementations(m1, m2);

	// Exchange _drawTitleStringIn:withColor:
	Method m3 = class_getInstanceMethod([DrawHelper class], @selector(_drawTitleStringIn:withColor:));
	class_addMethod(frameClass, @selector(_drawTitleStringOriginalIn:withColor:), method_getImplementation(m3), method_getTypeEncoding(m3));

	Method m4 = class_getInstanceMethod(frameClass, @selector(_drawTitleStringIn:withColor:));
	Method m5 = class_getInstanceMethod(frameClass, @selector(_drawTitleStringOriginalIn:withColor:));

	method_exchangeImplementations(m4, m5);

	// Exchange _titlebarTitleRect
	Method m6 = class_getInstanceMethod([DrawHelper class], @selector(_titlebarTitleRect));
	class_addMethod(frameClass, @selector(_titlebarTitleRectOriginal), method_getImplementation(m6), method_getTypeEncoding(m6));

	Method m7 = class_getInstanceMethod(frameClass, @selector(_titlebarTitleRect));
	Method m8 = class_getInstanceMethod(frameClass, @selector(_titlebarTitleRectOriginal));

	method_exchangeImplementations(m7, m8);

	// Add attributedTitle
	Method m9 = class_getInstanceMethod([DrawHelper class], @selector(attributedTitle));
	class_addMethod(frameClass, @selector(attributedTitle), method_getImplementation(m9), method_getTypeEncoding(m9));
}

void MacIntegrationPrivate::setCustomBorderColor(const QColor & frameColor)
{
	NSColor * nsFrameColor = [[NSColor colorWithCalibratedRed: frameColor.redF() green: frameColor.greenF() blue: frameColor.blueF() alpha: frameColor.alphaF()] retain];
	setFrameColor(nsFrameColor);
	foreach (QWidget * w, QApplication::topLevelWidgets())
		w->update();
}

void MacIntegrationPrivate::setCustomTitleColor(const QColor & titleColor)
{
	NSColor * nsTitleColor = [[NSColor colorWithCalibratedRed: titleColor.redF() green: titleColor.greenF() blue: titleColor.blueF() alpha: titleColor.alphaF()] retain];
	setTitleColor(nsTitleColor);
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

void MacIntegrationPrivate::checkForUpdates()
{
	if (sparkleUpdater)
		[sparkleUpdater checkForUpdates:nil];
}

void MacIntegrationPrivate::onUpdateTimer()
{
	QImage dockImage = ImageManager::rotatedImage(qImageFromNSImage([NSApp applicationIconImage]), updateTimer->property("angle").toFloat());
	setDockOverlay(dockImage, Qt::AlignCenter, false);
	updateTimer->setProperty("angle", updateTimer->property("angle").toFloat() + 5.0);
}
