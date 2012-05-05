#include "videolayout.h"

#include <QtDebug>

#include <QStyle>
#include <utils/options.h>
#include <utils/widgetmanager.h>

VideoLayout::VideoLayout(VideoFrame *ARemoteVideo, VideoFrame *ALocalVideo, QWidget *AParent) : QLayout(AParent)
{
	FLocalMargin = 4;
	FLocalStickDelta = 9;

	FControlls = NULL;
	FRemoteVideo = ARemoteVideo;
	FLocalVideo = ALocalVideo;

	connect(FLocalVideo,SIGNAL(doubleClicked()),SLOT(onLocalVideoDoubleClicked()));
	connect(FLocalVideo,SIGNAL(moveTo(const QPoint &)),SLOT(onLocalVideoMove(const QPoint &)));
	connect(FLocalVideo,SIGNAL(resizeTo(Qt::Corner, const QPoint &)),SLOT(onLocalVideoResize(Qt::Corner, const QPoint &)));
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
	return FRemoteVideo->sizeHint();
}

void VideoLayout::setGeometry(const QRect &ARect)
{
	QRect oldRect = geometry();
	QLayout::setGeometry(ARect);

	QSize remoteSize = FRemoteVideo->sizeHint();
	remoteSize.scale(ARect.size(),Qt::KeepAspectRatio);
	FRemoteVideo->setGeometry(QStyle::alignedRect(Qt::LeftToRight,remoteVideoAlignment(),remoteSize,ARect));

	if (!FLocalVideo->isCollapsed())
	{
		QRect localRect = adjustLocalVideoSize(FLocalVideo->geometry());
		localRect =	adjustLocalVideoPosition(localRect);
		FLocalVideo->setGeometry(localRect);
		FLocalVideo->setMaximumVideoSize(ARect.size()/2);
		FLocalVideo->setAlignment(geometryAlignment(localRect));
	}
	else
	{
		QRect availRect = ARect.adjusted(FLocalMargin,FLocalMargin,-FLocalMargin,-FLocalMargin);
		FLocalVideo->setGeometry(QStyle::alignedRect(Qt::LeftToRight,Qt::AlignRight|Qt::AlignBottom,FLocalVideo->sizeHint(),availRect));
	}
}

int VideoLayout::locaVideoMargin() const
{
	return FLocalMargin;
}

void VideoLayout::setLocalVideoMargin(int AMargin)
{
	if (0<=AMargin && AMargin<=FLocalStickDelta)
		FLocalMargin = AMargin;
}

void VideoLayout::saveLocalVideoGeometry()
{
	if (!FLocalScale.isNull())
	{
		Options::setFileValue(FLocalScale,"sipphone.videocallwindow.videolayout.localvideo.scale");
		Options::setFileValue((int)FLocalVideo->alignment(),"sipphone.videocallwindow.videolayout.localvideo.alignment");
	}
}

void VideoLayout::restoreLocalVideoGeometry()
{
	FLocalScale = Options::fileValue("sipphone.videocallwindow.videolayout.localvideo.scale").toRectF();
	if (FLocalScale.isNull())
	{
		FLocalScale = QRectF(0.75,0.75,0.25,0.25);
		FLocalVideo->setAlignment(Qt::AlignRight|Qt::AlignBottom);
	}
	else
	{
		FLocalVideo->setAlignment((Qt::Alignment)Options::fileValue("sipphone.videocallwindow.videolayout.localvideo.alignment").toInt());
	}
	update();
}

void VideoLayout::saveLocalVideoGeometryScale()
{
	QRect availRect = geometry();
	if (!FLocalVideo->isCollapsed() && availRect.width()>0 && availRect.height()>0)
	{
		QRect localRect = FLocalVideo->geometry();
		FLocalScale.setLeft((qreal)localRect.left()/availRect.width());
		FLocalScale.setRight((qreal)localRect.right()/availRect.width());
		FLocalScale.setTop((qreal)localRect.top()/availRect.height());
		FLocalScale.setBottom((qreal)localRect.bottom()/availRect.height());
	}
}

