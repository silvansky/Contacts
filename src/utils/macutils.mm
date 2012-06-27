#import <Cocoa/Cocoa.h>
#import <CoreFoundation/CFString.h>
#import <objc/runtime.h>

// note that you should #import <Cocoa/Cocoa.h> BEFORE macutils.h to work with Cocoa correctly
#import "macutils.h"
#import "macutils_p.h"

#include "log.h"

#define MACUTILS_ONTOP_WAS_SET "macUtilsOnTopWasSet"

#include <QDebug>

// class MacUtils::MacUtilsPrivate
QMap<QWidget *, NSWindow *> MacUtils::MacUtilsPrivate::fullScreenWidgets;

MacUtils::MacUtilsPrivate::MacUtilsPrivate(MacUtils *parent)
{
	connect(this, SIGNAL(windowFullScreenModeChanged(QWidget*,bool)), parent, SIGNAL(windowFullScreenModeChanged(QWidget*,bool)));
	connect(this, SIGNAL(windowFullScreenModeWillChange(QWidget*,bool)), parent, SIGNAL(windowFullScreenModeWillChange(QWidget*,bool)));
}

void MacUtils::MacUtilsPrivate::emitWindowFullScreenModeWillChange(QWidget *window, bool fullScreen)
{
	// save "ontop" if it is set
	if (fullScreen && isWindowOntop(window))
	{
		NSLog(@"Window is ontop! Saving state...");
		window->setProperty(MACUTILS_ONTOP_WAS_SET, true);
		setWindowOntop(window, false);
	}

	//qDebug() << QString("Emitting windowFullScreenModeWillChange signal with %1").arg(window->windowTitle());
	emit windowFullScreenModeWillChange(window, fullScreen);
}

void MacUtils::MacUtilsPrivate::emitWindowFullScreenModeChanged(QWidget *window, bool fullScreen)
{
	// restore "ontop" if needed
	if (!fullScreen && window->property(MACUTILS_ONTOP_WAS_SET).toBool())
	{
		NSLog(@"Window was ontop! Restoring state...");
		window->setProperty(MACUTILS_ONTOP_WAS_SET, false);
		setWindowOntop(window, true);
	}

	//qDebug() << QString("Emitting windowFullScreenModeChanged signal with %1").arg(window->windowTitle());
	emit windowFullScreenModeChanged(window, fullScreen);
}

// class MacUtils
MacUtils::MacUtils()
{
	p = new MacUtilsPrivate(this);
}

MacUtils::~MacUtils()
{
	delete p;
}

MacUtils *MacUtils::instance()
{
	static MacUtils *inst = NULL;
	if (!inst)
		inst = new MacUtils;
	return inst;
}

// private functions

MacUtils::MacUtilsPrivate * gp()
{
	return MacUtils::instance()->p;
}

// NSWindowDelegate's added functions implementation
void windowWillEnterFullScreen(id self, SEL _cmd, id notification)
{
	Q_UNUSED(self)
	Q_UNUSED(_cmd)
	NSNotification *n = (NSNotification *)notification;
	if (n)
	{
		NSWindow *window = [n object];
		QWidget *w = MacUtils::MacUtilsPrivate::fullScreenWidgets.key(window, NULL);
		if (w)
			gp()->emitWindowFullScreenModeWillChange(w, true);
	}
}

void windowDidEnterFullScreen(id self, SEL _cmd, id notification)
{
	Q_UNUSED(self)
	Q_UNUSED(_cmd)
	NSNotification *n = (NSNotification *)notification;
	if (n)
	{
		NSWindow *window = [n object];
		QWidget *w = MacUtils::MacUtilsPrivate::fullScreenWidgets.key(window, NULL);
		if (w)
			gp()->emitWindowFullScreenModeChanged(w, true);
	}
}

void windowWillExitFullScreen(id self, SEL _cmd, id notification)
{
	Q_UNUSED(self)
	Q_UNUSED(_cmd)
	NSNotification *n = (NSNotification *)notification;
	if (n)
	{
		NSWindow *window = [n object];
		QWidget *w = MacUtils::MacUtilsPrivate::fullScreenWidgets.key(window, NULL);
		if (w)
			gp()->emitWindowFullScreenModeWillChange(w, false);
	}
}

void windowDidExitFullScreen(id self, SEL _cmd, id notification)
{
	Q_UNUSED(self)
	Q_UNUSED(_cmd)
	Q_UNUSED(notification)
	NSNotification *n = (NSNotification *)notification;
	if (n)
	{
		NSWindow *window = [n object];
		QWidget *w = MacUtils::MacUtilsPrivate::fullScreenWidgets.key(window, NULL);
		if (w)
			gp()->emitWindowFullScreenModeChanged(w, false);
	}
}

