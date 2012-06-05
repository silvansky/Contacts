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
	Q_OBJECT
public:
	VideoCallWindow(IPluginManager *APluginManager, ISipCall *ASipCall, QWidget *AParent = NULL);
	virtual ~VideoCallWindow();
	ISipCall *sipCall() const;
public:
	QSize sizeHint() const;
signals:
	void chatWindowRequested();
protected:
	void closeWindowWithAnimation();
	void restoreWindowGeometryWithAnimation();
	void setRecursiveMouseTracking(QWidget *AWidget);
	void setWindowGeometryWithAnimation(const QRect &AGeometry, int ADuration);
protected:
	bool canShowVideo() const;
	void setVideoVisible(bool AVisible, bool AResizing = false);
	void setControllsVisible(bool AVisible);
	void setControlsMode(bool fullscreen);
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
#ifdef Q_WS_MAC
protected slots:
	void onWindowFullScreenModeWillChange(QWidget *window, bool fullScreen);
	void onWindowFullScreenModeChanged(QWidget *window, bool fullScreen);
#endif
private:
	Ui::VideoCallWindowClass ui;
private:
	bool FIsFirstShow;
	bool FVideoVisible;
	bool FControllsVisible;
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
