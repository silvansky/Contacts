#include "videolabel.h"

#include <QStyle>
#include <QPainter>
#include <QVariant>
#include <QMouseEvent>
#include <definitions/resources.h>
#include <definitions/menuicons.h>
#include <utils/iconstorage.h>

static const struct { 
	Qt::Corner corner;
	qreal rotateAngel;
	Qt::Alignment align;
	Qt::CursorShape cursor;
} Corners[] = {
	{ Qt::TopLeftCorner,     0.0,   Qt::AlignLeft|Qt::AlignTop,     Qt::SizeFDiagCursor },
	{ Qt::TopRightCorner,    90.0,  Qt::AlignRight|Qt::AlignTop,    Qt::SizeBDiagCursor },
	{ Qt::BottomLeftCorner,  270.0, Qt::AlignLeft|Qt::AlignBottom,  Qt::SizeBDiagCursor }, 
	{ Qt::BottomRightCorner, 180.0, Qt::AlignRight|Qt::AlignBottom, Qt::SizeFDiagCursor }
};

VideoFrame::VideoFrame(QWidget *AParent) : QFrame(AParent)
{
	setMouseTracking(true);

	FCursorCorner = -1;
	FCollapsed = false;
	FMoveEnabled = false;
	FResizeEnabled = false;
	FMinimumSize = QSize(50,50);
	FMaximumSize = QSize(QWIDGETSIZE_MAX,QWIDGETSIZE_MAX);
	FAlignment = Qt::AlignRight|Qt::AlignBottom;

	QIcon icon = IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getIcon(MNI_SIPPHONE_VIDEO_RESIZE);
	FResizeIcon = icon.pixmap(icon.availableSizes().value(0));

	icon = IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getIcon(MNI_SIPPHONE_VIDEO_COLLAPSED);
	FCollapsedIcon = icon.pixmap(icon.availableSizes().value(0));

	FWaitMovie = new QMovie(this);
	FWaitMovie->setFileName(IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->fileFullName(MNI_SIPPHONE_VIDEO_WAIT));
	connect(FWaitMovie,SIGNAL(frameChanged(int)),SLOT(onWaitMovieFrameChanged(int)));
	FWaitMovie->start();
}

VideoFrame::~VideoFrame()
{

}

bool VideoFrame::isEmpty() const
{
	return FVideoFrame.isNull();
}

bool VideoFrame::isCollapsed() const
{
	return FCollapsed;
}

void VideoFrame::setCollapsed(bool ACollapsed)
{
	if (FCollapsed != ACollapsed)
	{
		FCollapsed = ACollapsed;
		update();
	}
}

bool VideoFrame::isMoveEnabled() const
{
	return FMoveEnabled;
}

void VideoFrame::setMoveEnabled(bool AEnabled)
{
	if (FMoveEnabled != AEnabled)
	{
		FMoveEnabled = AEnabled;
		setProperty("ignoreFilter", AEnabled);
	}
}

bool VideoFrame::isResizeEnabled() const
{
	return FResizeEnabled;
}

void VideoFrame::setResizeEnabled(bool AEnabled)
{
	if (FResizeEnabled != AEnabled)
	{
		FResizeEnabled = AEnabled;
		update();
	}
}

Qt::Alignment VideoFrame::alignment() const
{
	return FAlignment;
}

void VideoFrame::setAlignment(Qt::Alignment AAlign)
{
	if (FAlignment != AAlign)
	{
		FAlignment = AAlign;
		update();
	}
}

QSize VideoFrame::minimumVideoSize() const
{
	return FMinimumSize;
}

void VideoFrame::setMinimumVideoSize(const QSize &ASize)
{
	FMinimumSize = ASize;
}

QSize VideoFrame::maximumVideoSize() const
{
	return FMaximumSize;
}

void VideoFrame::setMaximumVideoSize(const QSize &ASize)
{
	FMaximumSize = ASize;
}

const QPixmap *VideoFrame::pixmap() const
{
	return &FVideoFrame;
}

void VideoFrame::setPixmap(const QPixmap &APixmap)
{
	if (FVideoFrame.cacheKey() != APixmap.cacheKey())
	{
		if (FVideoFrame.isNull() != APixmap.isNull())
		{
			if (APixmap.isNull())
				FWaitMovie->start();
			else
				FWaitMovie->stop();
		}
		if (FVideoFrame.size() != APixmap.size())
		{
			FVideoFrame = APixmap;
			updateGeometry();
		}
		else
		{
			FVideoFrame = APixmap;
		}
		update();
	}
}

