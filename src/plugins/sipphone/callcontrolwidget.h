#ifndef CALLCONTROLWIDGET_H
#define CALLCONTROLWIDGET_H

#include <QWidget>
#include <definitions/resources.h>
#include <definitions/stylesheets.h>
#include <interfaces/ipluginmanager.h>
#include <interfaces/isipphone.h>
#include <utils/stylestorage.h>
#include "ui_callcontrolwidget.h"

class CallControlWidget : 
	public QWidget
{
	Q_OBJECT;
public:
	CallControlWidget(IPluginManager *APluginManager, ISipCall *ASipCall, QWidget *AParent = NULL);
	~CallControlWidget();
	ISipCall *sipCall() const;
protected:
	void initialize(IPluginManager *APluginManager);
protected slots:
	void onCallStateChanged(int AState);
	void onCallDeviceStateChanged(ISipDevice::Type AType, ISipDevice::State AState);
private:
	Ui::CallControlWidgetClass ui;
private:
	ISipCall *FSipCall;
};

#endif // CALLCONTROLWIDGET_H
