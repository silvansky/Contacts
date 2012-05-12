#include "volumecontrol.h"

#include <QStyle>
#include <QPainter>
#include <QWheelEvent>
#include <QMouseEvent>
#include <QApplication>
#include <definitions/resources.h>
#include <definitions/menuicons.h>
#include <utils/iconstorage.h>

VolumeControl::VolumeControl(QWidget *AParent) : QFrame(AParent)
{
	FVolume = 1.0;
	FSavedVolume = 1.0;
	FMaximumVolume = 4.0;

	FMoved = false;
	FPressedPos = QPoint();

	setProperty("ignoreFilter", true);
	setSizePolicy(QSizePolicy::Fixed,QSizePolicy::Fixed);
	FSizeHint = IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getImage(MNI_SIPPHONE_VOLUMECONTROL_VOLUME,4).size() + QSize(7,7);

	updatePixmap();
}

VolumeControl::~VolumeControl()
{

}

qreal VolumeControl::volume() const
{
	return FVolume;
}

void VolumeControl::setVolume( qreal AVolume )
{
	qreal volume = qMax(qMin(AVolume,FMaximumVolume),0.0);
	if (FVolume != volume)
	{
		FVolume = volume;
		emit volumeChanged(FVolume);
		updatePixmap();
	}
}

qreal VolumeControl::maximumValume() const
{
	return FMaximumVolume;
}

void VolumeControl::setMaximumValume(qreal AVolume)
{
	qreal volume = qMax(AVolume,1.0);
	if (FMaximumVolume != volume)
	{
		FMaximumVolume = volume;
		emit maximumVolumeChanged(FMaximumVolume);
		updatePixmap();
	}
}

void VolumeControl::setEnabled(bool AEnabled)
{
	QFrame::setEnabled(AEnabled);
	updatePixmap();
}

bool VolumeControl::isMutedVolume(qreal AVolume) const
{
	return AVolume*100/FMaximumVolume<1.0;
}

qreal VolumeControl::positionToVolume(const QPoint &APos) const
{
	QRect curRect = rect();
	if (!curRect.isEmpty() && curRect.contains(APos))
	{
		static const int mutePercent = 18*100/50;
		int percent = (APos.x() - curRect.left()) * 100 / (curRect.right() - curRect.left());
		return percent > mutePercent ? FMaximumVolume*(percent-mutePercent)/(100-mutePercent) : 0.0; 
	}
	return FVolume;
}

void VolumeControl::updatePixmap()
{
	qreal percent = FVolume*100/FMaximumVolume;
	if (!isEnabled())
		FCurPixmap = QPixmap::fromImage(IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getImage(MNI_SIPPHONE_VOLUMECONTROL_DISABLED));
	else if (percent > 75.0)
		FCurPixmap = QPixmap::fromImage(IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getImage(MNI_SIPPHONE_VOLUMECONTROL_VOLUME,4));
	else if (percent > 50.0)
		FCurPixmap = QPixmap::fromImage(IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getImage(MNI_SIPPHONE_VOLUMECONTROL_VOLUME,3));
	else if (percent > 25.0)
		FCurPixmap = QPixmap::fromImage(IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getImage(MNI_SIPPHONE_VOLUMECONTROL_VOLUME,2));
	else if (percent > 1.0)
		FCurPixmap = QPixmap::fromImage(IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getImage(MNI_SIPPHONE_VOLUMECONTROL_VOLUME,1));
	else
		FCurPixmap = QPixmap::fromImage(IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getImage(MNI_SIPPHONE_VOLUMECONTROL_VOLUME,0));
	update();
}

QSize VolumeControl::sizeHint() const
{
	return FSizeHint;
}

void VolumeControl::paintEvent(QPaintEvent *AEvent)
{
	QFrame::paintEvent(AEvent);
	QPainter p(this);
	QRect paintRect = QStyle::alignedRect(Qt::LeftToRight, Qt::AlignCenter, FCurPixmap.size(),rect());
	p.drawPixmap(paintRect, FCurPixmap);
}

void VolumeControl::wheelEvent(QWheelEvent *AEvent)
{
	int numDegrees = AEvent->delta() / 8;
	int numSteps = numDegrees / 15;
	setVolume(FVolume + FMaximumVolume * numSteps / 10);
	AEvent->accept();
}

void VolumeControl::mousePressEvent(QMouseEvent *AEvent)
{
	if(AEvent->button() == Qt::LeftButton)
		FPressedPos = AEvent->pos();
}

void VolumeControl::mouseMoveEvent(QMouseEvent *AEvent)
{
	if (!FPressedPos.isNull())
	{
		FMoved = FMoved || (AEvent->pos()-FPressedPos).manhattanLength()>qApp->startDragDistance();
		if (FMoved)
			setVolume(positionToVolume(AEvent->pos()));
	}
}

void VolumeControl::mouseReleaseEvent(QMouseEvent *AEvent)
{
	if (!FPressedPos.isNull() && !FMoved)
	{
		if (positionToVolume(FPressedPos)<0.001 && positionToVolume(AEvent->pos())<0.001)
		{
			if (!isMutedVolume(FVolume))
			{
				FSavedVolume = FVolume;
				setVolume(0.0);
			}
			else
			{
				setVolume(FSavedVolume);
			}
		}
		else
		{
			setVolume(positionToVolume(AEvent->pos()));
		}
	}

	FMoved = false;
	FPressedPos = QPoint();
}