Qt::Alignment VideoLayout::remoteVideoAlignment() const
{
	if (!FRemoteVideo->isEmpty() && !FLocalVideo->isCollapsed())
	{
		Qt::Alignment remoteAlign = 0;
		Qt::Alignment localAlign = FLocalVideo->alignment();

		if (localAlign & Qt::AlignLeft)
			remoteAlign |= Qt::AlignRight;
		else if (localAlign & Qt::AlignRight)
			remoteAlign |= Qt::AlignLeft;
		else
			remoteAlign |= Qt::AlignHCenter;

		if (localAlign & Qt::AlignTop)
			remoteAlign |= Qt::AlignBottom;
		else if (localAlign & Qt::AlignBottom)
			remoteAlign |= Qt::AlignTop;
		else
			remoteAlign |= Qt::AlignVCenter;

		return remoteAlign;
	}
	return Qt::AlignCenter;
}

Qt::Alignment VideoLayout::geometryAlignment(const QRect &AGeometry) const
{
	Qt::Alignment align = 0;
	QRect availRect = geometry();
	if (!availRect.isEmpty())
	{
		int leftDelta = AGeometry.left() - availRect.left();
		int topDelta = AGeometry.top() - availRect.top();
		int rightDelta = availRect.right() - AGeometry.right();
		int bottomDelta = availRect.bottom() - AGeometry.bottom();

		if (leftDelta < FLocalStickDelta)
			align |= Qt::AlignLeft;
		else if (rightDelta < FLocalStickDelta)
			align |= Qt::AlignRight;
		if (topDelta < FLocalStickDelta)
			align |= Qt::AlignTop;
		else if (bottomDelta < FLocalStickDelta)
			align |= Qt::AlignBottom;
	}
	return align;
}

QRect VideoLayout::adjustLocalVideoSize(const QRect &AGeometry) const
{
	QRect newGeometry = AGeometry;
	if (!FLocalScale.isNull())
	{
		newGeometry.setLeft(qRound(geometry().width()*FLocalScale.left()));
		newGeometry.setRight(qRound(geometry().width()*FLocalScale.right()));
		newGeometry.setTop(qRound(geometry().height()*FLocalScale.top()));
		newGeometry.setBottom(qRound(geometry().height()*FLocalScale.bottom()));

		if (newGeometry.width() < FLocalVideo->minimumVideoSize().width())
		{
			int delta = FLocalVideo->minimumVideoSize().width() - newGeometry.width();
			newGeometry.setLeft(newGeometry.left() - delta/2);
			newGeometry.setRight(newGeometry.right() + delta - delta/2);
		}
		else if (newGeometry.width() > FLocalVideo->maximumVideoSize().width())
		{
			int delta = newGeometry.width() - FLocalVideo->maximumVideoSize().width();
			newGeometry.setLeft(newGeometry.left() + delta/2);
			newGeometry.setRight(newGeometry.right() - delta + delta/2);
		}
		if (newGeometry.height() < FLocalVideo->minimumVideoSize().height())
		{
			int delta = FLocalVideo->minimumVideoSize().height() - newGeometry.height();
			newGeometry.setTop(newGeometry.top() - delta/2);
			newGeometry.setBottom(newGeometry.bottom() + delta - delta/2);
		}
		else if (newGeometry.height() > FLocalVideo->maximumVideoSize().height())
		{
			int delta = newGeometry.height() - FLocalVideo->maximumVideoSize().height();
			newGeometry.setTop(newGeometry.top() + delta/2);
			newGeometry.setBottom(newGeometry.bottom() - delta + delta/2);
		}

		if (!FLocalVideo->isEmpty() && !FLocalVideo->isCollapsed())
		{
			QSize videoSize = FLocalVideo->sizeHint();
			int newWidth = qRound(newGeometry.height()*((qreal)videoSize.width()/videoSize.height()));
			int newHeight = qRound(newGeometry.width()*((qreal)videoSize.height()/videoSize.width()));
			if (newGeometry.width() > newWidth)
				newGeometry.setWidth(newWidth);
			else if (newGeometry.height() > newHeight)
				newGeometry.setHeight(newHeight);
		}
	}
	return newGeometry;
}

