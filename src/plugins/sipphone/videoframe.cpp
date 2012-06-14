#include "videoframe.h"

#include <QStyle>
#include <QPainter>
#include <QVariant>
#include <QMouseEvent>
#include <QApplication>
#include <definitions/resources.h>
#include <definitions/menuicons.h>
#include <utils/iconstorage.h>
#include <utils/custombordercontainer.h>

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

	FMoved = false;
	FClicked = false;
	FCursorCorner = -1;
	FCollapsed = false;
	FMoveEnabled = false;
	FResizeEnabled = false;
	FMinimumSize = QSize(50,50);
	FMaximumSize = QSize(QWIDGETSIZE_MAX,QWIDGETSIZE_MAX);
	FAlignment = Qt::AlignRight|Qt::AlignBottom;
	FDoubleClickTime = QDateTime::currentDateTime();
	FDeviceState = ISipDevice::DS_UNAVAIL;

	QIcon icon = IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getIcon(MNI_SIPPHONE_VIDEO_RESIZE);
	FResizeIcon = icon.pixmap(icon.availableSizes().value(0));

	icon = IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getIcon(MNI_SIPPHONE_VIDEO_COLLAPSED);
	FCollapsedIcon = icon.pixmap(icon.availableSizes().value(0));

	icon = IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getIcon(MNI_SIPPHONE_CAMERA_DISABLED);
	FCameraDisabledIcon = icon.pixmap(icon.availableSizes().value(0));

	FWaitMovie = new QMovie(this);
	FWaitMovie->setFileName(IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->fileFullName(MNI_SIPPHONE_VIDEO_WAIT));
	connect(FWaitMovie,SIGNAL(frameChanged(int)),SLOT(onWaitMovieFrameChanged(int)));
	FWaitMovie->start();
}

VideoFrame::~VideoFrame()
{

}

bool VideoFrame::isNull() const
{
	return FDeviceState!=ISipDevice::DS_ENABLED;
}

bool VideoFrame::isEmpty() const
{
	return isNull() || FVideoFrame.isNull();
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
		updateGeometry();
		emit stateChanged();
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
		setProperty(CBC_IGNORE_FILTER, FMoveEnabled||FResizeEnabled);
		emit stateChanged();
		update();
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
		setProperty(CBC_IGNORE_FILTER, FMoveEnabled||FResizeEnabled);
		emit stateChanged();
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
		emit stateChanged();
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
	emit stateChanged();
}

QSize VideoFrame::maximumVideoSize() const
{
	return FMaximumSize;
}

