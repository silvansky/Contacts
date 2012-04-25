#ifndef VIDEOCALLWINDOW_H
#define VIDEOCALLWINDOW_H

#include <QWidget>
#include <interfaces/isipphone.h>
#include <interfaces/ipluginmanager.h>
#include "videolabel.h"
#include "videolayout.h"
#include "callcontrolwidget.h"
#include "ui_videocallwindow.h"

class VideoCallWindow : 
	public QWidget
{
	Q_OBJECT;
public:
	VideoCallWindow(IPluginManager *APluginManager, ISipCall *ASipCall, QWidget *AParent = NULL);
	~VideoCallWindow();
	ISipCall *sipCall() const;
protected:
	void initialize(IPluginManager *APluginManager);
	void closeWindowWithAnimation();
	void restoreGeometryWithAnimation();
protected slots:
	void onCallStateChanged(int AState);
	void onCallDeviceStateChanged(int AType, int AState);
	void onCallDevicePropertyChanged(int AType, int AProperty, const QVariant &AValue);
private:
	Ui::VideoCallWindowClass ui;
private:
	bool FVideoVisible;
	VideoLabel *FLocalCamera;
	VideoLabel *FRemoteCamera;
	CallControlWidget *FCtrlWidget;
};

#endif // VIDEOCALLWINDOW_H
