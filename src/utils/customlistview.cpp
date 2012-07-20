#include "customlistview.h"

#include "customborderstorage.h"
#include <definitions/resources.h>
#include <definitions/customborder.h>

#include <QEvent>

CustomListView::CustomListView() :
	QListView(NULL),
	border(NULL)
{
	setBorder();
}

void CustomListView::setBorder()
{
	border = CustomBorderStorage::staticStorage(RSR_STORAGE_CUSTOMBORDER)->addBorder(this, CBS_MENU);
	if (border)
	{
		setFrameShape(QFrame::NoFrame);
		setWindowFlags(Qt::Widget);
		border->setShowInTaskBar(false);
		border->setResizable(false);
		border->setMovable(false);
		border->setMinimizeButtonVisible(false);
		border->setMaximizeButtonVisible(false);
		border->setCloseButtonVisible(false);
	}
}

bool CustomListView::event(QEvent * evt)
{
	switch(evt->type())
	{
	case QEvent::Show:
		if (border)
		{
			border->show();
		}
		break;
	case QEvent::ShowToParent:
		if (border)
		{
			if (!parentWidget())
			{
				border->releaseWidget();
				border->setWidget(this);
			}
			QListView::event(evt);
			QRect geom = geometry();
			QPoint p = geom.topLeft();
			p.setX(p.x() - border->leftBorderWidth());
			p.setY(p.y() - border->topBorderWidth());
			geom.moveTopLeft(p);
			geom.setWidth(geom.width() + border->leftBorderWidth() + border->rightBorderWidth());
			geom.setHeight(geom.height() + border->topBorderWidth() + border->bottomBorderWidth());
			border->setGeometry(geom);
			border->show();
			return true;
		}
		break;
	case QEvent::Hide:
		if (border)
		{
			border->hide();
			return true;
		}
		break;
	default:
		break;
	}
	return QListView::event(evt);
}
