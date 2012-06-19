#include "searchedit.h"

#include <QPainter>
#include <QResizeEvent>
#include <definitions/resources.h>
#include <utils/stylestorage.h>
#include <utils/custombordercontainer.h>

SearchEdit::SearchEdit(QWidget *parent) : QLineEdit(parent)
{
	setAttribute(Qt::WA_MacShowFocusRect, false);
	setMouseTracking(true);
	int padding_left, padding_top, padding_right, padding_bottom;
	getTextMargins(&padding_left, &padding_top, &padding_right, &padding_bottom);
	setTextMargins(padding_left, padding_top, padding_right, padding_bottom);
	connect(this, SIGNAL(textChanged(const QString &)), SLOT(onTextChanged(const QString &)));
	iconStorage = IconStorage::staticStorage(RSR_STORAGE_MENUICONS);
	iconLabel = new QLabel(this);
	iconLabel->setFixedSize(16, 16);
	iconLabel->setMouseTracking(true);
	iconLabel->setProperty(CBC_IGNORE_FILTER, true);
	//currentIcon = iconStorage->getIcon(MNI_ROSTERSEARCH_ICON_GLASS);
	//if (!currentIcon.isNull())
	//	iconLabel->setPixmap(currentIcon.pixmap(16, QIcon::Normal, QIcon::On));
}

void SearchEdit::processKeyPressEvent(QKeyEvent * event)
{
	keyPressEvent(event);
}

void SearchEdit::resizeEvent(QResizeEvent * event)
{
	QLineEdit::resizeEvent(event);
	static const int rightMargin = StyleStorage::staticStorage(RSR_STORAGE_STYLESHEETS)->getStyleInt(SV_RS_SEARCHEDIT_RIGHT_MARGIN);
	static const int bottomMargin = StyleStorage::staticStorage(RSR_STORAGE_STYLESHEETS)->getStyleInt(SV_RS_SEARCHEDIT_BOTTOM_MARGIN);
	iconLabel->move(event->size().width() - rightMargin, (event->size().height() - bottomMargin) / 2);
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
			setCursor(QCursor(Qt::IBeamCursor));
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
	{
		if (!text().isEmpty())
			setText("");
		else
			setFocus();
	}
}

void SearchEdit::leaveEvent(QEvent *)
{
	if (text().isEmpty())
		updateIcon(Ready);
	else
		updateIcon(InProgress);
}

void SearchEdit::keyPressEvent(QKeyEvent * ke)
{
	if ((ke->key() == Qt::Key_Escape) && !text().isEmpty())
	{
		setText("");
	}
	else
		QLineEdit::keyPressEvent(ke);
}

void SearchEdit::onTextChanged(const QString &newText)
{
	if (newText.isEmpty())
	{
		updateIcon(Ready);
		iconLabel->setToolTip(QString::null);
	}
	else
	{
		updateIcon(InProgress);
		iconLabel->setToolTip(tr("Clear field"));
	}
}

void SearchEdit::updateIcon(IconState iconState)
{
	if (iconStorage)
	{
		switch (iconState)
		{
		case Ready:
			currentIcon = QIcon();//iconStorage->getIcon(MNI_ROSTERSEARCH_ICON_GLASS);
			break;
		case InProgress:
			currentIcon = iconStorage->getIcon(MNI_ROSTERSEARCH_ICON_CROSS,0);
			break;
		case Hover:
			currentIcon = iconStorage->getIcon(MNI_ROSTERSEARCH_ICON_CROSS,1);
			break;
		}
		iconLabel->setPixmap(currentIcon.pixmap(16, QIcon::Normal, QIcon::On));
	}
}
