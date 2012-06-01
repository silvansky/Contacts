#ifndef MACUTILS_H
#define MACUTILS_H

#ifndef __COREFOUNDATION__
class NSView;
class NSWindow;
class NSImage;
class NSString;
#endif

#include "utilsexport.h"

#include <Carbon/Carbon.h>
#include <QWidget>

class MacUtils : public QObject
{
	Q_OBJECT
protected:
	MacUtils();
	virtual ~MacUtils();
public:
	class MacUtilsPrivate;
	friend MacUtilsPrivate * gp();
	static MacUtils * instance();
signals:
	void windowFullScreenModeWillChange(QWidget *window, bool fullScreen);
	void windowFullScreenModeChanged(QWidget *window, bool fullScreen);
private:
	MacUtilsPrivate *p;
};

// Qt <-> Cocoa
WindowRef windowRefFromWidget(const QWidget *w);
NSWindow * nsWindowFromWidget(const QWidget *w);
NSView * nsViewFromWidget(const QWidget *w);
NSImage * nsImageFromQImage(const QImage &image);
QImage qImageFromNSImage(NSImage *image);
NSString * nsStringFromQString(const QString &s);
QString qStringFromNSString(NSString *s);

// window management
void setWindowShadowEnabled(QWidget *window, bool enabled);
bool isWindowGrowButtonEnabled(const QWidget *window);
void setWindowGrowButtonEnabled(QWidget *window, bool enabled);
void hideWindow(void */* (NSWindow*) */ window);
void setWindowFullScreenEnabled(QWidget *window, bool enabled);
bool isWindowFullScreenEnabled(const QWidget *window);
void setWindowFullScreen(QWidget *window, bool enabled);
bool isWindowFullScreen(const QWidget *window);
void setWindowOntop(QWidget *window, bool enabled);
bool isWindowOntop(const QWidget *window);

// app management
void setAppFullScreenEnabled(bool enabled);
bool isAppFullScreenEnabled();

// string management
QString convertFromMacCyrillic(const char *str);

#endif // MACUTILS_H