// adding methods above to QCocoaWindowDelegate, which is singletone (! that's really important !)
void initWindowDelegate(NSWindow *wnd)
{
#ifdef __MAC_OS_X_NATIVE_FULLSCREEN
	LogDetail("[initWindowDelegate]: Initializing NSWindowDelegate: adding -window*FullScreen: methods...");
	id delegate = [wnd delegate];
	Class delegateClass = [delegate class];
	if (!class_respondsToSelector(delegateClass, @selector(windowWillEnterFullScreen:)))
	{
		if (!class_addMethod(delegateClass, @selector(windowWillEnterFullScreen:), (IMP) windowWillEnterFullScreen, "v@:@"))
			LogError("class_addMethod failed for -windowWillEnterFullScreen:!");
	}
	if (!class_respondsToSelector(delegateClass, @selector(windowDidEnterFullScreen:)))
	{
		if (!class_addMethod(delegateClass, @selector(windowDidEnterFullScreen:), (IMP) windowDidEnterFullScreen, "v@:@"))
			LogError("class_addMethod failed for -windowDidEnterFullScreen:!");
	}
	if (!class_respondsToSelector(delegateClass, @selector(windowWillExitFullScreen:)))
	{
		if (!class_addMethod(delegateClass, @selector(windowWillExitFullScreen:), (IMP) windowWillExitFullScreen, "v@:@"))
			LogError("class_addMethod failed for -windowWillExitFullScreen:!");
	}
	if (!class_respondsToSelector(delegateClass, @selector(windowDidExitFullScreen:)))
	{
		if (!class_addMethod(delegateClass, @selector(windowDidExitFullScreen:), (IMP) windowDidExitFullScreen, "v@:@"))
			LogError("class_addMethod failed for -windowDidExitFullScreen:!");
	}
	// resetting delegate to get our methods work
	[wnd setDelegate:nil];
	[wnd setDelegate:delegate];
#else
	Q_UNUSED(wnd)
#endif
}

// public functions

WindowRef windowRefFromWidget(const QWidget *w)
{
	return (WindowRef)[nsWindowFromWidget(w) windowRef];
}

NSWindow * nsWindowFromWidget(const QWidget *w)
{
	return [nsViewFromWidget(w) window];
}

NSView * nsViewFromWidget(const QWidget *w)
{
	return (NSView *)w->winId();
}

// warning! NSImage isn't released!
NSImage * nsImageFromQImage(const QImage &image)
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

QImage qImageFromNSImage(NSImage *image)
{
	if (image)
	{
		CGImageRef ref = [image CGImageForProposedRect:NULL context:nil hints:nil];
		QImage result = QPixmap::fromMacCGImageRef(ref).toImage();
		return result;
	}
	return QImage();
}

// warning! NSString isn't released!
NSString * nsStringFromQString(const QString &s)
{
	const char * utf8String = s.toUtf8().constData();
	return [[NSString alloc] initWithUTF8String: utf8String];
}

QString qStringFromNSString(NSString *s)
{
	NSAutoreleasePool * pool = [[NSAutoreleasePool alloc] init];
	QString res = QString::fromUtf8([s UTF8String]);
	[pool release];
	return res;
}

void setWindowShadowEnabled(QWidget *window, bool enabled)
{
	[[nsViewFromWidget(window) window] setHasShadow: (enabled ? YES : NO)];
}

bool isWindowGrowButtonEnabled(const QWidget *window)
{
	if (window)
		return [[[nsViewFromWidget(window) window] standardWindowButton: NSWindowZoomButton] isEnabled] == YES;
	else
		return false;
}

void setWindowGrowButtonEnabled(QWidget *window, bool enabled)
{
	if (window)
		[[[nsViewFromWidget(window) window] standardWindowButton: NSWindowZoomButton] setEnabled: (enabled ? YES : NO)];
}

void hideWindow(void */* (NSWindow*) */ window)
{
	NSWindow *nsWindow = (NSWindow*)window;
	[nsWindow orderOut:nil];
}

void setWindowFullScreenEnabled(QWidget *window, bool enabled)
{
#ifdef __MAC_OS_X_NATIVE_FULLSCREEN
	NSWindow *wnd = nsWindowFromWidget(window->window());
	initWindowDelegate(wnd);
	NSWindowCollectionBehavior b = [wnd collectionBehavior];
	if (enabled)
	{
		MacUtils::MacUtilsPrivate::fullScreenWidgets.insert(window->window(), wnd);
		[wnd setCollectionBehavior: b | NSWindowCollectionBehaviorFullScreenPrimary];
	}
	else if (isWindowFullScreenEnabled(window))
	{
		MacUtils::MacUtilsPrivate::fullScreenWidgets.remove(window->window());
		[wnd setCollectionBehavior: b ^ NSWindowCollectionBehaviorFullScreenPrimary];
	}
#else
	Q_UNUSED(window)
	Q_UNUSED(enabled)
#endif
}

