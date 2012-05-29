#ifndef VIDEOCALLWINDOW_H
#define VIDEOCALLWINDOW_H

#include <QTimer>
#include <QWidget>
#include <interfaces/isipphone.h>
#include <interfaces/ipluginmanager.h>
#include "videoframe.h"
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
public:
	QSize sizeHint() const;
protected:
	void initialize(IPluginManager *APluginManager);
	void closeWindowWithAnimation();
	void restoreGeometryWithAnimation();
protected:
	void resizeEvent(QResizeEvent *AEvent);
	void mouseMoveEvent(QMouseEvent *AEvent);
protected slots:
	void onCallStateChanged(int AState);
	void onCallDeviceStateChanged(int AType, int AState);
	void onCallDevicePropertyChanged(int AType, int AProperty, const QVariant &AValue);
protected slots:
	void onSilentButtonClicked();
	void onFullScreenModeChangeRequested();
	void onHideControlsTimerTimeout();
private:
	Ui::VideoCallWindowClass ui;
private:
	bool FVideoVisible;
	QTimer FHideControllsTimer;
	QToolButton *FFullScreen;
	VideoFrame *FLocalCamera;
	VideoFrame *FRemoteCamera;
	VideoLayout *FVideoLayout;
	CallControlWidget *FCtrlWidget;
};

#endif // VIDEOCALLWINDOW_H
