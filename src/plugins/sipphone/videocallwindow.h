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
	bool canShowVideo() const;
	void setVideoVisible(bool AVisible, bool ACorrectSize = false);
protected:
	void showEvent(QShowEvent *AEvent);
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
	void onGeometryAnimationFinished();
private:
	Ui::VideoCallWindowClass ui;
private:
	bool FVideoVisible;
	int FBlockVideoChange;
	bool FAnimatingGeometry;
	QTimer FHideControllsTimer;
	QToolButton *FFullScreen;
	VideoFrame *FLocalCamera;
	VideoFrame *FRemoteCamera;
	VideoLayout *FVideoLayout;
	CallControlWidget *FCtrlWidget;
};

#endif // VIDEOCALLWINDOW_H
