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
		setWindowFlags((windowFlags() & ~(Qt::WindowCloseButtonHint|Qt::WindowMaximizeButtonHint)) | Qt::WindowStaysOnTopHint | Qt::CustomizeWindowHint);
#else
		setWindowOntop(this, true);
		setWindowShownOnAllSpaces(this, true);
		setWindowGrowButtonEnabled(this, false);
		connect(MacUtils::instance(), SIGNAL(windowFullScreenModeChanged(QWidget*,bool)), SLOT(onWindowFullScreenModeChanged(QWidget*,bool)));
		connect(MacUtils::instance(), SIGNAL(windowFullScreenModeWillChange(QWidget*,bool)), SLOT(onWindowFullScreenModeWillChange(QWidget*,bool)));
#endif
	}

	FVideoShown = false;
	FFirstShow = true;
	FVideoVisible = true;
	FFirstRestore = true;
	FBlockVideoChange = 0;
	FAnimatingGeometry = false;

	FRemoteCamera = new VideoFrame(ui.wdtVideo);
	FRemoteCamera->setMinimumVideoSize(QSize(100,100));
	FRemoteCamera->setObjectName("vlbRemoteCamera");
	connect(FRemoteCamera,SIGNAL(doubleClicked()),SLOT(onFullScreenButtonClicked()));

	FLocalCamera = new VideoFrame(ui.wdtVideo);
	FLocalCamera->setMoveEnabled(true);
	FLocalCamera->setResizeEnabled(true);
	FLocalCamera->setFrameShape(QLabel::Box);
	FLocalCamera->setObjectName("vlbLocalCamera");

	FCtrlWidget = new CallControlWidget(APluginManager,ASipCall,ui.wdtControls);
	FCtrlWidget->setMinimumWidthMode(true);
	ui.wdtControls->setLayout(new QHBoxLayout);
	ui.wdtControls->layout()->setMargin(0);
	ui.wdtControls->layout()->addWidget(FCtrlWidget);
	connect(FCtrlWidget,SIGNAL(silentButtonClicked()),SLOT(onSilentButtonClicked()));
	connect(FCtrlWidget,SIGNAL(chatWindowRequested()),SIGNAL(chatWindowRequested()));

#if !defined(Q_WS_MAC) || (defined (Q_WS_MAC) && !defined(__MAC_OS_X_NATIVE_FULLSCREEN))
	QWidget *videoButtons = new QWidget(ui.wdtVideo);
	videoButtons->setLayout(new QHBoxLayout);
	videoButtons->layout()->setMargin(0);
	videoButtons->layout()->setSpacing(0);

	FFSButton = new QToolButton(videoButtons);
	FFSButton->setObjectName("tlbFullScreen");
	FFSButton->setIcon(IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getIcon(MNI_SIPPHONE_CALL_FULLSCREEN));
	connect(FFSButton,SIGNAL(clicked()),SLOT(onFullScreenButtonClicked()));
	videoButtons->layout()->addWidget(FFSButton);

# if !defined(Q_WS_MAC)
	FMinButton = new QToolButton(videoButtons);
	FMinButton->setObjectName("tlbMinimize");
	FMinButton->setToolTip(tr("Minimize"));
	FMinButton->setIcon(IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getIcon(MNI_SIPPHONE_CALL_MINIMIZE));
	connect(FMinButton,SIGNAL(clicked()),SLOT(onMinimizeButtonClicked()));
	videoButtons->layout()->addWidget(FMinButton);
# else
	FMinButton = NULL;
# endif

#elif defined (Q_WS_MAC)
	FFSButton = NULL;
	FMinButton = NULL;
	QWidget *videoButtons = NULL;
#endif

	FVideoLayout = new VideoLayout(FRemoteCamera,FLocalCamera,videoButtons,ui.wdtVideo);
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
	setControlsVisible(true);

	sipCall()->destroyCall();
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

void VideoCallWindow::saveWindowGeometry()
{
	if (!FFirstRestore && !FCtrlWidget->isFullScreenMode())
	{
		QString ns = CustomBorderStorage::isBordered(this) ? QString::null : QString("system-border");
		Options::setFileValue(ui.wdtControls->width(),"sipphone.videocall-window.control-width",ns);
		Options::setFileValue(mapToGlobal(ui.wdtControls->geometry().topLeft()),"sipphone.videocall-window.control-top-left",ns);
		if (FVideoVisible && FVideoShown)
		{
			FVideoLayout->saveLocalVideoGeometry();
			Options::setFileValue(ui.wdtVideo->height(),"sipphone.videocall-window.video-height",ns);
		}
	}
}

