#include "rostertooltip.h"
#include "ui_rostertooltip.h"
#include <QDebug>
#include <QFocusEvent>

#define TIP_SHOW_TIME 3000

RosterToolTip::RosterToolTip(QWidget *parent) :
		QWidget(parent),
		ui(new Ui::RosterToolTip)
{
	ui->setupUi(this);
	ui->userInfo->setAttribute(Qt::WA_Hover, true);
	setWindowFlags(Qt::ToolTip | Qt::Popup);
	QPalette pal = palette();
	pal.setColor(QPalette::Window, pal.base().color());
	setPalette(pal);
	ui->userInfo->installEventFilter(this);
	timer = new QTimer(this);
	timer->setInterval(TIP_SHOW_TIME);
	timer->setSingleShot(true);
	connect(timer, SIGNAL(timeout()), SLOT(onTimer()));
	rightToolBar = new QToolBar(this);
	rightToolBar->setAttribute(Qt::WA_Hover, true);
	rightToolBar->setPalette(pal);
	rightToolBarChanger = new ToolBarChanger(rightToolBar);
	rightToolBar->setOrientation(Qt::Vertical);
	rightToolBar->installEventFilter(this);
	ui->frame->layout()->addWidget(rightToolBar);
	qApp->installEventFilter(this);
	hovered = false;
}

RosterToolTip::~RosterToolTip()
{
	delete ui;
}

QWidget * RosterToolTip::instance()
{
	return this;
}

ToolBarChanger * RosterToolTip::sideBarChanger()
{
	return rightToolBarChanger;
}

QString RosterToolTip::caption() const
{
	return tipCaption;
}

void RosterToolTip::setCaption(const QString & newCaption)
{
	tipCaption = newCaption;
	ui->userInfo->setText(newCaption);
}

IRosterIndex * RosterToolTip::rosterIndex() const
{
	return index;
}

void RosterToolTip::setRosterIndex(IRosterIndex * newIndex)
{
	index = newIndex;
}

void RosterToolTip::setVisible(bool visible)
{
	QWidget::setVisible(visible);
	if (visible)
	{
		timer->start();
		adjustSize();
	}
	else
	{
		QRect oldGeometry = geometry();
		oldGeometry.setSize(QSize(0, 0));
		setGeometry(oldGeometry);
	}
}

void RosterToolTip::changeEvent(QEvent *e)
{
	QWidget::changeEvent(e);
	switch (e->type())
	{
	case QEvent::LanguageChange:
		ui->retranslateUi(this);
		break;
	default:
		break;
	}
}

bool RosterToolTip::eventFilter(QObject * obj, QEvent * event)
{
	switch(event->type())
	{
	case QEvent::HoverEnter:
	case QEvent::Enter:
		hovered = true;
	case QEvent::HoverMove:
		timer->stop();
		break;
	case QEvent::HoverLeave:
	case QEvent::Leave:
		hovered = false;
		hideTip();
		break;
	case QEvent::MouseButtonRelease:
		if (rightToolBarChanger->childWidgets().contains((QWidget*)obj))
		{
			hovered = false;
			break;
		}
	case QEvent::WindowActivate:
	case QEvent::WindowDeactivate:
	case QEvent::MouseButtonPress:
	case QEvent::MouseButtonDblClick:
	case QEvent::FocusIn:
	case QEvent::FocusOut:
	case QEvent::Wheel:
		if (!hovered)
			hideTipImmediately();
		break;
	default:
		break;
	}
	return QWidget::eventFilter(obj, event);
}

void RosterToolTip::hideTipImmediately()
{
	hide();
}

void RosterToolTip::hideTip()
{
	timer->start();
}

void RosterToolTip::onTimer()
{
	hide();
}
