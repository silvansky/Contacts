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
protected:
	// signals
	virtual void dockClicked() = 0;
};

Q_DECLARE_INTERFACE(IMacIntegration,"Virtus.Core.IMacIntegration/1.0")

#endif // IMACINTEGRATION_H
