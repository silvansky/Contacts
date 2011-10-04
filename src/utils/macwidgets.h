#ifndef MACWIDGETS_H
#define MACWIDGETS_H

#include <Carbon/Carbon.h>
#include <QWidget>

class NSView;
class NSWindow;

WindowRef windowRefFromWidget(QWidget * w);
NSWindow * nsWindowFromWidget(QWidget * w);
NSView * nsViewFromWidget(QWidget * w);

#endif // MACWIDGETS_H
