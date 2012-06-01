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

#ifdef Q_WS_MAC
# include <utils/macutils.h>
#endif

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
#ifndef Q_WS_MAC
		setWindowFlags(windowFlags() | Qt::WindowStaysOnTopHint);
#else
		setWindowOntop(this, true);
#endif
		//setWindowFlags((windowFlags() & ~(Qt::WindowCloseButtonHint|Qt::WindowMaximizeButtonHint)) | Qt::WindowStaysOnTopHint | Qt::CustomizeWindowHint);
#ifdef Q_WS_MAC
		setWindowFullScreenEnabled(this, true);
		setWindowGrowButtonEnabled(this, false);
#endif
	}

	FIsFirstShow = true;
	FVideoVisible = true;
	FControllsVisible = true;
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
	//FLocalCamera->setFrameShape(QLabel::Box);
	FLocalCamera->setObjectName("vlbLocalCamera");
	
	FCtrlWidget = new CallControlWidget(APluginManager,ASipCall,ui.wdtControls);
	ui.wdtControls->setLayout(new QHBoxLayout);
	ui.wdtControls->layout()->setMargin(0);
	ui.wdtControls->layout()->addWidget(FCtrlWidget);
	connect(FCtrlWidget,SIGNAL(silentButtonClicked()),SLOT(onSilentButtonClicked()));

#ifdef Q_WS_MAC
	FFullScreen = NULL;
#else
	FFullScreen = new QToolButton(ui.wdtVideo);
	FFullScreen->setObjectName("tlbFullScreen");
	FFullScreen->setToolTip(tr("Change full screen mode on/off"));
	FFullScreen->setIcon(IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getIcon(MNI_SIPPHONE_CALL_FULLSCREEN));
	connect(FFullScreen,SIGNAL(clicked()),SLOT(onFullScreenModeChangeRequested()));
#endif

	FVideoLayout = new VideoLayout(FRemoteCamera,FLocalCamera,FFullScreen,ui.wdtVideo);
	ui.wdtVideo->setLayout(FVideoLayout);
	ui.wdtVideo->setMouseTracking(true);
	ui.wdtVideo->setVisible(false);

	FHideControllsTimer.setSingleShot(true);
	FHideControllsTimer.setInterval(HIDE_CONTROLLS_TIMEOT);
	connect(&FHideControllsTimer,SIGNAL(timeout()),SLOT(onHideControlsTimerTimeout()));

	connect(sipCall()->instance(),SIGNAL(stateChanged(int)),SLOT(onCallStateChanged(int)));
	connect(sipCall()->instance(),SIGNAL(deviceStateChanged(int, int)),SLOT(onCallDeviceStateChanged(int, int)));
	connect(sipCall()->instance(),SIGNAL(devicePropertyChanged(int, int, const QVariant &)),SLOT(onCallDevicePropertyChanged(int, int, const QVariant &)));

	setVideoVisible(false);
	setRecursiveMouseTracking(this);
	onCallStateChanged(sipCall()->state());
}

VideoCallWindow::~VideoCallWindow()
{
#ifdef Q_WS_MAC
	setWindowFullScreen(this, false);
#endif
	setControllsVisible(true);

	sipCall()->rejectCall();
	sipCall()->instance()->deleteLater();
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
	if (FVideoVisible && !FCtrlWidget->isFullScreenMode())
	{
		FVideoLayout->saveLocalVideoGeometry();
		QString ns = CustomBorderStorage::isBordered(this) ? QString::null : QString("system-border");
		Options::setFileValue(window()->geometry(),"sipphone.videocall-window.geometry",ns);
	}

	QPropertyAnimation *animation = new QPropertyAnimation(window(),"windowOpacity");
	animation->setDuration(500);
	animation->setStartValue(1.0);
	animation->setEndValue(0.0);
	connect(animation,SIGNAL(finished()),window(),SLOT(close()));
	animation->start();
}

void VideoCallWindow::restoreWindowGeometryWithAnimation()
{
	if (canShowVideo())
	{
		QString ns = CustomBorderStorage::isBordered(this) ? QString::null : QString("system-border");
		QRect newGeometry = Options::fileValue("sipphone.videocall-window.geometry",ns).toRect();
		if (newGeometry.isEmpty())
			newGeometry = WidgetManager::alignGeometry(QSize(500,480),window());

		FAnimatingGeometry = true;
		setVideoVisible(true);
		ui.wdtVideo->setVisible(true);
		setWindowGeometryWithAnimation(newGeometry,200);
	}
}

void VideoCallWindow::setRecursiveMouseTracking(QWidget *AWidget)
{
	foreach(QObject *child, AWidget->children())
		if (child->isWidgetType())
			setRecursiveMouseTracking(qobject_cast<QWidget *>(child));
	AWidget->setMouseTracking(true);
}

void VideoCallWindow::setWindowGeometryWithAnimation(const QRect &AGeometry, int ADuration)
{
	FAnimatingGeometry = true;
	FVideoLayout->setVideoVisible(false);

	QPropertyAnimation *animation = new QPropertyAnimation(window(),"geometry");
	animation->setDuration(ADuration);
	animation->setStartValue(window()->geometry());
	animation->setEndValue(AGeometry);
	connect(animation,SIGNAL(finished()),animation,SLOT(deleteLater()));
	connect(animation,SIGNAL(finished()),FVideoLayout,SLOT(restoreLocalVideoGeometry()));
	connect(animation,SIGNAL(finished()),SLOT(onGeometryAnimationFinished()));
	animation->start();
}

