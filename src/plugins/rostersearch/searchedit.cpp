#include "searchedit.h"
#include <QPainter>
#include <definations/resources.h>
#include <definations/menuicons.h>
#include <QResizeEvent>

SearchEdit::SearchEdit(QWidget *parent) :
		QLineEdit(parent)
{
	setMouseTracking(true);
	int padding_left, padding_top, padding_right, padding_bottom;
	getTextMargins(&padding_left, &padding_top, &padding_right, &padding_bottom);
	setTextMargins(padding_left, padding_top, padding_right + 18, padding_bottom);
	connect(this, SIGNAL(textChanged(const QString &)), SLOT(onTextChanged(const QString &)));
	iconStorage = IconStorage::staticStorage(RSR_STORAGE_MENUICONS);
	iconLabel = new QLabel(this);
	iconLabel->setFixedSize(16, 16);
	iconLabel->setMouseTracking(true);
	currentIcon = iconStorage->getIcon(MNI_ROSTERSEARCH_ICON_GLASS);
	if (!currentIcon.isNull())
		iconLabel->setPixmap(currentIcon.pixmap(16, QIcon::Normal, QIcon::On));
}

void SearchEdit::resizeEvent(QResizeEvent * event)
{
	QLineEdit::resizeEvent(event);
	iconLabel->move(event->size().width() - 18, 0);
}

void SearchEdit::mouseMoveEvent(QMouseEvent * event)
{
	if (iconLabel->geometry().contains(event->pos()))
	{
		if (!text().isEmpty())
		{
			setCursor(QCursor(Qt::PointingHandCursor));
			updateIcon(Hover);
		}
		else
			setCursor(QCursor(Qt::ArrowCursor));
	}
	else
	{
		setCursor(QCursor(Qt::IBeamCursor));
		if (text().isEmpty())
			updateIcon(Ready);
		else
			updateIcon(InProgress);
	}
	QLineEdit::mouseMoveEvent(event);
}

void SearchEdit::mousePressEvent(QMouseEvent *event)
{
	if (iconLabel->geometry().contains(event->pos()))
		setText("");
}

void SearchEdit::leaveEvent(QEvent *)
{
	if (text().isEmpty())
		updateIcon(Ready);
	else
		updateIcon(InProgress);
}

void SearchEdit::onTextChanged(const QString &newText)
{
	if (newText.isEmpty())
		updateIcon(Ready);
	else
		updateIcon(InProgress);
}

void SearchEdit::updateIcon(IconState iconState)
{
	if (iconStorage)
	{
		switch (iconState)
		{
		case Ready:
			currentIcon = iconStorage->getIcon(MNI_ROSTERSEARCH_ICON_GLASS);
			break;
		case InProgress:
			currentIcon = iconStorage->getIcon(MNI_ROSTERSEARCH_ICON_CROSS);
			break;
		case Hover:
			currentIcon = iconStorage->getIcon(MNI_ROSTERSEARCH_ICON_CROSS_HOVER);
			break;
		}
		if (!currentIcon.isNull())
			iconLabel->setPixmap(currentIcon.pixmap(16, QIcon::Normal, QIcon::On));
	}
}
