#include "closebutton.h"

#include <QStyle>
#include <QPainter>
#include <QPaintEvent>
#include "iconstorage.h"

CloseButton::CloseButton(QWidget *AParent) : QAbstractButton(AParent)
{
	setMouseTracking(true);
	setFocusPolicy(Qt::NoFocus);
}

QSize CloseButton::sizeHint() const
{
	ensurePolished();
	return icon().availableSizes().value(0);
}

void CloseButton::enterEvent(QEvent *AEvent)
{
	QAbstractButton::enterEvent(AEvent);
	style()->polish(this);
	update();
}

void CloseButton::leaveEvent(QEvent *AEvent)
{
	QAbstractButton::leaveEvent(AEvent);
	style()->polish(this);
	update();
}

void CloseButton::paintEvent(QPaintEvent *AEvent)
{
	if (!icon().isNull())
	{
		QPainter p(this);
		icon().paint(&p,AEvent->rect());
	}
}