QRect VideoLayout::adjustLocalVideoPosition(const QRect &AGeometry) const
{
	QRect availRect = geometry();
	if (!availRect.isEmpty())
	{
		availRect.adjust(FLocalMargin,FLocalMargin,-FLocalMargin,-FLocalMargin);
		return WidgetManager::alignRect(AGeometry,availRect,FLocalVideo->alignment());
	}
	return AGeometry;
}

QRect VideoLayout::correctLocalVideoPosition(const QRect &AGeometry) const
{
	QRect availRect = geometry();
	int leftDelta = AGeometry.left() - availRect.left();
	int topDelta = AGeometry.top() - availRect.top();
	int rightDelta = availRect.right() - AGeometry.right();
	int bottomDelta = availRect.bottom() - AGeometry.bottom();

	QPoint newTopLeft = AGeometry.topLeft();
	if (leftDelta < FLocalStickDelta)
		newTopLeft.rx() += FLocalMargin-leftDelta;
	if (topDelta < FLocalStickDelta)
		newTopLeft.ry() += FLocalMargin-topDelta;
	if (rightDelta < FLocalStickDelta)
		newTopLeft.rx() -= FLocalMargin-rightDelta;
	if (bottomDelta < FLocalStickDelta)
		newTopLeft.ry() -= FLocalMargin-bottomDelta;

	QRect newGeometry = AGeometry;
	newGeometry.moveTo(newTopLeft);

	return newGeometry;
}

QRect VideoLayout::correctLocalVideoSize(Qt::Corner ACorner, const QRect &AGeometry) const
{
	QRect availRect = geometry();
	int leftDelta = AGeometry.left() - availRect.left();
	int topDelta = AGeometry.top() - availRect.top();
	int rightDelta = availRect.right() - AGeometry.right();
	int bottomDelta = availRect.bottom() - AGeometry.bottom();

	QRect newGeometry = AGeometry;
	if (leftDelta<FLocalStickDelta && (ACorner==Qt::TopLeftCorner || ACorner==Qt::BottomLeftCorner))
		newGeometry.setLeft(newGeometry.left()+FLocalMargin-leftDelta);
	if (topDelta < FLocalStickDelta && (ACorner==Qt::TopLeftCorner || ACorner==Qt::TopRightCorner))
		newGeometry.setTop(newGeometry.top()+FLocalMargin-topDelta);
	if (rightDelta<FLocalStickDelta && (ACorner==Qt::TopRightCorner || ACorner==Qt::BottomRightCorner))
		newGeometry.setRight(newGeometry.right()-FLocalMargin+rightDelta);
	if (bottomDelta<FLocalStickDelta && (ACorner==Qt::BottomLeftCorner || ACorner==Qt::BottomRightCorner))
		newGeometry.setBottom(newGeometry.bottom()-FLocalMargin+bottomDelta);

	if (newGeometry.width() < FLocalVideo->minimumVideoSize().width())
	{
		if (ACorner==Qt::TopLeftCorner || ACorner==Qt::BottomLeftCorner)
			newGeometry.setLeft(newGeometry.left()-(FLocalVideo->minimumVideoSize().width()-newGeometry.width()));
		else if (ACorner==Qt::TopRightCorner || ACorner==Qt::BottomRightCorner)
			newGeometry.setRight(newGeometry.right()+(FLocalVideo->minimumVideoSize().width()-newGeometry.width()));
	}
	else if (newGeometry.width() > FLocalVideo->maximumVideoSize().width())
	{
		if (ACorner==Qt::TopLeftCorner || ACorner==Qt::BottomLeftCorner)
			newGeometry.setLeft(newGeometry.left()+(newGeometry.width()-FLocalVideo->maximumVideoSize().width()));
		else if (ACorner==Qt::TopRightCorner || ACorner==Qt::BottomRightCorner)
			newGeometry.setRight(newGeometry.right()-(newGeometry.width()-FLocalVideo->maximumVideoSize().width()));
	}

	if (newGeometry.height() < FLocalVideo->minimumVideoSize().height())
	{
		if (ACorner==Qt::TopLeftCorner || ACorner==Qt::TopRightCorner)
			newGeometry.setTop(newGeometry.top()-(FLocalVideo->minimumVideoSize().height()-newGeometry.height()));
		else if (ACorner==Qt::BottomLeftCorner || ACorner==Qt::BottomRightCorner)
			newGeometry.setBottom(newGeometry.bottom()+(FLocalVideo->minimumVideoSize().height()-newGeometry.height()));
	}
	else if (newGeometry.height() > FLocalVideo->maximumVideoSize().height())
	{
		if (ACorner==Qt::TopLeftCorner || ACorner==Qt::TopRightCorner)
			newGeometry.setTop(newGeometry.top()+(newGeometry.height()-FLocalVideo->maximumVideoSize().height()));
		else if (ACorner==Qt::BottomLeftCorner || ACorner==Qt::BottomRightCorner)
			newGeometry.setBottom(newGeometry.bottom()-(newGeometry.height()-FLocalVideo->maximumVideoSize().height()));
	}

	if (!FLocalVideo->isEmpty() && !FLocalVideo->isCollapsed())
	{
		QSize videoSize = FLocalVideo->sizeHint();
		int newWidth = qRound(newGeometry.height()*((qreal)videoSize.width()/videoSize.height()));
		int newHeight = qRound(newGeometry.width()*((qreal)videoSize.height()/videoSize.width()));
		if (newGeometry.width() > newWidth)
		{
			if (ACorner==Qt::TopLeftCorner || ACorner==Qt::BottomLeftCorner)
				newGeometry.setLeft(newGeometry.left()+(newGeometry.width()-newWidth));
			else if (ACorner==Qt::TopRightCorner || ACorner==Qt::BottomRightCorner)
				newGeometry.setRight(newGeometry.right()-(newGeometry.left()+newGeometry.width()-newWidth));
		}
		else if (newGeometry.height() > newHeight)
		{
			if (ACorner==Qt::TopLeftCorner || ACorner==Qt::TopRightCorner)
				newGeometry.setTop(newGeometry.top()+(newGeometry.height()-newHeight));
			else if (ACorner==Qt::BottomLeftCorner || ACorner==Qt::BottomRightCorner)
				newGeometry.setBottom(newGeometry.bottom()-(newGeometry.height()-newHeight));
		}
	}

	return newGeometry;
}