bool isWindowFullScreenEnabled(const QWidget *window)
{
#ifdef __MAC_OS_X_NATIVE_FULLSCREEN
	NSWindow *wnd = nsWindowFromWidget(window->window());
	return [wnd collectionBehavior] & NSWindowCollectionBehaviorFullScreenPrimary;
#else
	Q_UNUSED(window)
	return false;
#endif
}

void setWindowFullScreen(QWidget *window, bool enabled)
{
#ifdef __MAC_OS_X_NATIVE_FULLSCREEN
	if (isWindowFullScreenEnabled(window))
	{
		bool isFullScreen = isWindowFullScreen(window);
		if ((enabled && !isFullScreen) || (!enabled && isFullScreen))
		{
			if (enabled)
			{
				// removing Qt's ontop flag
				Qt::WindowFlags flags = window->windowFlags();
				if (flags & Qt::WindowStaysOnTopHint)
				{
					LogWarning(QString("[setWindowFullScreen]: Widget %1 has Qt ontop flag! It will be removed and window will be redisplayed.").arg(window->windowTitle()));
					window->setWindowFlags(flags ^ Qt::WindowStaysOnTopHint);
					window->show();
					setWindowFullScreenEnabled(window, true);
				}
			}

			NSWindow *wnd = nsWindowFromWidget(window->window());
			[wnd toggleFullScreen:nil];
		}
	}
#else
	Q_UNUSED(window)
	Q_UNUSED(enabled)
#endif
}

bool isWindowFullScreen(const QWidget *window)
{
#ifdef __MAC_OS_X_NATIVE_FULLSCREEN
	NSWindow *wnd = nsWindowFromWidget(window->window());
	return [wnd styleMask] & NSFullScreenWindowMask;
#else
	Q_UNUSED(window)
	return false;
#endif
}

void setWindowOntop(QWidget *window, bool enabled)
{
	NSWindow *wnd = nsWindowFromWidget(window->window());
	[wnd setLevel:(enabled ? NSModalPanelWindowLevel : NSNormalWindowLevel)];
}

bool isWindowOntop(const QWidget *window)
{
	NSWindow *wnd = nsWindowFromWidget(window->window());
	return [wnd level] == NSModalPanelWindowLevel;
}

void setWindowResizable(QWidget *window, bool enabled)
{
	NSWindow *wnd = nsWindowFromWidget(window->window());
	bool isResizable = isWindowResizable(window);
	if ((enabled && !isResizable) || (!enabled && isResizable))
	{
		int oldStyleMask = [wnd styleMask];
		int newStyleMask = enabled ? oldStyleMask | NSResizableWindowMask : oldStyleMask ^ NSResizableWindowMask;
		[wnd setStyleMask:newStyleMask];
	}
}

bool isWindowResizable(QWidget *window)
{
	NSWindow *wnd = nsWindowFromWidget(window->window());
	return [wnd styleMask] & NSResizableWindowMask;
}

void setWindowShownOnAllSpaces(QWidget *window, bool enabled)
{
	NSWindow *wnd = nsWindowFromWidget(window->window());
	bool isEnabled = isWindowShownOnAllSpaces(window);
	if ((enabled && !isEnabled) || (!enabled && isEnabled))
	{
		NSWindowCollectionBehavior oldCollectionBhv = [wnd collectionBehavior];
		NSWindowCollectionBehavior newCollectionBhv = enabled ? oldCollectionBhv | NSWindowCollectionBehaviorCanJoinAllSpaces : oldCollectionBhv ^ NSWindowCollectionBehaviorCanJoinAllSpaces;
		[wnd setCollectionBehavior:newCollectionBhv];
	}
}

bool isWindowShownOnAllSpaces(QWidget *window)
{
	NSWindow *wnd = nsWindowFromWidget(window->window());
	return [wnd collectionBehavior] & NSWindowCollectionBehaviorCanJoinAllSpaces;
}

void setAppFullScreenEnabled(bool enabled)
{
#ifdef __MAC_OS_X_NATIVE_FULLSCREEN
	NSApplication *app = [NSApplication sharedApplication];
	if (enabled)
		[app setPresentationOptions: [app presentationOptions] | NSApplicationPresentationFullScreen];
	else if (isAppFullScreenEnabled())
		[app setPresentationOptions: [app presentationOptions] ^ NSApplicationPresentationFullScreen];
#else
	Q_UNUSED(enabled)
#endif
}

bool isAppFullScreenEnabled()
{
#ifdef __MAC_OS_X_NATIVE_FULLSCREEN
	return [[NSApplication sharedApplication] presentationOptions] & NSApplicationPresentationFullScreen;
#else
	return false;
#endif
}

QString convertFromMacCyrillic(const char *str)
{
	NSString *srcString = [[NSString alloc] initWithCString:str encoding:CFStringConvertEncodingToNSStringEncoding(kCFStringEncodingMacCyrillic)];
	QString resString = qStringFromNSString(srcString);
	[srcString release];
	return resString;
}
