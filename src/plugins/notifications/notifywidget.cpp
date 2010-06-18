#include "notifywidget.h"

#include <QTimer>

#define DEFAUTL_TIMEOUT           8000
#define ANIMATE_STEPS             17
#define ANIMATE_TIME              700
#define ANIMATE_STEP_TIME         (ANIMATE_TIME/ANIMATE_STEPS)
#define ANIMATE_OPACITY_START     0.0
#define ANIMATE_OPACITY_END       0.9
#define ANIMATE_OPACITY_STEP      (ANIMATE_OPACITY_END - ANIMATE_OPACITY_START)/ANIMATE_STEPS

QList<NotifyWidget *> NotifyWidget::FWidgets;
QDesktopWidget *NotifyWidget::FDesktop = new QDesktopWidget;

NotifyWidget::NotifyWidget(const INotification &ANotification) : QWidget(NULL, Qt::ToolTip|Qt::WindowStaysOnTopHint)
{
	ui.setupUi(this);
	setFocusPolicy(Qt::NoFocus);
	setAttribute(Qt::WA_DeleteOnClose,true);

	QPalette pallete = ui.frmWindowFrame->palette();
	pallete.setColor(QPalette::Window, pallete.color(QPalette::Base));
	ui.frmWindowFrame->setPalette(pallete);

	FYPos = -1;
	FAnimateStep = -1;
	setNotification(ANotification);
	connect(ui.closeButton, SIGNAL(clicked()), SIGNAL(closeButtonCLicked()));
	connect(ui.settingsButton, SIGNAL(clicked()), SIGNAL(settingsButtonCLicked()));
	deleteTimer = new QTimer(this);
}

NotifyWidget::~NotifyWidget()
{
	FWidgets.removeAll(this);
	layoutWidgets();
	emit windowDestroyed();
}

void NotifyWidget::appear()
{
	if (!FWidgets.contains(this))
	{
		QTimer *timer = new QTimer(this);
		timer->setSingleShot(false);
		timer->setInterval(ANIMATE_STEP_TIME);
		timer->start();
		connect(timer,SIGNAL(timeout()),SLOT(onAnimateStep()));

		if (FTimeOut > 0)
		{
			deleteTimer->setSingleShot(true);
			deleteTimer->setInterval(FTimeOut);
			connect(deleteTimer, SIGNAL(timeout()), SLOT(deleteLater()));
			deleteTimer->start();
		}

		setWindowOpacity(ANIMATE_OPACITY_START);

		FWidgets.prepend(this);
		layoutWidgets();
	}
}

void NotifyWidget::animateTo(int AYPos)
{
	if (FYPos != AYPos)
	{
		FYPos = AYPos;
		FAnimateStep = ANIMATE_STEPS;
	}
}

INotification NotifyWidget::notification() const
{
	return FNotification;
}

void NotifyWidget::setNotification(const INotification & notification)
{
	FNotification = notification;
	QIcon icon = qvariant_cast<QIcon>(FNotification.data.value(NDR_ICON));
	QString iconKey = FNotification.data.value(NDR_ICON_KEY).toString();
	QString iconStorage = FNotification.data.value(NDR_ICON_STORAGE).toString();
	QImage image = qvariant_cast<QImage>(FNotification.data.value(NDR_POPUP_IMAGE));
	QString caption = FNotification.data.value(NDR_POPUP_CAPTION,tr("Notification")).toString();
	QString title = FNotification.data.value(NDR_POPUP_TITLE).toString();
	QString text = FNotification.data.value(NDR_POPUP_TEXT).toString();
	FTimeOut = FNotification.data.value(NDR_POPUP_TIMEOUT,DEFAUTL_TIMEOUT).toInt();

	if (!caption.isEmpty())
		ui.notifyCaption->setText(caption);
	else
		ui.notifyCaption->setVisible(false);

	if (!iconKey.isEmpty() && !iconStorage.isEmpty())
		IconStorage::staticStorage(iconStorage)->insertAutoIcon(ui.lblIcon,iconKey,0,0,"pixmap");
	else if (!icon.isNull())
		ui.lblIcon->setPixmap(icon.pixmap(icon.availableSizes().value(0)));
	else
		ui.lblIcon->setVisible(false);

	if (!title.isEmpty())
		ui.lblTitle->setText(title);
	else
		ui.lblTitle->setVisible(false);

	if (!text.isEmpty())
	{
		FTextMessages.append(text);
		if (!image.isNull())
			ui.lblImage->setPixmap(QPixmap::fromImage(image.scaled(ui.lblImage->maximumSize(),Qt::KeepAspectRatio)));
		else
			ui.lblImage->setVisible(false);
		ui.lblText->setText(text);
	}
	else
	{
		ui.lblImage->setVisible(false);
		ui.lblText->setVisible(false);
	}
}

void NotifyWidget::appendText(const QString & text)
{
	FTextMessages.append(text);
	if (FTextMessages.count() > 3)
		FTextMessages.takeFirst();
	QString fullText = FTextMessages.join("\n\n");
	//QRect oldGeometry = geometry();
	if (!fullText.isEmpty())
	{
		ui.lblText->setText(fullText);
	}
	else
	{
		ui.lblImage->setVisible(false);
		ui.lblText->setVisible(false);
	}
	if (deleteTimer->isActive())
	{
		deleteTimer->stop();
		deleteTimer->start();
	}
	adjustSize();
	//setGeometry(oldGeometry.translated(0, geometry().bottom() - oldGeometry.bottom()));
	//layoutWidgets();
}

NotifyWidget* NotifyWidget::findNotifyWidget(Jid AStreamJid, Jid AContactJid)
{
	foreach(NotifyWidget * widget, FWidgets)
		if (((Jid(widget->notification().data.value(NDR_STREAM_JID).toString()) && AStreamJid)) && (Jid(widget->notification().data.value(NDR_CONTACT_JID).toString()) && AContactJid))
			return widget;
	return 0;
}

void NotifyWidget::mouseReleaseEvent(QMouseEvent *AEvent)
{
	QWidget::mouseReleaseEvent(AEvent);
	if (AEvent->button() == Qt::LeftButton)
		emit notifyActivated();
	else if (AEvent->button() == Qt::RightButton)
		emit notifyRemoved();
}

void NotifyWidget::resizeEvent(QResizeEvent * event)
{
	QWidget::resizeEvent(event);
	layoutWidgets();
}

void NotifyWidget::onAnimateStep()
{
	if (FAnimateStep > 0)
	{
		int ypos = y()+(FYPos-y())/(FAnimateStep);
		setWindowOpacity(qMin(windowOpacity()+ANIMATE_OPACITY_STEP, ANIMATE_OPACITY_END));
		move(x(),ypos);
		FAnimateStep--;
	}
	else if (FAnimateStep == 0)
	{
		move(x(),FYPos);
		setWindowOpacity(ANIMATE_OPACITY_END);
		FAnimateStep--;
	}
}

void NotifyWidget::layoutWidgets()
{
	QRect display = FDesktop->availableGeometry();
	int ypos = display.bottom();
	for (int i=0; ypos>0 && i<FWidgets.count(); i++)
	{
		NotifyWidget *widget = FWidgets.at(i);
		if (!widget->isVisible())
		{
			widget->show();
			WidgetManager::raiseWidget(widget);
			widget->move(display.right() - widget->frameGeometry().width(), display.bottom());
		}
		ypos -=  widget->frameGeometry().height();
		widget->animateTo(ypos);
	}
}