void VideoLayout::onLocalVideoDoubleClicked()
{
	if (!FLocalVideo->isCollapsed())
		saveLocalVideoGeometry();
	else
		restoreLocalVideoGeometry();
	FLocalVideo->setCollapsed(!FLocalVideo->isCollapsed());
}

void VideoLayout::onLocalVideoMove(const QPoint &APos)
{
	if (!geometry().isEmpty())
	{
		QRect newGeometry = FLocalVideo->geometry();
		newGeometry.moveTo(APos);
		newGeometry = correctLocalVideoPosition(newGeometry);

		FLocalVideo->setGeometry(newGeometry);
		FLocalVideo->setAlignment(geometryAlignment(newGeometry));

		saveLocalVideoGeometryScale();
		update();
	}
}

void VideoLayout::onLocalVideoResize(Qt::Corner ACorner, const QPoint &APos)
{
	QRect availRect = geometry();
	if (!availRect.isEmpty())
	{
		QRect newGeometry = FLocalVideo->geometry();
		if (ACorner == Qt::TopLeftCorner)
			newGeometry.setTopLeft(APos);
		else if (ACorner == Qt::TopRightCorner)
			newGeometry.setTopRight(APos);
		else if (ACorner == Qt::BottomLeftCorner)
			newGeometry.setBottomLeft(APos);
		else if (ACorner == Qt::BottomRightCorner)
			newGeometry.setBottomRight(APos);
		newGeometry = correctLocalVideoSize(ACorner,newGeometry);

		FLocalVideo->setGeometry(newGeometry);
		FLocalVideo->setAlignment(geometryAlignment(newGeometry));

		saveLocalVideoGeometryScale();
		update();
	}
}
