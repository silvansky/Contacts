#ifndef IMACINTEGRATION_H
#define IMACINTEGRATION_H

#include <QObject>
#include <QMenuBar>
#include <utils/menu.h>

class IMacIntegration
{
public:
	virtual QObject * instance() = 0;
	virtual Menu * dockMenu() = 0;
	virtual QMenuBar * menuBar() = 0;
	virtual Menu * fileMenu() = 0;
	virtual Menu * editMenu() = 0;
	virtual Menu * contactsMenu() = 0;
	virtual Menu * windowMenu() = 0;
	virtual void setDockBadge(const QString & badgeText) = 0;
	virtual void postGrowlNotify(const QImage & icon, const QString & title, const QString & text, const QString & type, int id) = 0;
	virtual void showGrowlPreferencePane() = 0;
	virtual void setCustomBorderColor(const QColor & color) = 0;
	virtual void setCustomTitleColor(const QColor & color) = 0;
	virtual void setWindowMovableByBackground(QWidget * window, bool movable) = 0;
protected:
	// signals
	virtual void dockClicked() = 0;
	virtual void growlNotifyClicked(int id) = 0;
};

Q_DECLARE_INTERFACE(IMacIntegration,"Virtus.Core.IMacIntegration/1.0")

#endif // IMACINTEGRATION_H
