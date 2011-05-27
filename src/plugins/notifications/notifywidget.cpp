#include "notifywidget.h"

#include <QTimer>
#include <QPainter>
#include <QTextFrame>
#include <QFontMetrics>
#include <QTextDocumentFragment>
#include <utils/customborderstorage.h>
#include <definitions/resources.h>
#include <definitions/customborder.h>

#define DEFAUTL_TIMEOUT           9
#define ANIMATE_STEPS             20
#define ANIMATE_TIME              1000
#define ANIMATE_STEP_TIME         (ANIMATE_TIME/ANIMATE_STEPS)
#define ANIMATE_OPACITY_START     0.0
#define ANIMATE_OPACITY_END       1.0
#define ANIMATE_OPACITY_STEP      ((ANIMATE_OPACITY_END - ANIMATE_OPACITY_START)/ANIMATE_STEPS)

#define IMAGE_SIZE                QSize(36,36)
#define MAX_MESSAGES              3

QList<NotifyWidget *> NotifyWidget::FWidgets;
QDesktopWidget *NotifyWidget::FDesktop = new QDesktopWidget;

NotifyWidget::NotifyWidget(const INotification &ANotification, bool AOptionsAvailable) :
	QWidget(NULL, Qt::ToolTip | Qt::WindowStaysOnTopHint)
{
	// TODO: remove
	Q_UNUSED(AOptionsAvailable)
	ui.setupUi(this);

	border = CustomBorderStorage::staticStorage(RSR_STORAGE_CUSTOMBORDER)->addBorder(this, CBS_NOTIFICATION);
	if (border)
	{
		border->setResizable(false);
		border->setMovable(false);
		border->setMinimizeButtonVisible(false);
		border->setMaximizeButtonVisible(false);
		border->setWindowFlags(border->windowFlags() | Qt::ToolTip);
		border->setAttribute(Qt::WA_DeleteOnClose, true);
		connect(border, SIGNAL(closeClicked()), SIGNAL(notifyRemoved()));
//		QMargins m = ui.frmPopup->layout()->contentsMargins();
//		m.setRight(14);
//		ui.frmPopup->layout()->setContentsMargins(m);
//		ui.clbClose->setVisible(false);
	}

	ui.wdtButtons->setVisible(ANotification.actions.count());

	setFocusPolicy(Qt::NoFocus);
	setAttribute(Qt::WA_DeleteOnClose,true);
	StyleStorage::staticStorage(RSR_STORAGE_STYLESHEETS)->insertAutoStyle(this,STS_NOTIFICATION_NOTIFYWIDGET);
//	IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->insertAutoIcon(ui.tlbOptions,MNI_NOTIFICATIONS_POPUP_OPTIONS);

	ui.tbrText->setMaxHeight(FDesktop->availableGeometry().height() / 6);
	ui.tbrText->setContentsMargins(0,0,0,0);

	FYPos = -1;
	FAnimateStep = -1;
	FTimeOut = ANotification.data.value(NDR_POPUP_TIMEOUT,DEFAUTL_TIMEOUT).toInt();

	FCloseTimer = new QTimer(this);
	FCloseTimer->setInterval(1000);
	FCloseTimer->setSingleShot(false);
	connect(FCloseTimer, SIGNAL(timeout()), SLOT(onCloseTimerTimeout()));

	FCaption = ANotification.data.value(NDR_POPUP_CAPTION).toString();
	FTitle = ANotification.data.value(NDR_POPUP_TITLE).toString();
	FNotice = ANotification.data.value(NDR_POPUP_NOTICE).toString();

	QIcon icon = qvariant_cast<QIcon>(ANotification.data.value(NDR_POPUP_ICON));
	if (!icon.isNull())
		ui.lblIcon->setPixmap(icon.pixmap(QSize(16,16)));
	else
		ui.lblIcon->setVisible(false);

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
	ui.lblCaption->setVisible(!FCaption.isEmpty());
	ui.lblTitle->setVisible(!FTitle.isEmpty());
	ui.lblNotice->setVisible(!FNotice.isEmpty());

	QString styleKey = ANotification.data.value(NDR_POPUP_STYLEKEY).toString();
	if (!styleKey.isEmpty())
		StyleStorage::staticStorage(RSR_STORAGE_STYLESHEETS)->insertAutoStyle(ui.frmPopup,styleKey);

//	ui.tlbOptions->setVisible(AOptionsAvailable);
//	connect(ui.tlbOptions, SIGNAL(clicked()), SIGNAL(showOptions()));
//	connect(ui.clbClose, SIGNAL(clicked()), SIGNAL(notifyRemoved()));

	appendNotification(ANotification);
	updateElidedText();
}

