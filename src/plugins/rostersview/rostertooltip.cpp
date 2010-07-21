#include "rostertooltip.h"
#include "ui_rostertooltip.h"

RosterToolTip::RosterToolTip(QWidget *parent) :
		QWidget(parent),
		ui(new Ui::RosterToolTip)
{
	ui->setupUi(this);
	rightToolBar = new QToolBar(this);
	rightToolBarChanger = new ToolBarChanger(rightToolBar);
	rightToolBar->setOrientation(Qt::Vertical);
	setWindowFlags(Qt::ToolTip | Qt::Popup);
	QPalette pal = palette();
	pal.setColor(QPalette::Window, pal.base().color());
	setPalette(pal);
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
