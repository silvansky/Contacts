#include "notifywidget.h"

#include <QTimer>
#include <QTextFrame>
#include <QFontMetrics>
#include <QTextDocumentFragment>

#define DEFAUTL_TIMEOUT           9
#define ANIMATE_STEPS             20
#define ANIMATE_TIME              1000
#define ANIMATE_STEP_TIME         (ANIMATE_TIME/ANIMATE_STEPS)
#define ANIMATE_OPACITY_START     0.0
#define ANIMATE_OPACITY_END       1.0
#define ANIMATE_OPACITY_STEP      (ANIMATE_OPACITY_END - ANIMATE_OPACITY_START)/ANIMATE_STEPS

#define IMAGE_SIZE                QSize(32,32)
#define MAX_MESSAGES              3

QList<NotifyWidget *> NotifyWidget::FWidgets;
QDesktopWidget *NotifyWidget::FDesktop = new QDesktopWidget;

NotifyWidget::NotifyWidget(const INotification &ANotification) : QWidget(NULL, Qt::ToolTip|Qt::WindowStaysOnTopHint)
{
	ui.setupUi(this);
	setFocusPolicy(Qt::NoFocus);
	setAttribute(Qt::WA_DeleteOnClose,true);
	StyleStorage::staticStorage(RSR_STORAGE_STYLESHEETS)->insertAutoStyle(this,STS_NOTIFICATION_NOTIFYWIDGET);
	IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->insertAutoIcon(ui.tlbOptions,MNI_NOTIFICATIONS_POPUP_OPTIONS);
	
	ui.tbrText->setMaxHeight(FDesktop->availableGeometry().height() / 6);
	ui.tbrText->setContentsMargins(0,0,0,0);

	FYPos = -1;
	FAnimateStep = -1;
	FTimeOut = ANotification.data.value(NDR_POPUP_TIMEOUT,DEFAUTL_TIMEOUT).toInt();

	FCloseTimer = new QTimer(this);
	FCloseTimer->setInterval(1000);
	FCloseTimer->setSingleShot(false);
	connect(FCloseTimer, SIGNAL(timeout()), SLOT(onCloseTimerTimeout()));

	ui.lblIcon->setVisible(false);
	FCaption = ANotification.data.value(NDR_POPUP_CAPTION).toString();
	FTitle = ANotification.data.value(NDR_POPUP_TITLE).toString();
	updateElidedText();

	QImage image = qvariant_cast<QImage>(ANotification.data.value(NDR_POPUP_IMAGE));
	if (!image.isNull())
	{
		ui.lblImage->setFixedWidth(IMAGE_SIZE.width());
		ui.lblImage->setPixmap(QPixmap::fromImage(image.scaled(IMAGE_SIZE,Qt::KeepAspectRatio,Qt::SmoothTransformation)));
	}
	else
	{
		ui.lblImage->setVisible(false);
	}

	QString styleKey = ANotification.data.value(NDR_POPUP_STYLEKEY).toString();
	if (!styleKey.isEmpty())
		StyleStorage::staticStorage(RSR_STORAGE_STYLESHEETS)->insertAutoStyle(ui.frmPopup,styleKey);

	connect(ui.clbClose, SIGNAL(clicked()), SIGNAL(notifyRemoved()));
	connect(ui.tlbOptions, SIGNAL(clicked()), SIGNAL(showOptions()));

	appendNotification(ANotification);
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
			FCloseTimer->start();

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

void NotifyWidget::appendAction(Action *AAction)
{
	ActionButton *button = new ActionButton(AAction,ui.wdtButtons);
	ui.wdtButtons->layout()->addWidget(button);
}

void NotifyWidget::appendNotification(const INotification &ANotification)
{
	QString text = ANotification.data.value(NDR_POPUP_TEXT).toString().trimmed();
	if (!text.isEmpty())
	{
		QTextDocument doc;
		doc.setHtml(text);
		if (doc.rootFrame()->lastPosition() > 140)
		{
			QTextCursor cursor(&doc);
			cursor.movePosition(QTextCursor::NextCharacter,QTextCursor::MoveAnchor,120);
			cursor.movePosition(QTextCursor::End,QTextCursor::KeepAnchor);
			cursor.removeSelectedText();
			cursor.insertText("...");
			text = getHtmlBody(doc.toHtml());
		}

		FTextMessages.append(text);
		while(FTextMessages.count() > MAX_MESSAGES)
			FTextMessages.removeAt(0);
	}
	QString html = FTextMessages.join("<br>");
	ui.tbrText->setHtml(html);
	ui.tbrText->setVisible(!html.isEmpty());
	QTimer::singleShot(0,this,SLOT(adjustHeight()));
}

void NotifyWidget::adjustHeight()
{
	resize(width(),sizeHint().height());
}

void NotifyWidget::updateElidedText()
{
	ui.lblCaption->setText(ui.lblCaption->fontMetrics().elidedText(FCaption,Qt::ElideRight,ui.lblCaption->width() - ui.lblCaption->frameWidth()*2));
	ui.lblTitle->setText(ui.lblTitle->fontMetrics().elidedText(FTitle,Qt::ElideRight,ui.lblTitle->width() - ui.lblTitle->frameWidth()*2));
}

void NotifyWidget::enterEvent(QEvent *AEvent)
{
	FTimeOut += 14;
	QWidget::enterEvent(AEvent);
}

void NotifyWidget::leaveEvent(QEvent *AEvent)
{
	FTimeOut -= 14;
	QWidget::leaveEvent(AEvent);
}

void NotifyWidget::mouseReleaseEvent(QMouseEvent *AEvent)
{
	QWidget::mouseReleaseEvent(AEvent);
	if (AEvent->button() == Qt::LeftButton)
		emit notifyActivated();
	else if (AEvent->button() == Qt::RightButton)
		emit notifyRemoved();
}

void NotifyWidget::resizeEvent(QResizeEvent *AEvent)
{
	QWidget::resizeEvent(AEvent);
	updateElidedText();
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

void NotifyWidget::onCloseTimerTimeout()
{
	if (FTimeOut > 0)
		FTimeOut--;
	else
		close();
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
			QTimer::singleShot(0,widget,SLOT(adjustHeight()));
			QTimer::singleShot(10,widget,SLOT(adjustHeight()));
		}
		ypos -=  widget->frameGeometry().height();
		widget->animateTo(ypos--);
	}
}

