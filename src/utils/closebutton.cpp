#include "closebutton.h"

#include <QStyle>
#include <QPainter>
#include <QPaintEvent>
#include <definitions/resources.h>
#include <definitions/stylesheets.h>
#include "iconstorage.h"
#include "stylestorage.h"

CloseButton::CloseButton(QWidget *AParent) : QAbstractButton(AParent)
{
	setMouseTracking(true);
	setFocusPolicy(Qt::NoFocus);
	setProperty("isHover",false);
	StyleStorage::staticStorage(RSR_STORAGE_STYLESHEETS)->insertAutoStyle(this,STS_UTILS_CLOSEBUTTON);
}

QSize CloseButton::sizeHint() const
{
	ensurePolished();
	return icon().availableSizes().value(0);
}

void CloseButton::enterEvent(QEvent *AEvent)
{
	QAbstractButton::enterEvent(AEvent);
	setProperty("isHover",true);
	StyleStorage::updateStyle(this);
}

void CloseButton::leaveEvent(QEvent *AEvent)
{
	QAbstractButton::leaveEvent(AEvent);
	setProperty("isHover",false);
	StyleStorage::updateStyle(this);
}

void CloseButton::paintEvent(QPaintEvent *AEvent)
{
	Q_UNUSED(AEvent)
	if (!icon().isNull())
	{
		QPainter p(this);
		icon().paint(&p,rect());
	}
}