void VideoCallWindow::restoreWindowGeometryWithAnimation(bool AShowVideo)
{
	if (FFirstRestore || FVideoShown!=AShowVideo)
	{
		QRect newGeometry = window()->geometry();
		QString ns = CustomBorderStorage::isBordered(this) ? QString::null : QString("system-border");
		if (FFirstRestore)
		{
			int ctrlWidth = Options::fileValue("sipphone.videocall-window.control-width",ns).toInt();
			QPoint ctrlTopLeft = Options::fileValue("sipphone.videocall-window.control-top-left",ns).toPoint();
			if (ctrlWidth>0 && !ctrlTopLeft.isNull())
			{
				newGeometry.setWidth(newGeometry.width() + (ctrlWidth - ui.wdtControls->width()));
				newGeometry.moveTopLeft(newGeometry.topLeft() + (ctrlTopLeft - mapToGlobal(ui.wdtControls->geometry().topLeft())));
			}
			FVideoLayout->restoreLocalVideoGeometry();
		}
		else if (!FAnimatingGeometry)
		{
			saveWindowGeometry();
		}

		if (AShowVideo)
		{
			int videoHeight = Options::fileValue("sipphone.videocall-window.video-height",ns).toInt();
			videoHeight = videoHeight>100 ? videoHeight : 400;
			newGeometry.setTop(newGeometry.top() - videoHeight);
		}
		else if (FVideoShown)
		{
			newGeometry.setTop(newGeometry.top() + ui.wdtVideo->height());
		}
		newGeometry = WidgetManager::correctWindowGeometry(newGeometry,this);

		FAnimatingGeometry = true;
		setVideoVisible(AShowVideo);
		setWindowGeometryWithAnimation(newGeometry,200);
	}
}

void VideoCallWindow::closeWindowWithAnimation(int ATimeout)
{
	saveWindowGeometry();
	QPropertyAnimation *animation = new QPropertyAnimation(window(),"windowOpacity");
	animation->setDuration(500);
	animation->setStartValue(1.0);
	animation->setEndValue(0.0);
	connect(animation,SIGNAL(finished()),window(),SLOT(close()));
	connect(animation,SIGNAL(finished()),animation,SLOT(deleteLater()));
	QTimer::singleShot(ATimeout,animation,SLOT(start()));
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
	setVideoWidgetVisible(true);
	FVideoLayout->setVideoVisible(false);
	window()->setMaximumHeight(QWIDGETSIZE_MAX);

	QPropertyAnimation *animation = new QPropertyAnimation(window(),"geometry");
	animation->setDuration(ADuration);
	animation->setStartValue(window()->geometry());
	animation->setEndValue(AGeometry);
	connect(animation,SIGNAL(finished()),animation,SLOT(deleteLater()));
	connect(animation,SIGNAL(finished()),SLOT(onGeometryAnimationFinished()));
	connect(animation,SIGNAL(finished()),SLOT(onChangeVideoWidgetVisibility()));
	animation->start();
}

bool VideoCallWindow::isVideoAvailable() const
{
	return sipCall()->deviceState(ISipDevice::DT_REMOTE_CAMERA)==ISipDevice::DS_ENABLED || sipCall()->deviceState(ISipDevice::DT_LOCAL_CAMERA)==ISipDevice::DS_ENABLED;
}

void VideoCallWindow::toggleFullScreen(bool AFullScreen)
{
	setControlsFullScreenMode(AFullScreen);
#if defined(Q_WS_MAC) && defined(__MAC_OS_X_NATIVE_FULLSCREEN)
	setWindowFullScreen(this, AFullScreen);
#else
	if (CustomBorderStorage::isBordered(window()))
		CustomBorderStorage::widgetBorder(window())->showFullScreen();
	else if (AFullScreen)
		window()->showFullScreen();
	else
		window()->showNormal();
#endif
}

void VideoCallWindow::setWindowResizeEnabled(bool AEnabled)
{
	if (AEnabled && CustomBorderStorage::isBordered(window()))
		IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->insertAutoIcon(ui.lblResizer,MNI_SIPPHONE_CALL_RESIZE,0,0,"pixmap");
	else
		IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->removeAutoIcon(ui.lblResizer);
	window()->setMaximumHeight(AEnabled ? QWIDGETSIZE_MAX : window()->minimumSizeHint().height());
}

void VideoCallWindow::setVideoWidgetVisible(bool AVisible)
{
	if (FVideoShown != AVisible)
	{
		FVideoShown = AVisible;
		ui.wdtVideo->setVisible(AVisible);
		setWindowResizeEnabled(AVisible);
	}
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
			QTimer::singleShot(10,this,SLOT(onGeometryAnimationFinished()));
		}
		else
		{
			FVideoLayout->setVideoVisible(AVisible);
		}

#ifdef Q_WS_MAC
		setWindowFullScreenEnabled(this, AVisible);
		setWindowGrowButtonEnabled(this, AVisible);
#endif

		if (CustomBorderStorage::isBordered(window()))
			CustomBorderStorage::widgetBorder(window())->setMinimizeButtonVisible(!FVideoVisible && sipCall()->state()==ISipCall::CS_TALKING);

		FRemoteCamera->setNullVideoImage(FCtrlWidget->contactAvatar());

		ui.wdtBackground->setProperty("videovisible",AVisible);
		StyleStorage::updateStyle(this);
	}
}

void VideoCallWindow::setControlsVisible(bool AVisible)
{
	if (FVideoLayout->isControlsVisible() != AVisible)
	{
		if (AVisible)
		{
			qApp->restoreOverrideCursor();
			FVideoLayout->setControlsVisible(true);
		}
		else if (FCtrlWidget->isFullScreenMode())
		{
			qApp->setOverrideCursor(Qt::BlankCursor);
			FVideoLayout->setControlsVisible(false);
		}
	}
}

