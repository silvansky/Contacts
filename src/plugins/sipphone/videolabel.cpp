#include "videolabel.h"

#include <QStyle>
#include <QPainter>
#include <QVariant>
#include <definitions/resources.h>
#include <definitions/menuicons.h>
#include <utils/iconstorage.h>

VideoLabel::VideoLabel(QWidget *AParent) : QLabel(AParent)
{
	setMouseTracking(true);

	FMoveEnabled = false;
	FResizeEnabled = false;
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

QSize VideoLabel::sizeHint() const
{
	const QPixmap *frame = pixmap();
	return frame!=NULL && !frame->isNull() ? frame->size() : QLabel::sizeHint();
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
	QLabel::leaveEvent(AEvent);
}

void VideoLabel::mouseMoveEvent(QMouseEvent *AEvent)
{
	QLabel::mouseMoveEvent(AEvent);
}

void VideoLabel::mousePressEvent(QMouseEvent *AEvent)
{
	QLabel::mousePressEvent(AEvent);
}

void VideoLabel::mouseReleaseEvent(QMouseEvent *AEvent)
{
	QLabel::mouseReleaseEvent(AEvent);
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
		static const struct { qreal angel; Qt::Alignment align; } corners[] = { 
			{ 0.0,   Qt::AlignLeft|Qt::AlignTop },
			{ 90.0,  Qt::AlignRight|Qt::AlignTop },
			{ 180.0, Qt::AlignRight|Qt::AlignBottom },
			{ 270.0, Qt::AlignLeft|Qt::AlignBottom }
		};

		QRect iconRect = QRect(rect().topLeft(),FResizeIcon.size());
		iconRect.moveCenter(QPoint(0,0));
		for (int i=0; i<4; i++)
		{
			if ((FAlignment & corners[i].align) == 0)
			{
				p.save();
				p.translate(QStyle::alignedRect(Qt::LeftToRight,corners[i].align,FResizeIcon.size(),rect()).center());
				p.rotate(corners[i].angel);
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
