#include "videocallwindow.h"

#include <QTimer>
#include <QPropertyAnimation>
#include <definitions/resources.h>
#include <definitions/menuicons.h>
#include <definitions/stylesheets.h>
#include <definitions/customborder.h>
#include <utils/options.h>
#include <utils/stylestorage.h>
#include <utils/widgetmanager.h>
#include <utils/customborderstorage.h>

#define CLOSE_WINDOW_TIMEOUT    2000

VideoCallWindow::VideoCallWindow(IPluginManager *APluginManager, ISipCall *ASipCall, QWidget *AParent) : QWidget(AParent)
{
	ui.setupUi(this);
	StyleStorage::staticStorage(RSR_STORAGE_STYLESHEETS)->insertAutoStyle(this,STS_SIPPHONE_VIDEOCALLWINDOW);

	CustomBorderContainer *border = CustomBorderStorage::staticStorage(RSR_STORAGE_CUSTOMBORDER)->addBorder(this, CBS_VIDEOCALL);
	if (border)
	{
		border->setMovable(true);
		border->setResizable(true);
		border->setStaysOnTop(true);
		border->setCloseButtonVisible(false);
		border->setMinimizeButtonVisible(true);
		border->setMaximizeButtonVisible(false);
		border->setAttribute(Qt::WA_DeleteOnClose,true);
	}
	else
	{
		setAttribute(Qt::WA_DeleteOnClose,true);
		setWindowFlags(windowFlags() | Qt::WindowStaysOnTopHint);
	}

	FVideoVisible = false;
	initialize(APluginManager);

	FRemoteCamera = new VideoFrame(ui.wdtVideo);
	FRemoteCamera->setObjectName("vlbRemoteCamera");

	FLocalCamera = new VideoFrame(ui.wdtVideo);
	FLocalCamera->setMoveEnabled(true);
	FLocalCamera->setResizeEnabled(true);
	FLocalCamera->setFrameShape(QLabel::Box);
	FLocalCamera->setObjectName("vlbLocalCamera");
	
	FVideoLayout = new VideoLayout(FRemoteCamera,FLocalCamera,ui.wdtVideo);
	ui.wdtVideo->setLayout(FVideoLayout);
	ui.wdtVideo->setVisible(false);

	FCtrlWidget = new CallControlWidget(APluginManager,ASipCall,ui.wdtControls);
	ui.wdtControls->setLayout(new QHBoxLayout);
	ui.wdtControls->layout()->setMargin(0);
	ui.wdtControls->layout()->addWidget(FCtrlWidget);

	connect(sipCall()->instance(),SIGNAL(stateChanged(int)),SLOT(onCallStateChanged(int)));
	connect(sipCall()->instance(),SIGNAL(deviceStateChanged(int, int)),SLOT(onCallDeviceStateChanged(int, int)));
	connect(sipCall()->instance(),SIGNAL(devicePropertyChanged(int, int, const QVariant &)),SLOT(onCallDevicePropertyChanged(int, int, const QVariant &)));

	onCallStateChanged(sipCall()->state());
}

VideoCallWindow::~VideoCallWindow()
{
	if (FVideoVisible)
		Options::setFileValue(window()->geometry(),"sipphone.videocall-window.geometry");
}

ISipCall *VideoCallWindow::sipCall() const
{
	return FCtrlWidget->sipCall();
}

void VideoCallWindow::initialize(IPluginManager *APluginManager)
{

}

void VideoCallWindow::closeWindowWithAnimation()
{
	FVideoLayout->saveLocalVideoGeometry();

	QPropertyAnimation *animation = new QPropertyAnimation(window(),"windowOpacity");
	animation->setDuration(500);
	animation->setStartValue(1.0);
	animation->setEndValue(0.0);
	connect(animation,SIGNAL(finished()),window(),SLOT(close()));
	animation->start();		
}

void VideoCallWindow::restoreGeometryWithAnimation()
{
	if (true/*sipCall()->deviceState(ISipDevice::DT_REMOTE_CAMERA) == ISipDevice::DS_ENABLED*/)
	{
		FVideoVisible = true;
		ui.wdtVideo->setVisible(true);

		QRect newGeometry = Options::fileValue("sipphone.videocall-window.geometry").toRect();
		if (newGeometry.isEmpty())
			newGeometry = WidgetManager::alignGeometry(QSize(640,480),window());

		QPropertyAnimation *animation = new QPropertyAnimation(window(),"geometry");
		animation->setDuration(200);
		animation->setStartValue(window()->geometry());
		animation->setEndValue(newGeometry);
		connect(animation,SIGNAL(finished()),animation,SLOT(deleteLater()));
		connect(animation,SIGNAL(finished()),FVideoLayout,SLOT(restoreLocalVideoGeometry()));
		animation->start();		
	}
}

void VideoCallWindow::onCallStateChanged(int AState)
{
	switch (AState)
	{
	case ISipCall::CS_TALKING:
		restoreGeometryWithAnimation();
		break;
	case ISipCall::CS_FINISHED:
	case ISipCall::CS_ERROR:
		closeWindowWithAnimation();
		break;
	default:
		break;
	}
	setWindowTitle(FCtrlWidget->windowTitle());
}

void VideoCallWindow::onCallDeviceStateChanged(int AType, int AState)
{
	if (AType==ISipDevice::DT_REMOTE_CAMERA && AState!=ISipDevice::DS_ENABLED)
	{
		FRemoteCamera->setPixmap(QPixmap());
	}
	else if (AType==ISipDevice::DT_LOCAL_CAMERA && AState!=ISipDevice::DS_ENABLED)
	{
		FLocalCamera->setPixmap(QPixmap());
	}
}

void VideoCallWindow::onCallDevicePropertyChanged(int AType, int AProperty, const QVariant &AValue)
{
	if (AType==ISipDevice::DT_REMOTE_CAMERA && AProperty==ISipDevice::RCP_CURRENTFRAME)
	{
		QImage frame = AValue.value<QImage>();
		FRemoteCamera->setPixmap(QPixmap::fromImage(frame));
	}
	else if (AType==ISipDevice::DT_LOCAL_CAMERA && AProperty==ISipDevice::LCP_CURRENTFRAME)
	{
		QImage frame = AValue.value<QImage>();
		FLocalCamera->setPixmap(QPixmap::fromImage(frame));
	}
}
