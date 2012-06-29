#include "phonecallwindow.h"

#include <QPropertyAnimation>
#include <definitions/resources.h>
#include <definitions/menuicons.h>
#include <definitions/stylesheets.h>
#include <definitions/customborder.h>
#include <utils/stylestorage.h>
#include <utils/widgetmanager.h>
#include <utils/customborderstorage.h>

PhoneCallWindow::PhoneCallWindow(IPluginManager *APluginManager, ISipCall *ASipCall, QWidget *AParent) : QWidget(AParent)
{
	ui.setupUi(this);
	StyleStorage::staticStorage(RSR_STORAGE_STYLESHEETS)->insertAutoStyle(this,STS_SIPPHONE_PHONECALLWINDOW);

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
#endif
	}

	FCtrlWidget = new CallControlWidget(APluginManager,ASipCall,ui.wdtControls);
	FCtrlWidget->setMinimumWidthMode(true);
	ui.wdtControls->setLayout(new QHBoxLayout);
	ui.wdtControls->layout()->setMargin(0);
	ui.wdtControls->layout()->addWidget(FCtrlWidget);

	connect(sipCall()->instance(),SIGNAL(stateChanged(int)),SLOT(onCallStateChanged(int)));
	onCallStateChanged(sipCall()->state());
}

PhoneCallWindow::~PhoneCallWindow()
{
	sipCall()->destroyCall();
}

ISipCall *PhoneCallWindow::sipCall() const
{
	return FCtrlWidget->sipCall();
}

QSize PhoneCallWindow::sizeHint() const
{
	static const QSize minHint(460,1);
	return QWidget::sizeHint().expandedTo(minHint);
}

void PhoneCallWindow::saveWindowGeometry()
{
	if (!FFirstRestore)
	{
		QString ns = CustomBorderStorage::isBordered(this) ? QString::null : QString("system-border");
		Options::setFileValue(ui.wdtControls->width(),"sipphone.phonecall-window.control-width",ns);
		Options::setFileValue(mapToGlobal(ui.wdtControls->geometry().topLeft()),"sipphone.phonecall-window.control-top-left",ns);
	}
}

void PhoneCallWindow::restoreWindowGeometryWithAnimation()
{
	if (FFirstRestore)
	{
		FFirstRestore = false;
		QRect newGeometry = window()->geometry();
		QString ns = CustomBorderStorage::isBordered(this) ? QString::null : QString("system-border");
		int ctrlWidth = Options::fileValue("sipphone.phonecall-window.control-width",ns).toInt();
		QPoint ctrlTopLeft = Options::fileValue("sipphone.phonecall-window.control-top-left",ns).toPoint();
		if (ctrlWidth>0 && !ctrlTopLeft.isNull())
		{
			newGeometry.setWidth(newGeometry.width() + (ctrlWidth - ui.wdtControls->width()));
			newGeometry.moveTopLeft(newGeometry.topLeft() + (ctrlTopLeft - mapToGlobal(ui.wdtControls->geometry().topLeft())));
		}
		newGeometry = WidgetManager::correctWindowGeometry(newGeometry,this);
		setWindowGeometryWithAnimation(newGeometry,200);
	}
}

void PhoneCallWindow::closeWindowWithAnimation(int ATimeout)
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

void PhoneCallWindow::setWindowGeometryWithAnimation(const QRect &AGeometry, int ADuration)
{
	QPropertyAnimation *animation = new QPropertyAnimation(window(),"geometry");
	animation->setDuration(ADuration);
	animation->setStartValue(window()->geometry());
	animation->setEndValue(AGeometry);
	connect(animation,SIGNAL(finished()),animation,SLOT(deleteLater()));
	animation->start();
}

void PhoneCallWindow::setWindowResizeEnabled(bool AEnabled)
{
	if (AEnabled && CustomBorderStorage::isBordered(window()))
		IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->insertAutoIcon(ui.lblResizer,MNI_SIPPHONE_CALL_RESIZE,0,0,"pixmap");
	else
		IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->removeAutoIcon(ui.lblResizer);
	window()->setMaximumHeight(AEnabled ? QWIDGETSIZE_MAX : window()->minimumSizeHint().height());
}

void PhoneCallWindow::showEvent( QShowEvent *AEvent )
{
	if (FFirstShow)
	{
		setWindowResizeEnabled(false);
		window()->adjustSize();
	}
	FFirstShow = false;
	QWidget::showEvent(AEvent);
}

void PhoneCallWindow::onCallStateChanged(int AState)
{
	switch (AState)
	{
	case ISipCall::CS_TALKING:
		restoreWindowGeometryWithAnimation();
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