bool VideoCallWindow::canShowVideo() const
{
	return sipCall()->state()==ISipCall::CS_TALKING && sipCall()->deviceState(ISipDevice::DT_REMOTE_CAMERA)!=ISipDevice::DS_UNAVAIL;
}

void VideoCallWindow::setVideoVisible(bool AVisible, bool AResizing)
{
	if (FVideoVisible != AVisible)
	{
		FVideoVisible = AVisible;
		if (AResizing)
		{
			QRect newGeometry = window()->geometry();
			int hDelta = !AVisible ? ui.wdtVideo->height() : FRemoteCamera->minimumVideoSize().height()+5;
			QPoint cursorPos = QCursor::pos();
			if (mapFromGlobal(cursorPos).y() < 5)
			{
				cursorPos.ry() += !AVisible ? hDelta : -hDelta;
				newGeometry.setTop(newGeometry.top() + (!AVisible ? hDelta : -hDelta));
			}
			else
			{
				cursorPos.ry() -= !AVisible ? hDelta : -hDelta;
				newGeometry.setBottom(newGeometry.bottom() - (!AVisible ? hDelta : -hDelta));
			}

			FAnimatingGeometry = true;
			FVideoLayout->setVideoVisible(false);
			QCursor::setPos(cursorPos);
			QTimer::singleShot(50,this,SLOT(onGeometryAnimationFinished()));
		}
		else
		{
			FVideoLayout->setVideoVisible(AVisible);
		}

		ui.wdtBackground->setProperty("videovisible",AVisible);
		StyleStorage::updateStyle(this);
	}
}

void VideoCallWindow::setControllsVisible(bool AVisible)
{
	if (FControllsVisible != AVisible)
	{
		if (AVisible)
		{
			FControllsVisible = AVisible;
			FCtrlWidget->setVisible(true);
			qApp->restoreOverrideCursor();
		}
		else if (FCtrlWidget->isFullScreenMode())
		{
			FControllsVisible = AVisible;
			FCtrlWidget->setVisible(false);
			qApp->setOverrideCursor(Qt::BlankCursor);
		}
	}
}

void VideoCallWindow::showEvent(QShowEvent *AEvent)
{
	if (FIsFirstShow)
		window()->adjustSize();
	FIsFirstShow = false;
	QWidget::showEvent(AEvent);
}

void VideoCallWindow::resizeEvent(QResizeEvent *AEvent)
{
	QWidget::resizeEvent(AEvent);
	if (!FAnimatingGeometry && FBlockVideoChange==0)
	{
		if (ui.wdtControls->height()>0 && (ui.wdtVideo->height()>0 || !FVideoVisible))
		{
			if (FVideoVisible && ui.wdtVideo->height()<FRemoteCamera->minimumVideoSize().height()+2)
			{
				FBlockVideoChange++;
				setVideoVisible(false,true);
			}
			else if (!FVideoVisible && canShowVideo() && ui.wdtVideo->height()>2)
			{
				FBlockVideoChange++;
				setVideoVisible(true,true);
			}
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

		setControllsVisible(true);
	}
	QWidget::mouseMoveEvent(AEvent);
}

void VideoCallWindow::onCallStateChanged(int AState)
{
	switch (AState)
	{
	case ISipCall::CS_TALKING:
		restoreWindowGeometryWithAnimation();
		break;
	case ISipCall::CS_FINISHED:
	case ISipCall::CS_ERROR:
		closeWindowWithAnimation();
		break;
	default:
		break;
	}

	if (CustomBorderStorage::isBordered(window()))
	{
		if (AState==ISipCall::CS_TALKING)
		{
			FVideoLayout->setButtonsPadding(25);
			CustomBorderStorage::widgetBorder(window())->setMinimizeButtonVisible(true);
		}
		else
		{
			FVideoLayout->setButtonsPadding(5);
			CustomBorderStorage::widgetBorder(window())->setMinimizeButtonVisible(false);
		}
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
#ifdef Q_WS_MAC
	setWindowFullScreen(this, !isWindowFullScreen(this));
	FCtrlWidget->setFullScreenMode(isWindowFullScreen(this));
#else
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

		setControllsVisible(true);
		FCtrlWidget->setFullScreenMode(false);
		FCtrlWidget->setParent(ui.wdtControls);
		ui.wdtControls->layout()->addWidget(FCtrlWidget);
		FCtrlWidget->setVisible(true);

		if (CustomBorderStorage::isBordered(window()))
			CustomBorderStorage::widgetBorder(window())->showFullScreen();
		else
			window()->showNormal();
	}
#endif
}

void VideoCallWindow::onHideControlsTimerTimeout()
{
	setControllsVisible(false);
}

void VideoCallWindow::onGeometryAnimationFinished()
{
	FAnimatingGeometry = false;
	FVideoLayout->setVideoVisible(FVideoVisible);
}
