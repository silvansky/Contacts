#include "qimagelabel.h"



#include <QPainter>
#include <QResizeEvent>

int QImageLabel::spacing = 4;

QImageLabel::QImageLabel(QWidget *parent) : QLabel(parent)
{
	setMouseTracking(true);
	setObjectName("lblLocalImage");
	setProperty("ignoreFilter", true);

	iconLabel = new QLabel(this);
	iconLabel->setFixedSize(16, 16);
	iconLabel->setMouseTracking(true);
	iconLabel->setProperty("ignoreFilter", true);
	iconStorage = IconStorage::staticStorage(RSR_STORAGE_MENUICONS);

	currentIcon = iconStorage->getIcon(MNI_ROSTERSEARCH_ICON_CROSS);
	if (!currentIcon.isNull())
		iconLabel->setPixmap(currentIcon.pixmap(16, QIcon::Normal, QIcon::On));
}


QPoint QImageLabel::correctTopLeftPos( const QPoint &APos ) const
{
	QPoint newPos = APos;
	QRect parentRect = parentWidget()->rect();
	if (newPos.x()+width()+spacing > parentRect.right())
		newPos.setX(parentRect.right()-width()-spacing);
	if (newPos.x() < spacing)
		newPos.setX(spacing);
	if (newPos.y() < spacing)
		newPos.setY(spacing);
	if (newPos.y()+height()+spacing > parentRect.bottom())
		newPos.setY(parentRect.bottom()-height()-spacing);
	return newPos;
}


void QImageLabel::resizeEvent(QResizeEvent *revent)
{
	iconLabel->move(revent->size().width() - 20, 4);
	QLabel::resizeEvent(revent);
}

void QImageLabel::mouseMoveEvent(QMouseEvent * mevent)
{
	if (iconLabel->geometry().contains(mevent->pos()))
	{
		setCursor(QCursor(Qt::PointingHandCursor));
		updateIcon(Hover);
	}
	else if (!pressedPos.isNull())
	{
		QPoint newPos = mapToParent(mevent->pos())-pressedPos;
		emit moveTo(correctTopLeftPos(newPos));
	}
	else
	{
		setCursor(QCursor(Qt::ArrowCursor));
		updateIcon(Stable);
	}
	QLabel::mouseMoveEvent(mevent);
}

void QImageLabel::mousePressEvent(QMouseEvent *pevent)
{
	if (iconLabel->geometry().contains(pevent->pos()))
	{
		hide();
	}
	else if (pevent->button() == Qt::LeftButton)
	{
		pressedPos = mapToParent(pevent->pos()) - geometry().topLeft();
	}
	else
	{
		QLabel::mousePressEvent(pevent);
	}
}

void QImageLabel::mouseReleaseEvent(QMouseEvent *revent)
{
	if (!pressedPos.isNull())
		pressedPos = QPoint();
	else
		QLabel::mouseReleaseEvent(revent);
}

void QImageLabel::leaveEvent(QEvent *)
{

}

void QImageLabel::paintEvent(QPaintEvent *evt)
{
	Q_UNUSED(evt);

	QPainter p(this);
	p.setPen(Qt::white);

	QRect borderRect = rect().adjusted(0,0,-1,-1);
	QRect imageRect = borderRect.adjusted(1,1,-1,-1);

	const QPixmap *px = pixmap();
	if(px && !px->isNull())
	{
		p.drawRect(borderRect);
		p.drawPixmap(imageRect, *px);
	}
	else
	{
		p.setBrush(Qt::black);
		p.drawRect(borderRect);

		QTextOption option(Qt::AlignCenter);
		p.drawText(imageRect, tr("no image"),  option);
	}
}


void QImageLabel::updateIcon(IconCrossState iconState)
{
	if (iconStorage)
	{
		switch (iconState)
		{
		case Stable:
			currentIcon = iconStorage->getIcon(MNI_ROSTERSEARCH_ICON_CROSS,0);
			break;
		case Hover:
			currentIcon = iconStorage->getIcon(MNI_ROSTERSEARCH_ICON_CROSS,0);
			break;
		}
		if (!currentIcon.isNull())
			iconLabel->setPixmap(currentIcon.pixmap(16, QIcon::Normal, QIcon::On));
	}
}

void QImageLabel::setVisible(bool state)
{
	QLabel::setVisible(state);
	emit visibleState(state);
}

void QImageLabel::setPixmap(const QPixmap &pix)
{
	if (!pix.isNull() && (pixmap()==NULL || pix.cacheKey()!=pixmap()->cacheKey()))
		QLabel::setPixmap(pix);
	else
		QLabel::clear();
	update();
}

