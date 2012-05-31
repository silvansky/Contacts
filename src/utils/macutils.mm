#import <Cocoa/Cocoa.h>
#import <CoreFoundation/CFString.h>

#define COCOA_CLASSES_DEFINED

#import "macutils.h"
#include <QDebug>

#define MW_OLD_WINDOW_FLAGS "macwidgets_windowFlagsBeforeFullscreen"

//static NSAutoreleasePool * pool = [[NSAutoreleasePool alloc] init];

WindowRef windowRefFromWidget(QWidget *w)
{
	return (WindowRef)[nsWindowFromWidget(w) windowRef];
}

NSWindow * nsWindowFromWidget(QWidget *w)
{
	return [nsViewFromWidget(w) window];
}

NSView * nsViewFromWidget(QWidget *w)
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

bool isWindowGrowButtonEnabled(QWidget *window)
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
	NSWindow *wnd = nsWindowFromWidget(window->window());
	if (enabled)
		[wnd setCollectionBehavior: [wnd collectionBehavior] | NSWindowCollectionBehaviorFullScreenPrimary];
	else if (isWindowFullScreenEnabled(window))
		[wnd setCollectionBehavior: [wnd collectionBehavior] ^ NSWindowCollectionBehaviorFullScreenPrimary];
}

bool isWindowFullScreenEnabled(QWidget *window)
{
	NSWindow *wnd = nsWindowFromWidget(window->window());
	return [wnd collectionBehavior] & NSWindowCollectionBehaviorFullScreenPrimary;
}

void setWindowFullScreen(QWidget *window, bool enabled)
{
	if (isWindowFullScreenEnabled(window))
	{
		bool isFullScreen = isWindowFullScreen(window);
		if ((enabled && !isFullScreen) || (!enabled && isFullScreen))
		{
			if (enabled)
			{
				// saving old window flags, if window was ontop and removing ontop flag
				Qt::WindowFlags flags = window->windowFlags();
				if (flags & Qt::WindowStaysOnTopHint)
				{
					window->setProperty(MW_OLD_WINDOW_FLAGS, (int)flags);
					window->setWindowFlags(flags ^ Qt::WindowStaysOnTopHint);
					window->show();
					setWindowFullScreenEnabled(window, true);
				}
				else
					window->setProperty(MW_OLD_WINDOW_FLAGS, 0);
			}

			NSWindow *wnd = nsWindowFromWidget(window->window());
			[wnd toggleFullScreen:nil];

			if (!enabled)
			{
				// if old flags present, setting them again
				Qt::WindowFlags flags = (Qt::WindowFlags)window->property(MW_OLD_WINDOW_FLAGS).toInt();
				if (flags)
				{
					window->setWindowFlags(flags);
					window->show();
					setWindowFullScreenEnabled(window, true);
				}
			}
		}
	}
}

bool isWindowFullScreen(QWidget *window)
{
	NSWindow *wnd = nsWindowFromWidget(window->window());
	return [wnd styleMask] & NSFullScreenWindowMask;
}

void setAppFullScreenEnabled(bool enabled)
{
	NSApplication *app = [NSApplication sharedApplication];
	if (enabled)
		[app setPresentationOptions: [app presentationOptions] | NSApplicationPresentationFullScreen];
	else if (isAppFullScreenEnabled())
		[app setPresentationOptions: [app presentationOptions] ^ NSApplicationPresentationFullScreen];
}

bool isAppFullScreenEnabled()
{
	return [[NSApplication sharedApplication] presentationOptions] & NSApplicationPresentationFullScreen;
}

QString convertFromMacCyrillic(const char *str)
{
	NSString *srcString = [[NSString alloc] initWithCString:str encoding:CFStringConvertEncodingToNSStringEncoding(kCFStringEncodingMacCyrillic)];
	QString resString = qStringFromNSString(srcString);
	[srcString release];
	return resString;
}
