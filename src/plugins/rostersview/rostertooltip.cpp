#include "rostertooltip.h"

#include <QStyle>
#include <QToolTip>
#include <QKeyEvent>
#include <QVBoxLayout>
#include <QApplication>
#include <QStylePainter>
#include <QDesktopWidget>
#include <QStyleOptionFrame>

#define TIP_HIDE_TIME 500
#define TIP_SHOW_TIME 60000

RosterToolTip *RosterToolTip::instance = NULL;

RosterToolTip::RosterToolTip(QWidget *AParent) : QFrame(AParent, Qt::ToolTip | Qt::BypassGraphicsProxyWidget)
{
	delete instance;
	instance = this;

	ui.setupUi(this);
	StyleStorage::staticStorage(RSR_STORAGE_STYLESHEETS)->insertAutoStyle(this,STS_ROSTERVIEW_TOOLTIP);

	setMouseTracking(true);
	setFocusPolicy(Qt::NoFocus);
	setFrameStyle(QFrame::NoFrame);
	qApp->installEventFilter(this);

	setFont(QToolTip::font());
	setForegroundRole(QPalette::ToolTipText);
	setBackgroundRole(QPalette::ToolTipBase);
	setPalette(QToolTip::palette());

	ensurePolished();
	ui.lblToolTip->setIndent(1);
	setWindowOpacity(style()->styleHint(QStyle::SH_ToolTipLabel_Opacity, NULL, this) / 255.0);
	layout()->setMargin(1 + style()->pixelMetric(QStyle::PM_ToolTipLabelFrameWidth, NULL, this));

	FWidget = NULL;
	FMouseOver = false;

	QToolBar *toolBar = new QToolBar(this);
	toolBar->setObjectName("tlbActions");
	FToolBarChanger = new ToolBarChanger(toolBar);
	FToolBarChanger->setSeparatorsVisible(false);
	toolBar->setOrientation(Qt::Vertical);
	layout()->addWidget(toolBar);
}

RosterToolTip::~RosterToolTip()
{
	instance = NULL;
}

void RosterToolTip::createInstance(const QPoint &APos, QWidget *AWidget)
{
	if (instance == NULL)
	{
#ifndef Q_WS_WIN
		new RosterToolTip(AWidget);
#else
		new RosterToolTip(QApplication::desktop()->screen(getTipScreen(APos, AWidget)));
#endif
		instance->restartExpireTimer();
	}
}

ToolBarChanger *RosterToolTip::toolBarChanger()
{
	return instance!=NULL ? instance->FToolBarChanger : NULL;
}

void RosterToolTip::showTip(const QPoint &APos, const QString &AText, QWidget *AWidget, const QRect &ARect)
{
	if (instance && AText.isEmpty())
	{
		instance->hideTipImmediately();
	}
	else if (instance!=NULL && instance->isVisible())
	{
		QPoint localPos = AWidget!=NULL ? AWidget->mapFromGlobal(APos) : APos;
		if (instance->isTipChanged(localPos, AText, AWidget))
		{
			instance->reuseTip(AText);
			instance->setTipRect(AWidget, ARect);
			instance->placeTip(APos, AWidget);
		}
	}
	else if (!AText.isEmpty())
	{
		createInstance(APos,AWidget);
		instance->reuseTip(AText);
		instance->setTipRect(AWidget, ARect);
		instance->placeTip(APos, AWidget);
		instance->show();
	}
}

void RosterToolTip::hideTip()
{
	if (!FMouseOver && !FHideTimer.isActive())
		FHideTimer.start(TIP_HIDE_TIME, this);
}

void RosterToolTip::hideTipImmediately()
{
	deleteLater();
}

void RosterToolTip::restartExpireTimer()
{
	FHideTimer.stop();
	FExpireTimer.start(TIP_SHOW_TIME, this);
}

void RosterToolTip::reuseTip(const QString &AText)
{
	ui.lblToolTip->setText(AText);
	adjustSize();
	restartExpireTimer();
}

