#ifndef MACWIDGETS_H
#define MACWIDGETS_H

#include <Carbon/Carbon.h>
#include <QWidget>

#ifndef COCOA_CLASSES_DEFINED
class NSView;
class NSWindow;
class NSImage;
class NSString;
#endif

// Qt <-> Cocoa
WindowRef windowRefFromWidget(QWidget *w);
NSWindow * nsWindowFromWidget(QWidget *w);
NSView * nsViewFromWidget(QWidget *w);
NSImage * nsImageFromQImage(const QImage &image);
QImage qImageFromNSImage(NSImage *image);
NSString * nsStringFromQString(const QString &s);
QString qStringFromNSString(NSString *s);

// window management
void setWindowShadowEnabled(QWidget *window, bool enabled);
bool isWindowGrowButtonEnabled(QWidget *window);
void setWindowGrowButtonEnabled(QWidget *window, bool enabled);
void hideWindow(void */* (NSWindow*) */ window);
void setWindowFullScreenEnabled(QWidget *window, bool enabled);
bool isWindowFullScreenEnabled(QWidget *window);
void setWindowFullScreen(QWidget *window, bool enabled);
bool isWindowFullScreen(QWidget *window);

// app management
void setAppFullScreenEnabled(bool enabled);
bool isAppFullScreenEnabled();

// string management
QString convertFromMacCyrillic(const char *str);

#endif // MACWIDGETS_H
