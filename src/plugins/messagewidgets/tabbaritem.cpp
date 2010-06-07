#include "tabbaritem.h"

#include <QPaintEvent>
#include <QHBoxLayout>
#include <QStylePainter>

TabBarItem::TabBarItem(QWidget *AParent) : QFrame(AParent)
{
	FIconSize = QSize(15,15);

	setFrameShape(QFrame::Box);
	setFrameShadow(QFrame::Sunken);

	setLayout(new QHBoxLayout);
	layout()->setMargin(2);
	layout()->setSpacing(2);

	layout()->addWidget(FIcon = new QLabel(this));
	layout()->addWidget(FLabel = new QLabel(this));
	layout()->addWidget(FClose = new CloseButton(this));

	FIcon->installEventFilter(this);
	FLabel->installEventFilter(this);
	FClose->installEventFilter(this);

	FIcon->setFixedSize(FIconSize);
	FLabel->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Preferred);
	connect(FClose,SIGNAL(clicked()),SIGNAL(closeButtonClicked()));

	setActive(false);
	setDragged(false);
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

void TabBarItem::setDragged(bool ADragged)
{
	FDragged = ADragged;
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
	if (!FDragged)
	{
		QPalette pal = palette();
		QRect rect = QRect(0,0,width(),height());

		QLinearGradient background(rect.topLeft(),rect.bottomLeft());
		if (active())
		{
			background.setColorAt(0.0,pal.color(QPalette::Base));
			background.setColorAt(1.0,pal.color(QPalette::Window));
		}
		else if (underMouse())
		{
			background.setColorAt(0.0,pal.color(QPalette::Window));
			background.setColorAt(1.0,pal.color(QPalette::Dark).lighter(120));
		}
		else
		{
			background.setColorAt(0.0,pal.color(QPalette::Window));
			background.setColorAt(1.0,pal.color(QPalette::Dark));
		}

		QStylePainter p(this);
		p.fillRect(rect,background);

		QFrame::paintEvent(AEvent);
	}
}

bool TabBarItem::eventFilter(QObject *AObject, QEvent *AEvent)
{
	if (FDragged && AEvent->type()==QEvent::Paint)
		return true;
	return QFrame::eventFilter(AObject,AEvent);
}