void VideoFrame::setMaximumVideoSize(const QSize &ASize)
{
	FMaximumSize = ASize;
	emit stateChanged();
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

int VideoFrame::videoDeviceState() const
{
	return FDeviceState;
}

void VideoFrame::setVideoDeviceState(int AState)
{
	if (FDeviceState != AState)
	{
		FDeviceState = AState;
		emit stateChanged();
		update();
	}
}

QSize VideoFrame::sizeHint() const
{
	if (FCollapsed)
		return !FCollapsedIcon.isNull() ? FCollapsedIcon.size() : QSize(15,15);
	else if (isEmpty())
		return FMinimumSize;
	return pixmap()->size();
}

QSize VideoFrame::minimumSizeHint() const
{
	return FMinimumSize;
}

void VideoFrame::enterEvent(QEvent *AEvent)
{
	if (!FCollapsed && FResizeEnabled)
		update();
	QFrame::enterEvent(AEvent);
}

void VideoFrame::leaveEvent(QEvent *AEvent)
{
	if (!FCollapsed && FResizeEnabled)
		update();
	FCursorCorner = -1;
	setCursor(Qt::ArrowCursor);
	QFrame::leaveEvent(AEvent);
}

void VideoFrame::mouseMoveEvent(QMouseEvent *AEvent)
{
	static const QSize cornerSize = QSize(10,10);

	if (FPressedPos.isNull() || FCollapsed)
	{
		FCursorCorner = -1;
		for (int i=0; FCursorCorner<0 && i<4; i++)
			if ((Corners[i].align & FAlignment)==0 && QStyle::alignedRect(Qt::LeftToRight,Corners[i].align,cornerSize,rect()).contains(AEvent->pos()))
				FCursorCorner = i;

		if (FCollapsed)
			setCursor(Qt::ArrowCursor);
		else if (FResizeEnabled && FCursorCorner>=0)
			setCursor(Corners[FCursorCorner].cursor);
		else if (FMoveEnabled)
			setCursor(Qt::OpenHandCursor);
	}
	else if (FResizeEnabled && FCursorCorner>=0)
	{
		FMoved = true;
		emit resizeTo((Qt::Corner)FCursorCorner,mapToParent(AEvent->pos()));
	}
	else if (FMoveEnabled && FMoved)
	{
		emit moveTo(mapToParent(AEvent->pos())-FPressedPos);
	}
	else if ((FGlobalPressed-AEvent->globalPos()).manhattanLength()>=qApp->startDragDistance())
	{
		FMoved = true;
	}

	QFrame::mouseMoveEvent(AEvent);
}

void VideoFrame::mousePressEvent(QMouseEvent *AEvent)
{
	if (AEvent->button() == Qt::LeftButton)
	{
		FPressedPos = AEvent->pos();
		FGlobalPressed = AEvent->globalPos();
		if (!FCollapsed)
		{
			if (FMoveEnabled && FCursorCorner<0)
			{
				FPressedPos = mapToParent(AEvent->pos()) - geometry().topLeft();
				setCursor(Qt::ClosedHandCursor);
			}
			else if (!FResizeEnabled || FCursorCorner<0)
			{
				QFrame::mousePressEvent(AEvent);
			}
		}
		else
		{
			QFrame::mousePressEvent(AEvent);
		}
		FClicked = FDoubleClickTime.msecsTo(QDateTime::currentDateTime())>qApp->doubleClickInterval();
	}
	else
	{
		QFrame::mousePressEvent(AEvent);
	}
}

void VideoFrame::mouseReleaseEvent(QMouseEvent *AEvent)
{
	if (!FPressedPos.isNull() && AEvent->button()==Qt::LeftButton)
	{
		if (!FCollapsed && FMoveEnabled && FCursorCorner<0)
			setCursor(Qt::OpenHandCursor);
		if (FClicked && !FMoved)
			QTimer::singleShot(qApp->doubleClickInterval(),this,SLOT(onEmitSingleClicked()));
		FMoved = false;
		FPressedPos = QPoint();
	}
	QFrame::mouseReleaseEvent(AEvent);
}

void VideoFrame::mouseDoubleClickEvent(QMouseEvent *AEvent)
{
	Q_UNUSED(AEvent);
	emit doubleClicked();
	FClicked = false;
	FDoubleClickTime = QDateTime::currentDateTime();
}

void VideoFrame::paintEvent(QPaintEvent *AEvent)
{
	QFrame::paintEvent(AEvent);

	QPainter p(this);
	if (frameShape() != QFrame::NoFrame)
		p.setClipRect(rect().adjusted(lineWidth(),lineWidth(),-lineWidth(),-lineWidth()),Qt::IntersectClip);
	p.fillRect(rect(), Qt::black);
	
	if (FCollapsed)
	{
		QRect iconRect = QStyle::alignedRect(Qt::LeftToRight,Qt::AlignCenter,sizeHint(),rect());
		p.drawPixmap(iconRect, FCollapsedIcon);
	}
	else if (isNull())
	{
		QString text = FDeviceState==ISipDevice::DS_DISABLED ? tr("Camera disabled") : tr("Camera unavailable");

		QSize textSize = fontMetrics().size(Qt::AlignHCenter|Qt::AlignTop|Qt::TextSingleLine,text);
		QSize iconSize = FCameraDisabledIcon.size();

		QSize iconTextSize(qMax(textSize.width(),iconSize.width()),iconSize.height()+textSize.height()+5);
		QRect iconTextRect = QStyle::alignedRect(Qt::LeftToRight,Qt::AlignCenter,iconTextSize,rect());

		QRect iconRect = QStyle::alignedRect(Qt::LeftToRight,Qt::AlignHCenter|Qt::AlignTop,iconSize,iconTextRect);
		QRect textRect = QStyle::alignedRect(Qt::LeftToRight,Qt::AlignHCenter|Qt::AlignBottom,textSize,iconTextRect);
		
		p.drawPixmap(iconRect, FCameraDisabledIcon);
		p.drawText(textRect,Qt::AlignCenter,text);
	}
	else if (!isEmpty())
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

	if (!FCollapsed && FResizeEnabled && underMouse())
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
}

void VideoFrame::onWaitMovieFrameChanged(int AFrameNumber)
{
	Q_UNUSED(AFrameNumber);
	update();
}

void VideoFrame::onEmitSingleClicked()
{
	if (FClicked)
		emit singleClicked();
}