void RosterToolTip::placeTip(const QPoint &APos, QWidget *AWidget)
{
#ifdef Q_WS_MAC
	QRect screen = QApplication::desktop()->availableGeometry(getTipScreen(APos, AWidget));
#else
	QRect screen = QApplication::desktop()->screenGeometry(getTipScreen(APos, AWidget));
#endif

	QPoint p = APos;
#ifdef Q_WS_WIN
		p += QPoint(2,21);
#else
		p += QPoint(2,16);
#endif

	if (p.x() + this->width() > screen.x() + screen.width())
		p.rx() -= 4 + this->width();
	if (p.y() + this->height() > screen.y() + screen.height())
		p.ry() -= 24 + this->height();
	if (p.y() < screen.y())
		p.setY(screen.y());
	if (p.x() + this->width() > screen.x() + screen.width())
		p.setX(screen.x() + screen.width() - this->width());
	if (p.x() < screen.x())
		p.setX(screen.x());
	if (p.y() + this->height() > screen.y() + screen.height())
		p.setY(screen.y() + screen.height() - this->height());
	
	this->move(p);
}

void RosterToolTip::setTipRect(QWidget *AWidget, const QRect &ARect)
{
	FRect = ARect;
	FWidget = AWidget;
}

bool RosterToolTip::isTipChanged(const QPoint &APos, const QString &AText, QObject *AObject)
{
	if (ui.lblToolTip->text() != AText)
		return true;

	if (AObject != FWidget)
		return true;

	if (!FRect.isNull())
		return !FRect.contains(APos);

	return false;
}

int RosterToolTip::getTipScreen(const QPoint &APos, QWidget *AWidget)
{
	if (QApplication::desktop()->isVirtualDesktop())
		return QApplication::desktop()->screenNumber(APos);
	else
		return QApplication::desktop()->screenNumber(AWidget);
}

void RosterToolTip::paintEvent(QPaintEvent *AEvent)
{
	QStyleOptionFrame option;
	option.init(this);

	QStylePainter painter(this);
	painter.drawPrimitive(QStyle::PE_PanelTipLabel, option);
	painter.end();

	QFrame::paintEvent(AEvent);
}

void RosterToolTip::resizeEvent(QResizeEvent *AEvent)
{
	QStyleOption option;
	option.init(this);

	QStyleHintReturnMask frameMask;
	if (style()->styleHint(QStyle::SH_ToolTip_Mask, &option, this, &frameMask))
		setMask(frameMask.region);

	QFrame::resizeEvent(AEvent);
}

void RosterToolTip::enterEvent(QEvent *AEvent)
{
	Q_UNUSED(AEvent);
	FMouseOver = true;
	FHideTimer.stop();
	FExpireTimer.stop();
}

void RosterToolTip::leaveEvent(QEvent *AEvent)
{
	Q_UNUSED(AEvent);
	FMouseOver = false;
	restartExpireTimer();
}

void RosterToolTip::timerEvent(QTimerEvent *AEvent)
{
	if (AEvent->timerId() == FHideTimer.timerId() || AEvent->timerId() == FExpireTimer.timerId())
	{
		FHideTimer.stop();
		FExpireTimer.stop();
		hideTipImmediately();
	}
}

void RosterToolTip::mouseMoveEvent(QMouseEvent *AEvent)
{
	if (!FRect.isNull())
	{
		QPoint pos = FWidget!=NULL ? FWidget->mapFromGlobal(AEvent->globalPos()) : AEvent->globalPos();
		if (!FRect.contains(pos))
			hideTip();
	}
	QFrame::mouseMoveEvent(AEvent);
}

bool RosterToolTip::eventFilter(QObject *AWatch, QEvent *AEvent)
{
	switch (AEvent->type()) 
	{
#ifdef Q_WS_MAC
		case QEvent::KeyPress:
		case QEvent::KeyRelease: 
			{
				int key = static_cast<QKeyEvent *>(AEvent)->key();
				Qt::KeyboardModifiers mody = static_cast<QKeyEvent *>(AEvent)->modifiers();
				if (!(mody & Qt::KeyboardModifierMask) && key!=Qt::Key_Shift && key!=Qt::Key_Control && key!=Qt::Key_Alt && key!=Qt::Key_Meta)
					hideTip();
				break;
			}
#endif
		case QEvent::Enter:
		case QEvent::Leave:
			hideTip();
			break;
		case QEvent::MouseButtonRelease:
		case QEvent::MouseButtonDblClick:
			hideTipImmediately();
			break;
		case QEvent::WindowActivate:
		case QEvent::WindowDeactivate:
		case QEvent::MouseButtonPress:
		case QEvent::FocusIn:
		case QEvent::FocusOut:
		case QEvent::Wheel:
			if (!FMouseOver)
				hideTipImmediately();
			break;
		case QEvent::MouseMove:
			if (AWatch == FWidget && !FRect.isNull() && !FRect.contains(static_cast<QMouseEvent*>(AEvent)->pos()))
				hideTip();
		default:
			break;
	}
	return false;
}
