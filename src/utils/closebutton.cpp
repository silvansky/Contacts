#include "closebutton.h"

#include <QStyle>
#include <QPainter>
#include <QStyleOption>
#include "iconstorage.h"
#include <definitions/resources.h>
#include <definitions/menuicons.h>

CloseButton::CloseButton(QWidget *AParent) : QAbstractButton(AParent)
{
	setFocusPolicy(Qt::NoFocus);
	setCursor(Qt::ArrowCursor);
	resize(sizeHint());
}

QSize CloseButton::sizeHint() const
{
	ensurePolished();
	int width = IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getImage(MNI_MESSAGEWIDGETS_CLOSE_TAB).width();
	int height = IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getImage(MNI_MESSAGEWIDGETS_CLOSE_TAB).height();
	return QSize(width, height);
}

void CloseButton::enterEvent(QEvent *event)
{
	if (isEnabled())
		update();
	QAbstractButton::enterEvent(event);
}

void CloseButton::leaveEvent(QEvent *event)
{
	if (isEnabled())
		update();
	QAbstractButton::leaveEvent(event);
}

void CloseButton::paintEvent(QPaintEvent *)
{
	QPainter p(this);
//	QStyleOption opt;
//	opt.init(this);
//	opt.state |= QStyle::State_AutoRaise;
//	if (isEnabled() && underMouse() && !isChecked() && !isDown())
//		opt.state |= QStyle::State_Raised;
//	if (isChecked())
//		opt.state |= QStyle::State_On;
//	if (isDown())
//		opt.state |= QStyle::State_Sunken;

//	style()->drawPrimitive(QStyle::PE_IndicatorTabClose, &opt, &p, this);
	p.drawImage(0, 0, IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getImage(MNI_MESSAGEWIDGETS_CLOSE_TAB));
}
