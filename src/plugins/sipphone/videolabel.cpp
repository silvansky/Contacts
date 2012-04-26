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

VideoLabel::VideoLabel(QWidget *AParent) : QLabel(AParent)
{
	setMouseTracking(true);

	FCursorCorner = -1;
	FMoveEnabled = false;
	FResizeEnabled = false;
	FMinimumSize = QSize(50,50);
	FMaximumSize = QSize(QWIDGETSIZE_MAX,QWIDGETSIZE_MAX);
	FAlignment = Qt::AlignRight|Qt::AlignBottom;

	QIcon icon = IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getIcon(MNI_SIPPHONE_VIDEO_RESIZE);
	FResizeIcon = icon.pixmap(icon.availableSizes().value(0));

	FWaitMovie = new QMovie(this);
	FWaitMovie->setFileName(IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->fileFullName(MNI_SIPPHONE_VIDEO_WAIT));
	connect(FWaitMovie,SIGNAL(frameChanged(int)),SLOT(onWaitMovieFrameChanged(int)));
	FWaitMovie->start();
}

VideoLabel::~VideoLabel()
{

}

bool VideoLabel::isMoveEnabled() const
{
	return FMoveEnabled;
}

void VideoLabel::setMoveEnabled(bool AEnabled)
{
	if (FMoveEnabled != AEnabled)
	{
		FMoveEnabled = AEnabled;
		setProperty("ignoreFilter", AEnabled);
	}
}

bool VideoLabel::isResizeEnabled() const
{
	return FResizeEnabled;
}

void VideoLabel::setResizeEnabled(bool AEnabled)
{
	if (FResizeEnabled != AEnabled)
	{
		FResizeEnabled = AEnabled;
		update();
	}
}

Qt::Alignment VideoLabel::alignment() const
{
	return FAlignment;
}

void VideoLabel::setAlignment(Qt::Alignment AAlign)
{
	if (FAlignment != AAlign)
	{
		FAlignment = AAlign;
		update();
	}
}

QSize VideoLabel::minimumVideoSize() const
{
	return FMinimumSize;
}

void VideoLabel::setMinimumVideoSize(const QSize &ASize)
{
	FMinimumSize = ASize;
}

QSize VideoLabel::maximumVideoSize() const
{
	return FMaximumSize;
}

void VideoLabel::setMaximumVideoSize(const QSize &ASize)
{
	FMaximumSize = ASize;
}

QSize VideoLabel::sizeHint() const
{
	const QPixmap *frame = pixmap();
	return frame!=NULL && !frame->isNull() ? frame->size() : FMinimumSize;
}

QSize VideoLabel::minimumSizeHint() const
{
	return FMinimumSize;
}

void VideoLabel::setPixmap(const QPixmap &APixmap)
{
	if (APixmap.isNull())
		FWaitMovie->start();
	else
		FWaitMovie->stop();
	QLabel::setPixmap(APixmap);
}

void VideoLabel::enterEvent(QEvent *AEvent)
{
	if (FResizeEnabled)
		update();
	QLabel::enterEvent(AEvent);
}

void VideoLabel::leaveEvent(QEvent *AEvent)
{
	if (FResizeEnabled)
		update();
	FCursorCorner = -1;
	setCursor(Qt::ArrowCursor);
	QLabel::leaveEvent(AEvent);
}

void VideoLabel::mouseMoveEvent(QMouseEvent *AEvent)
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

	QLabel::mouseMoveEvent(AEvent);
}

void VideoLabel::mousePressEvent(QMouseEvent *AEvent)
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
		QLabel::mousePressEvent(AEvent);
	}
}

void VideoLabel::mouseReleaseEvent(QMouseEvent *AEvent)
{
	if (!FPressedPos.isNull())
	{
		if (FMoveEnabled && FCursorCorner<0)
			setCursor(Qt::OpenHandCursor);
		FPressedPos = QPoint();
	}
	else
	{
		QLabel::mousePressEvent(AEvent);
	}
}

void VideoLabel::paintEvent(QPaintEvent *AEvent)
{
	Q_UNUSED(AEvent);
	QPainter p(this);
	p.fillRect(rect(),Qt::black);
	
	const QPixmap *frame = pixmap();
	if (frame && !frame->isNull())
	{
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

void VideoLabel::onWaitMovieFrameChanged(int AFrameNumber)
{
	Q_UNUSED(AFrameNumber);
	update();
}
