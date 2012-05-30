#include "videocallwindow.h"

#include <QMouseEvent>
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
#define HIDE_CONTROLLS_TIMEOT   3000

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
		border->setMinimizeButtonVisible(false);
		border->setMaximizeButtonVisible(false);
		border->setAttribute(Qt::WA_DeleteOnClose,true);
	}
	else
	{
		setAttribute(Qt::WA_DeleteOnClose,true);
		setWindowFlags(Qt::WindowStaysOnTopHint | Qt::CustomizeWindowHint | Qt::WindowMinimizeButtonHint);
	}

	FVideoVisible = true;
	FBlockVideoChange = 0;
	FAnimatingGeometry = false;
	initialize(APluginManager);

	FRemoteCamera = new VideoFrame(ui.wdtVideo);
	FRemoteCamera->setMinimumVideoSize(QSize(100,100));
	FRemoteCamera->setObjectName("vlbRemoteCamera");
	connect(FRemoteCamera,SIGNAL(doubleClicked()),SLOT(onFullScreenModeChangeRequested()));

	FLocalCamera = new VideoFrame(ui.wdtVideo);
	FLocalCamera->setMoveEnabled(true);
	FLocalCamera->setResizeEnabled(true);
	FLocalCamera->setFrameShape(QLabel::Box);
	FLocalCamera->setObjectName("vlbLocalCamera");
	
	FCtrlWidget = new CallControlWidget(APluginManager,ASipCall,ui.wdtControls);
	ui.wdtControls->setLayout(new QHBoxLayout);
	ui.wdtControls->layout()->setMargin(0);
	ui.wdtControls->layout()->addWidget(FCtrlWidget);
	connect(FCtrlWidget,SIGNAL(silentButtonClicked()),SLOT(onSilentButtonClicked()));

	FFullScreen = new QToolButton(ui.wdtVideo);
	FFullScreen->setObjectName("tlbFullScreen");
	FFullScreen->setIcon(IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getIcon(MNI_SIPPHONE_CALL_FULLSCREEN));
	connect(FFullScreen,SIGNAL(clicked()),SLOT(onFullScreenModeChangeRequested()));

	FVideoLayout = new VideoLayout(FRemoteCamera,FLocalCamera,FFullScreen,ui.wdtVideo);
	ui.wdtVideo->setLayout(FVideoLayout);

	FHideControllsTimer.setSingleShot(true);
	FHideControllsTimer.setInterval(HIDE_CONTROLLS_TIMEOT);
	connect(&FHideControllsTimer,SIGNAL(timeout()),SLOT(onHideControlsTimerTimeout()));

	connect(sipCall()->instance(),SIGNAL(stateChanged(int)),SLOT(onCallStateChanged(int)));
	connect(sipCall()->instance(),SIGNAL(deviceStateChanged(int, int)),SLOT(onCallDeviceStateChanged(int, int)));
	connect(sipCall()->instance(),SIGNAL(devicePropertyChanged(int, int, const QVariant &)),SLOT(onCallDevicePropertyChanged(int, int, const QVariant &)));

	setVideoVisible(false);
	onCallStateChanged(sipCall()->state());
}

VideoCallWindow::~VideoCallWindow()
{
	if (!FCtrlWidget->isFullScreenMode() && FVideoVisible)
		Options::setFileValue(window()->geometry(),"sipphone.videocall-window.geometry");
}

ISipCall *VideoCallWindow::sipCall() const
{
	return FCtrlWidget->sipCall();
}

QSize VideoCallWindow::sizeHint() const
{
	static const QSize minHint(460,1);
	return QWidget::sizeHint().expandedTo(minHint);
}

void VideoCallWindow::initialize(IPluginManager *APluginManager)
{
	Q_UNUSED(APluginManager);
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
	if (canShowVideo())
	{
		FAnimatingGeometry = true;
		setVideoVisible(true);

		if (CustomBorderStorage::isBordered(window()))
			CustomBorderStorage::widgetBorder(window())->setMinimizeButtonVisible(true);

		QRect newGeometry = Options::fileValue("sipphone.videocall-window.geometry").toRect();
		if (newGeometry.isEmpty())
			newGeometry = WidgetManager::alignGeometry(QSize(500,480),window());

		QPropertyAnimation *animation = new QPropertyAnimation(window(),"geometry");
		animation->setDuration(200);
		animation->setStartValue(window()->geometry());
		animation->setEndValue(newGeometry);
		connect(animation,SIGNAL(finished()),animation,SLOT(deleteLater()));
		connect(animation,SIGNAL(finished()),FVideoLayout,SLOT(restoreLocalVideoGeometry()));
		connect(animation,SIGNAL(finished()),SLOT(onGeometryAnimationFinished()));
		animation->start();
	}
}

