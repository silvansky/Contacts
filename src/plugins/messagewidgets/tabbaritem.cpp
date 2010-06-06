#include "tabbaritem.h"

#include <QPaintEvent>
#include <QHBoxLayout>
#include <QStylePainter>

TabBarItem::TabBarItem(QWidget *AParent) : QFrame(AParent)
{
	FIconSize = QSize(15,15);

	setFrameShadow(QFrame::Plain);
	setFrameShape(QFrame::StyledPanel);

	setLayout(new QHBoxLayout);
	layout()->setMargin(2);
	layout()->setSpacing(2);

	layout()->addWidget(FIcon = new QLabel(this));
	layout()->addWidget(FLabel = new QLabel(this));
	layout()->addWidget(FClose = new CloseButton(this));

	FIcon->setFixedSize(FIconSize);
	FLabel->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Preferred);
	connect(FClose,SIGNAL(clicked()),SIGNAL(closeButtonClicked()));

	setActive(false);
	setState(TPS_NORMAL);
}

TabBarItem::~TabBarItem()
{

}

int TabBarItem::state() const
{
	return FState;
}

void TabBarItem::setState(int AState)
{
	FState = AState;
	update();
}

bool TabBarItem::active() const
{
	return FActive;
}

void TabBarItem::setActive(bool AActive)
{
	FActive = AActive;
	update();
}

QSize TabBarItem::iconSize() const
{
	return FIconSize;
}

void TabBarItem::setIconSize(const QSize &ASize)
{
	FIcon->setFixedSize(ASize);
	FIconSize = ASize;
}

QIcon TabBarItem::icon() const
{
	QIcon icon;
	icon.addPixmap(*FIcon->pixmap());
	return icon;
}

void TabBarItem::setIcon(const QIcon &AIcon)
{
	setIconKey(QString::null);
	FIcon->setPixmap(AIcon.pixmap(FIconSize));
}

QString TabBarItem::iconKey() const
{
	return FIconKey;
}

void TabBarItem::setIconKey(const QString &AIconKey)
{
	if (!AIconKey.isEmpty())
	{
		IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->insertAutoIcon(FIcon,AIconKey,0,0,"pixmap");
	}
	else
	{
		IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->removeAutoIcon(FIcon);
		FIcon->clear();
	}
	FIconKey = AIconKey;
}

QString TabBarItem::text() const
{
	return FLabel->text();
}

void TabBarItem::setText(const QString &AText)
{
	FLabel->setText(AText);
}

bool TabBarItem::isCloseable() const
{
	return FClose->isVisible();
}

void TabBarItem::setCloseable(bool ACloseable)
{
	FClose->setVisible(ACloseable);
	update();
}

void TabBarItem::enterEvent(QEvent *AEvent)
{
	QFrame::enterEvent(AEvent);
	update();
}

void TabBarItem::leaveEvent(QEvent *AEvent)
{
	QFrame::leaveEvent(AEvent);
	update();
}

void TabBarItem::paintEvent(QPaintEvent *AEvent)
{
	QStyleOptionTabV3 option;
	initStyleOption(&option);
	QStylePainter p(this);
	p.drawControl(QStyle::CE_TabBarTab, option);
	QFrame::paintEvent(AEvent);
}

void TabBarItem::initStyleOption(QStyleOptionTabV3 *AOption)
{
	AOption->initFrom(this);
	AOption->state &= ~(QStyle::State_HasFocus | QStyle::State_MouseOver);
	AOption->rect = QRect(0,0,width(),height()+1);
	AOption->row = 0;

	if (active())
		AOption->state |= QStyle::State_Selected;
	if (active() && hasFocus())
		AOption->state |= QStyle::State_HasFocus;
	if (!isEnabled())
		AOption->state &= ~QStyle::State_Enabled;
	if (isActiveWindow())
		AOption->state |= QStyle::State_Active;
	if (underMouse())
		AOption->state |= QStyle::State_MouseOver;
	AOption->shape = QTabBar::RoundedSouth;

	AOption->iconSize = FIconSize;  // Will get the default value then.
	AOption->leftButtonSize = FIcon->size();
	AOption->rightButtonSize = FClose->size();
	AOption->documentMode = true;
	AOption->selectedPosition = QStyleOptionTab::NotAdjacent;
	AOption->position = QStyleOptionTab::Middle;
}
