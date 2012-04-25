#include "videolayout.h"

#include <QStyle>

VideoLayout::VideoLayout(QWidget *ARemote, QWidget *ALocal, QWidget *AParent) : QLayout(AParent)
{
	FRemote = ARemote;
	FLocal = ALocal;
	FControlls = NULL;
}

VideoLayout::~VideoLayout()
{

}

int VideoLayout::count() const
{
	return 0;
}

void VideoLayout::addItem(QLayoutItem *AItem)
{
	Q_UNUSED(AItem);
}

QLayoutItem *VideoLayout::itemAt(int AIndex) const
{
	Q_UNUSED(AIndex);
	return NULL;
}

QLayoutItem *VideoLayout::takeAt(int AIndex)
{
	Q_UNUSED(AIndex);
	return NULL;
}

QSize VideoLayout::sizeHint() const
{
	return FRemote->sizeHint();
}

void VideoLayout::setGeometry(const QRect &ARect)
{
	QLayout::setGeometry(ARect);
	FRemote->setGeometry(ARect);
	FLocal->setGeometry(QStyle::alignedRect(Qt::LeftToRight,Qt::AlignBottom|Qt::AlignRight,ARect.size()/4,ARect));
}
