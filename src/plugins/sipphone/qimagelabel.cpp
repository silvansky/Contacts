#include "qimagelabel.h"



#include <QPainter>
#include <QResizeEvent>


QImageLabel::QImageLabel(QWidget *parent) : QLabel(parent)
{
	setMouseTracking(true);



	iconStorage = IconStorage::staticStorage(RSR_STORAGE_MENUICONS);
	//icon.addFile(iconStorage->fileFullName(MNI_ROSTERSEARCH_ICON_CROSS), QSize(16,16));
	//icon.addFile(iconStorage->fileFullName(MNI_ROSTERSEARCH_ICON_CROSS_HOVER), QSize(24,24));

	//iconStorage = IconStorage::staticStorage(RSR_STORAGE_MENUICONS);
	iconLabel = new QLabel(this);
	iconLabel->setFixedSize(16, 16);
	iconLabel->setMouseTracking(true);

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


void QImageLabel::updateIcon(IconCrossState iconState)
{
	if (iconStorage)
	{
		switch (iconState)
		{
		case Stable:
			currentIcon = iconStorage->getIcon(MNI_ROSTERSEARCH_ICON_CROSS);
			//{
			//	QPixmap crossPic("D:\\cross.png");
			//	iconLabel->setPixmap(crossPic);
			//}
			break;
		case Hover:
			currentIcon = iconStorage->getIcon(MNI_ROSTERSEARCH_ICON_CROSS_HOVER);
			//{
			//	QPixmap crosshoverPic("D:\\crosshover.png");
			//	iconLabel->setPixmap(crosshoverPic);
			//}
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