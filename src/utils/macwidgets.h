#ifndef MACWIDGETS_H
#define MACWIDGETS_H

#include <Carbon/Carbon.h>
#include <QWidget>

#ifndef COCOA_CLASSES_DEFINED
class NSView;
class NSWindow;
#endif

WindowRef windowRefFromWidget(QWidget * w);
NSWindow * nsWindowFromWidget(QWidget * w);
NSView * nsViewFromWidget(QWidget * w);
void setWindowShadowEnabled(QWidget * window, bool enabled);
bool isWindowGrowButtonEnabled(QWidget * window);
void setWindowGrowButtonEnabled(QWidget * window, bool enabled);

#endif // MACWIDGETS_H