QSize VideoFrame::sizeHint() const
{
	if (FCollapsed)
		return !FCollapsedIcon.isNull() ? FCollapsedIcon.size() : QSize(15,15);
	return !isEmpty() ? pixmap()->size() : FMinimumSize;
}

QSize VideoFrame::minimumSizeHint() const
{
	return FMinimumSize;
}

void VideoFrame::enterEvent(QEvent *AEvent)
{
	if (FResizeEnabled)
		update();
	QFrame::enterEvent(AEvent);
}

void VideoFrame::leaveEvent(QEvent *AEvent)
{
	if (FResizeEnabled)
		update();
	FCursorCorner = -1;
	setCursor(Qt::ArrowCursor);
	QFrame::leaveEvent(AEvent);
}

void VideoFrame::mouseMoveEvent(QMouseEvent *AEvent)
{
	static const QSize cornerSize = QSize(10,10);

	if (FPressedPos.isNull())
	{
		FCursorCorner = -1;
		for (int i=0; FCursorCorner<0 && i<4; i++)
			if ((Corners[i].align & FAlignment)==0 && QStyle::alignedRect(Qt::LeftToRight,Corners[i].align,cornerSize,rect()).contains(AEvent->pos()))
				FCursorCorner = i;

		if (FResizeEnabled && FCursorCorner>=0)
			setCursor(Corners[FCursorCorner].cursor);
		else if (FMoveEnabled)
			setCursor(Qt::OpenHandCursor);
	}
	else if (FCursorCorner >= 0)
	{
		emit resizeTo((Qt::Corner)FCursorCorner,mapToParent(AEvent->pos()));
	}
	else
	{
		emit moveTo(mapToParent(AEvent->pos())-FPressedPos);
	}

	QFrame::mouseMoveEvent(AEvent);
}

void VideoFrame::mousePressEvent(QMouseEvent *AEvent)
{
	if (FResizeEnabled && FCursorCorner>=0)
	{
		FPressedPos = AEvent->pos();
	}
	else if (FMoveEnabled)
	{
		FPressedPos = mapToParent(AEvent->pos()) - geometry().topLeft();
		setCursor(Qt::ClosedHandCursor);
	}
	else
	{
		QFrame::mousePressEvent(AEvent);
	}
}

void VideoFrame::mouseReleaseEvent(QMouseEvent *AEvent)
{
	if (!FPressedPos.isNull())
	{
		if (FMoveEnabled && FCursorCorner<0)
			setCursor(Qt::OpenHandCursor);
		FPressedPos = QPoint();
	}
	else
	{
		QFrame::mousePressEvent(AEvent);
	}
}

void VideoFrame::paintEvent(QPaintEvent *AEvent)
{
	Q_UNUSED(AEvent);
	QPainter p(this);
	p.fillRect(rect(),Qt::black);
	
	if (!isEmpty())
	{
		const QPixmap *frame = pixmap();
		QSize frameSize = frame->size();
		frameSize.scale(size(),Qt::KeepAspectRatio);
		QRect frameRect = QStyle::alignedRect(Qt::LeftToRight,Qt::AlignCenter,frameSize,rect());
		p.drawPixmap(frameRect, *frame);
	}
	else
	{
		QRect waitRect = QStyle::alignedRect(Qt::LeftToRight,Qt::AlignCenter,FWaitMovie->currentPixmap().size(),rect());
		p.drawPixmap(waitRect, FWaitMovie->currentPixmap());
	}

	if (FResizeEnabled && underMouse())
	{
		QRect iconRect = QRect(rect().topLeft(),FResizeIcon.size());
		iconRect.moveCenter(QPoint(0,0));
		for (int i=0; i<4; i++)
		{
			if ((FAlignment & Corners[i].align) == 0)
			{
				p.save();
				p.translate(QStyle::alignedRect(Qt::LeftToRight,Corners[i].align,FResizeIcon.size(),rect()).center());
				p.rotate(Corners[i].rotateAngel);
				p.drawPixmap(iconRect,FResizeIcon);
				p.restore();
			}
		}
	}

	QFrame::paintEvent(AEvent);
}

void VideoFrame::onWaitMovieFrameChanged(int AFrameNumber)
{
	Q_UNUSED(AFrameNumber);
	update();
}