bool VideoCallWindow::canShowVideo() const
{
	return sipCall()->state()==ISipCall::CS_TALKING && sipCall()->deviceState(ISipDevice::DT_REMOTE_CAMERA)!=ISipDevice::DS_UNAVAIL;
}

void VideoCallWindow::setVideoVisible(bool AVisible, bool ACorrectSize)
{
	if (FVideoVisible != AVisible)
	{
		FVideoVisible = AVisible;
		ui.wdtVideo->setVisible(AVisible);
		
		if (ACorrectSize)
		{
			int hDelta = !AVisible ? ui.wdtVideo->height() : FRemoteCamera->minimumVideoSize().height()+5;
			QPoint cursorPos = QCursor::pos();
			if (mapFromGlobal(cursorPos).y() < 5)
				cursorPos.ry() += !AVisible ? hDelta : -hDelta;
			else
				cursorPos.ry() -= !AVisible ? hDelta : -hDelta;
			QCursor::setPos(cursorPos);
		}
	}
}

void VideoCallWindow::showEvent( QShowEvent *AEvent )
{
	window()->adjustSize();
	QWidget::showEvent(AEvent);
}

void VideoCallWindow::resizeEvent(QResizeEvent *AEvent)
{
	QWidget::resizeEvent(AEvent);
	if (!FAnimatingGeometry && FBlockVideoChange==0)
	{
		if (FVideoVisible && ui.wdtVideo->height()<FVideoLayout->minimumSize().height()+2)
		{
			FBlockVideoChange++;
			setVideoVisible(false,true);
		}
		else if (!FVideoVisible && canShowVideo() && ui.wdtControls->height()>ui.wdtControls->sizeHint().height()+2)
		{
			FBlockVideoChange++;
			setVideoVisible(true,true);
		}
	}
	else if (FBlockVideoChange>0)
	{
		FBlockVideoChange--;
	}
}

void VideoCallWindow::mouseMoveEvent(QMouseEvent *AEvent)
{
	if (FCtrlWidget->isFullScreenMode())
	{
		if (FCtrlWidget->geometry().contains(AEvent->pos()))
			FHideControllsTimer.stop();
		else
			FHideControllsTimer.start();

		FCtrlWidget->setVisible(true);
		qApp->restoreOverrideCursor();
	}
	QWidget::mouseMoveEvent(AEvent);
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

void VideoCallWindow::onSilentButtonClicked()
{
	if (CustomBorderStorage::isBordered(window()))
		CustomBorderStorage::widgetBorder(window())->minimizeWidget();
	else
		window()->showMinimized();
}

void VideoCallWindow::onFullScreenModeChangeRequested()
{
	if (!FCtrlWidget->isFullScreenMode() && !FRemoteCamera->isEmpty())
	{
		ui.wdtControls->layout()->removeWidget(FCtrlWidget);

		FCtrlWidget->setFullScreenMode(true);
		FCtrlWidget->setParent(ui.wdtVideo);
		FVideoLayout->setControllsWidget(FCtrlWidget);
		FCtrlWidget->setVisible(true);

		if (CustomBorderStorage::isBordered(window()))
			CustomBorderStorage::widgetBorder(window())->showFullScreen();
		else
			window()->showFullScreen();

		FHideControllsTimer.start();
	}
	else if (FCtrlWidget->isFullScreenMode())
	{
		FVideoLayout->setControllsWidget(NULL);

		qApp->restoreOverrideCursor();
		FCtrlWidget->setFullScreenMode(false);
		FCtrlWidget->setParent(ui.wdtControls);
		ui.wdtControls->layout()->addWidget(FCtrlWidget);
		FCtrlWidget->setVisible(true);

		if (CustomBorderStorage::isBordered(window()))
			CustomBorderStorage::widgetBorder(window())->showFullScreen();
		else
			window()->showNormal();
	}
}

void VideoCallWindow::onHideControlsTimerTimeout()
{
	if (FCtrlWidget->isFullScreenMode())
	{
		FCtrlWidget->setVisible(false);
		qApp->setOverrideCursor(Qt::BlankCursor);
	}
}

void VideoCallWindow::onGeometryAnimationFinished()
{
	FAnimatingGeometry = false;
}