NotifyWidget::~NotifyWidget()
{
	FWidgets.removeAll(this);
	layoutWidgets();
	emit windowDestroyed();
}

// returns false if widget can't appear
bool NotifyWidget::appear()
{
	if (!FWidgets.contains(this) && (FWidgets.count() < MAX_MESSAGES))
	{
		QTimer *timer = new QTimer(this);
		timer->setSingleShot(false);
		timer->setInterval(ANIMATE_STEP_TIME);
		timer->start();
		connect(timer,SIGNAL(timeout()),SLOT(onAnimateStep()));

		if (FTimeOut > 0)
			FCloseTimer->start();

		if (border)
			border->setWindowOpacity(ANIMATE_OPACITY_START);
		else
			setWindowOpacity(ANIMATE_OPACITY_START);

		FWidgets.prepend(this);
		layoutWidgets();
		return true;
	}
	if (border)
		border->deleteLater();
	else
		deleteLater();
	return false;
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
	button->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
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
	if (border)
		border->resize(border->width(),border->sizeHint().height());
	else
		resize(width(),sizeHint().height());
}

void NotifyWidget::updateElidedText()
{
	ui.lblCaption->setText(ui.lblCaption->fontMetrics().elidedText(FCaption,Qt::ElideRight,ui.lblCaption->width() - ui.lblCaption->frameWidth()*2));
	ui.lblCaption->setVisible(!ui.lblCaption->text().isEmpty());
	ui.lblTitle->setText(ui.lblTitle->fontMetrics().elidedText(FTitle,Qt::ElideRight,ui.lblTitle->width() - ui.lblTitle->frameWidth()*2));
	ui.lblTitle->setVisible(!ui.lblTitle->text().isEmpty());
	ui.lblNotice->setText(ui.lblNotice->fontMetrics().elidedText(FNotice,Qt::ElideRight,ui.lblNotice->width() - ui.lblNotice->frameWidth()*2));
	ui.lblNotice->setVisible(!ui.lblNotice->text().isEmpty());
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

void NotifyWidget::paintEvent(QPaintEvent * pe)
{
	QStyleOption opt;
	opt.init(this);
	QPainter p(this);
	p.setClipRect(pe->rect());
	style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
}

void NotifyWidget::onAnimateStep()
{
	if (FAnimateStep > 0)
	{
		int ypos;
		if (border)
		{
			ypos = border->y()+(FYPos-border->y())/(FAnimateStep);
			border->setWindowOpacity(qMin(border->windowOpacity()+ANIMATE_OPACITY_STEP, ANIMATE_OPACITY_END));
			border->move(border->x(),ypos);
		}
		else
		{
			ypos = y()+(FYPos-y())/(FAnimateStep);
			setWindowOpacity(qMin(windowOpacity()+ANIMATE_OPACITY_STEP, ANIMATE_OPACITY_END));
			move(x(),ypos);
		}
		FAnimateStep--;
	}
	else if (FAnimateStep == 0)
	{
		if (border)
		{
			border->move(border->x(),FYPos);
			border->setWindowOpacity(ANIMATE_OPACITY_END);
		}
		else
		{
			move(x(),FYPos);
			setWindowOpacity(ANIMATE_OPACITY_END);
		}
		FAnimateStep--;
	}
}

void NotifyWidget::onCloseTimerTimeout()
{
	if (FTimeOut > 0)
		FTimeOut--;
	else
	{
		if (border)
			border->close();
		else
			close();
	}
}

void NotifyWidget::layoutWidgets()
{
	QRect display = FDesktop->availableGeometry();
	int ypos = display.bottom();
	for (int i = 0; ypos > 0 && i < FWidgets.count(); i++)
	{
		NotifyWidget *widget = FWidgets.at(i);
		if (!widget->isVisible())
		{
			if (widget->border)
				widget->border->show();
			else
				widget->show();
			if (widget->border)
				widget->border->move(display.right() - widget->border->geometry().width(), display.bottom());
			else
				widget->move(display.right() - widget->frameGeometry().width(), display.bottom());
			QTimer::singleShot(0, widget, SLOT(adjustHeight()));
			QTimer::singleShot(10, widget, SLOT(adjustHeight()));
		}
		if (widget->border)
			ypos -= widget->border->geometry().height();
		else
			ypos -= widget->frameGeometry().height();
		widget->animateTo(ypos--);
	}
}