void VideoCallWindow::setControlsFullScreenMode(bool AEnabled)
{
	if (AEnabled)
	{
		ui.wdtControls->layout()->removeWidget(FCtrlWidget);

		setControlsVisible(true);
		FCtrlWidget->setFullScreenMode(true);
		FCtrlWidget->setMinimumWidthMode(false);
		FCtrlWidget->setParent(ui.wdtVideo);
		FVideoLayout->setControlsWidget(FCtrlWidget);
		FCtrlWidget->setVisible(true);

		FFSButton->setToolTip(tr("Enter Full Screen"));
	}
	else
	{
		FVideoLayout->setControlsWidget(NULL);

		setControlsVisible(true);
		FCtrlWidget->setFullScreenMode(false);
		FCtrlWidget->setMinimumWidthMode(true);
		FCtrlWidget->setParent(ui.wdtControls);
		ui.wdtControls->layout()->addWidget(FCtrlWidget);
		FCtrlWidget->setVisible(true);

		FFSButton->setToolTip(tr("Exit Full Screen"));
	}
}

void VideoCallWindow::showEvent(QShowEvent *AEvent)
{
	if (FFirstShow)
	{
		setWindowResizeEnabled(false);
		window()->adjustSize();
	}
	FFirstShow = false;
	QWidget::showEvent(AEvent);
}

void VideoCallWindow::resizeEvent(QResizeEvent *AEvent)
{
	QWidget::resizeEvent(AEvent);
	if (!FAnimatingGeometry && FVideoShown && FBlockVideoChange==0)
	{
		if (ui.wdtControls->height()>0 && (ui.wdtVideo->height()>0 || !FVideoVisible))
		{
			if (FVideoVisible && ui.wdtVideo->height()<FRemoteCamera->minimumVideoSize().height()+2)
			{
				FBlockVideoChange++;
				setVideoVisible(false,true);
			}
			else if (!FVideoVisible && ui.wdtVideo->height()>2)
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

		setControlsVisible(true);
	}
	QWidget::mouseMoveEvent(AEvent);
}

void VideoCallWindow::onCallStateChanged(int AState)
{
	switch (AState)
	{
	case ISipCall::CS_TALKING:
		if (CustomBorderStorage::isBordered(window()))
			CustomBorderStorage::widgetBorder(window())->setMinimizeButtonVisible(!FVideoVisible);
		restoreWindowGeometryWithAnimation(isVideoAvailable());
		break;
	case ISipCall::CS_FINISHED:
		closeWindowWithAnimation();
		break;
	case ISipCall::CS_ERROR:
		closeWindowWithAnimation(5000);
		break;
	default:
		break;
	}
	setWindowTitle(FCtrlWidget->windowTitle());
}

void VideoCallWindow::onCallDeviceStateChanged(int AType, int AState)
{
	if (AType==ISipDevice::DT_REMOTE_CAMERA)
	{
		FRemoteCamera->setVideoDeviceState(AState);
	}
	else if (AType==ISipDevice::DT_LOCAL_CAMERA)
	{
		FLocalCamera->setVideoDeviceState(AState);
	}

	if (sipCall()->state() == ISipCall::CS_TALKING)
		restoreWindowGeometryWithAnimation(isVideoAvailable());
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

void VideoCallWindow::onMinimizeButtonClicked()
{
	if (CustomBorderStorage::isBordered(window()))
		CustomBorderStorage::widgetBorder(window())->minimizeWidget();
	else
		window()->showMinimized();
}

void VideoCallWindow::onFullScreenButtonClicked()
{
	bool fullScreen = !FCtrlWidget->isFullScreenMode() && !FRemoteCamera->isEmpty();
	if (fullScreen != FCtrlWidget->isFullScreenMode())
	{
		if (fullScreen)
			FNormalGeometry = window()->geometry();

		toggleFullScreen(fullScreen);

		if (FMinButton)
			FMinButton->setVisible(!fullScreen);

		if (!fullScreen)
			setWindowGeometryWithAnimation(FNormalGeometry,10);
		else
			FHideControllsTimer.start();
	}
}

void VideoCallWindow::onHideControlsTimerTimeout()
{
	setControlsVisible(false);
}

void VideoCallWindow::onGeometryAnimationFinished()
{
	FFirstRestore = false;
	FAnimatingGeometry = false;
	FVideoLayout->setVideoVisible(FVideoVisible);
}

void VideoCallWindow::onChangeVideoWidgetVisibility()
{
	setVideoWidgetVisible(FVideoVisible);
}

#ifdef Q_WS_MAC
void VideoCallWindow::onWindowFullScreenModeWillChange(QWidget *window, bool fullScreen)
{
	if (window == this->window())
	{
		setControlsFullScreenMode(fullScreen);
	}
}

void VideoCallWindow::onWindowFullScreenModeChanged(QWidget *window, bool fullScreen)
{
	Q_UNUSED(window)
	Q_UNUSED(fullScreen)
}
#endif
