#include "videocallwindow.h"

#include <QTimer>
#include <definitions/resources.h>
#include <definitions/menuicons.h>
#include <definitions/stylesheets.h>
#include <definitions/customborder.h>
#include <utils/stylestorage.h>
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
		border->setCloseButtonVisible(true);
		border->setMinimizeButtonVisible(false);
		border->setMaximizeButtonVisible(false);
		border->setAttribute(Qt::WA_DeleteOnClose,true);
	}
	else
	{
		setAttribute(Qt::WA_DeleteOnClose,true);
		setWindowFlags(windowFlags() | Qt::WindowStaysOnTopHint);
	}

	initialize(APluginManager);

	ui.wdtVideo->setLayout(new QHBoxLayout);

	FRemoteCamera = new QLabel(ui.wdtVideo);
	FRemoteCamera->setScaledContents(true);
	FRemoteCamera->setObjectName("lblRemoteCamera");
	ui.wdtVideo->layout()->addWidget(FRemoteCamera);

	FLocalCamera = new QLabel(ui.wdtVideo);
	FLocalCamera->setScaledContents(true);
	FLocalCamera->setObjectName("lblLocalCamera");
	ui.wdtVideo->layout()->addWidget(FLocalCamera);

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

}

ISipCall *VideoCallWindow::sipCall() const
{
	return FCtrlWidget->sipCall();
}

void VideoCallWindow::initialize(IPluginManager *APluginManager)
{

}

void VideoCallWindow::onCallStateChanged(int AState)
{
	switch (AState)
	{
	case ISipCall::CS_FINISHED:
	case ISipCall::CS_ERROR:
		QTimer::singleShot(CLOSE_WINDOW_TIMEOUT,window(),SLOT(close()));
		break;
	default:
		break;
	}
}

void VideoCallWindow::onCallDeviceStateChanged(int AType, int AState)
{

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
