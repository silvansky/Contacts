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

WindowRef windowRefFromWidget(QWidget *w);
NSWindow * nsWindowFromWidget(QWidget *w);
NSView * nsViewFromWidget(QWidget *w);
NSImage * nsImageFromQImage(const QImage &image);
QImage qImageFromNSImage(NSImage *image);
NSString * nsStringFromQString(const QString &s);
QString qStringFromNSString(NSString *s);
void setWindowShadowEnabled(QWidget *window, bool enabled);
bool isWindowGrowButtonEnabled(QWidget *window);
void setWindowGrowButtonEnabled(QWidget *window, bool enabled);
void hideWindow(void */* (NSWindow*) */ window);
QString convertFromMacCyrillic(const char *str);

#endif // MACWIDGETS_H
