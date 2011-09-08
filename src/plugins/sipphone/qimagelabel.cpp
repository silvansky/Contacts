#include "qimagelabel.h"



#include <QPainter>
#include <QResizeEvent>


QImageLabel::QImageLabel(QWidget *parent) : QLabel(parent)
{
	setObjectName("QImageLabel");

	setMouseTracking(true);

	setProperty("ignoreFilter", true);

	iconStorage = IconStorage::staticStorage(RSR_STORAGE_MENUICONS);
	//icon.addFile(iconStorage->fileFullName(MNI_ROSTERSEARCH_ICON_CROSS), QSize(16,16));
	//icon.addFile(iconStorage->fileFullName(MNI_ROSTERSEARCH_ICON_CROSS_HOVER), QSize(24,24));

	//iconStorage = IconStorage::staticStorage(RSR_STORAGE_MENUICONS);
	iconLabel = new QLabel(this);
	iconLabel->setFixedSize(16, 16);
	iconLabel->setMouseTracking(true);
	iconLabel->setProperty("ignoreFilter", true);

	currentIcon = iconStorage->getIcon(MNI_ROSTERSEARCH_ICON_CROSS);
	if (!currentIcon.isNull())
		iconLabel->setPixmap(currentIcon.pixmap(16, QIcon::Normal, QIcon::On));

	//QPixmap crossPic("D:\\cross.png");
	//iconLabel->setPixmap(crossPic);
}



void QImageLabel::resizeEvent(QResizeEvent * revent)
{
	iconLabel->move(revent->size().width() - 20, 4);//(revent->size().height() - 16) / 2);
	QLabel::resizeEvent(revent);
}

void QImageLabel::mouseMoveEvent(QMouseEvent * mevent)
{
	if (iconLabel->geometry().contains(mevent->pos()))
	{
		setCursor(QCursor(Qt::PointingHandCursor));
		updateIcon(Hover);
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
}

void QImageLabel::leaveEvent(QEvent *)
{

}

void QImageLabel::paintEvent(QPaintEvent * evt)
{
	Q_UNUSED(evt);
	// Issue
	static qint64 prevCache = 0;
	//static bool blackFill = false;

	const QPixmap* px = pixmap();
	qint64 cache = 0;

	QPainter p(this);
	QRect curRect = rect();

	if(px)
	{
		p.drawPixmap(rect(), *px);
		cache = px->cacheKey();
		//blackFill = false;
	}

	if(px == NULL || px->isNull() || cache == prevCache)
	{
		//if(!blackFill)
		{
			p.fillRect(curRect, Qt::black);
			//blackFill = true;
		}

		QTextOption option(Qt::AlignCenter);
		p.drawText(curRect, tr("no image"),  option);
	}

	prevCache = cache;
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
	QLabel::setPixmap(pix);
	update(rect());
}
