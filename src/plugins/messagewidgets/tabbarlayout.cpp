#include "tabbarlayout.h"

#include <QWidget>

TabBarLayout::TabBarLayout(QWidget *AParent) : QLayout(AParent)
{
	FMinWidth = 80;
	FMaxWidth = 180;
	FItemsWidth = FMaxWidth;
	FStreatch = false;
	FUpdateBlocked = false;

	setMargin(1);
	setSpacing(1);
}

TabBarLayout::~TabBarLayout()
{
	QLayoutItem *item;
	while (item =takeAt(0))
		delete item;
}

int TabBarLayout::minimumItemWidth() const
{
	return FMinWidth;
}

int TabBarLayout::maximumItemWidth() const
{
	return FMaxWidth;
}

void TabBarLayout::setMinMaxItemWidth(int AMin, int AMax)
{
	if (AMin>0 && AMax>AMin)
	{
		FMinWidth = AMin;
		FMaxWidth = AMax;
		updateLayout();
	}
}

void TabBarLayout::blockUpdate(bool ABlock)
{
	FUpdateBlocked = ABlock;
	updateLayout();
}

void TabBarLayout::updateLayout()
{
	if (FUpdateBlocked)
	{
		int fakeItemWidth;
		calcLayoutParams(geometry().width(),fakeItemWidth,FStreatch);
	}
	else
		calcLayoutParams(geometry().width(),FItemsWidth,FStreatch);
	doLayout(geometry(),FItemsWidth,FStreatch,true);
}

int TabBarLayout::count() const
{
	return FItems.count();
}

void TabBarLayout::addItem(QLayoutItem *AItem)
{
	FItems.append(AItem);
	updateLayout();
}

QLayoutItem *TabBarLayout::itemAt(int AIndex) const
{
	return FItems.value(AIndex, NULL);
}

QLayoutItem *TabBarLayout::takeAt(int AIndex)
{
	return AIndex>=0 && AIndex<FItems.count() ? FItems.takeAt(AIndex) : NULL;
}

QSize TabBarLayout::sizeHint() const
{
	int left, top, right, bottom;
	getContentsMargins(&left, &top, &right, &bottom);

	int height = 0;
	foreach(QLayoutItem *item, FItems)
		height = qMax(height, item->sizeHint().height());
	int width = (FMaxWidth + spacing()) * FItems.count() - spacing();

	return QSize(left + width + right, top + height + bottom);
}

bool TabBarLayout::hasHeightForWidth() const
{
	return true;
}

int TabBarLayout::heightForWidth(int AWidth) const
{
	int itemWidth;
	bool streatch;
	calcLayoutParams(AWidth,itemWidth,streatch);
	return doLayout(QRect(0,0,AWidth,0),itemWidth,streatch,false);
}

Qt::Orientations TabBarLayout::expandingDirections() const
{
	return Qt::Horizontal;
}

void TabBarLayout::setGeometry(const QRect &ARect)
{
	QLayout::setGeometry(ARect);
	updateLayout();
}

void TabBarLayout::calcLayoutParams(int AWidth, int &AItemWidth, bool &AStreatch) const
{
	int left, right;
	getContentsMargins(&left, NULL, &right, NULL);

	int availWidth = AWidth - left - right - 1;

	if ((FMaxWidth + spacing()) * FItems.count() - spacing() >= availWidth)
	{
		int itemsPerLine = qMin(availWidth / (FMinWidth + spacing()),FItems.count());
		AItemWidth = itemsPerLine >0 ? (availWidth - spacing()*(itemsPerLine-1)) / itemsPerLine : FMinWidth;
		AStreatch = true;
	}
	else
	{
		AItemWidth = FMaxWidth;
		AStreatch = false;
	}
}

int TabBarLayout::doLayout(QRect ARect, int AItemWidth, bool AStreatch, bool AResize) const
{
	int left, top, right, bottom;
	getContentsMargins(&left, &top, &right, &bottom);

	QRect availRect = ARect.adjusted(+left,+top,-right,-bottom);

	int x = availRect.left();
	int y = availRect.top();
	int lineHeight = 0;

	int counter =0;
	foreach(QLayoutItem *item, FItems)
	{
		counter++;
		QRect itemRect = QRect(x,y,AItemWidth,item->sizeHint().height());
		lineHeight = qMax(lineHeight, itemRect.height());
		x += AItemWidth + spacing();

		if ( x + AItemWidth - spacing() > availRect.right() )
		{
			if (item != FItems.last())
			{
				y += lineHeight + spacing();
				lineHeight = 0;
			}
			if (AStreatch)
			{
				itemRect.setRight(availRect.right());
			}
			x = availRect.left();
		}

		if (AResize)
		{
			item->setGeometry(itemRect);
		}
	}
	return y - availRect.top() + lineHeight + top + bottom;
}
