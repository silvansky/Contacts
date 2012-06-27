#ifndef MACUTILS_P_H
#define MACUTILS_P_H

#include "macutils.h"

class MacUtils::MacUtilsPrivate : public QObject
{
	Q_OBJECT
public:
	MacUtilsPrivate(MacUtils *parent);
public:
	void emitWindowFullScreenModeWillChange(QWidget *window, bool fullScreen);
	void emitWindowFullScreenModeChanged(QWidget *window, bool fullScreen);
signals:
	void windowFullScreenModeWillChange(QWidget *window, bool fullScreen);
	void windowFullScreenModeChanged(QWidget *window, bool fullScreen);
public:
	static QMap<QWidget *, NSWindow *> fullScreenWidgets;
};

#endif // MACUTILS_P_H
