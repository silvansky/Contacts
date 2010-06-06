#include "tabbar.h"

TabBar::TabBar(QWidget *AParent) : QFrame(AParent)
{
	FActiveIndex = -1;
	FTabsCloseable = true;

	setFrameShadow(QFrame::Plain);
	setFrameShape(QFrame::StyledPanel);

	setLayout(FLayout = new TabBarLayout);

	int minItemWidth, maxItemWidth;
	TabBarItem *item = new TabBarItem(this);
	item->setText("12345");
	minItemWidth = item->sizeHint().width();
	item->setText("12345678901234567890");
	maxItemWidth = item->sizeHint().width();
	delete item;

	FLayout->setMinMaxItemWidth(minItemWidth,maxItemWidth);
}

TabBar::~TabBar()
{
	while (FItems.count() > 0)
		removeTab(0);
}

int TabBar::count() const
{
	return FItems.count();
}

int TabBar::currentIndex() const
{
	return FActiveIndex;
}

void TabBar::setCurrentIndex(int AIndex)
{
	if (AIndex>=0 && AIndex<FItems.count())
	{
		TabBarItem *item = FItems.value(FActiveIndex);
		if (item)
			item->setActive(false);

		FActiveIndex = AIndex;

		item = FItems.value(AIndex);
		if (item)
			item->setActive(true);

		emit currentChanged(AIndex);
	}
}

QIcon TabBar::tabIcon(int AIndex) const
{
	TabBarItem *item = FItems.value(AIndex);
	return item!=NULL ? item->icon() : QIcon();
}

void TabBar::setTabIcon(int AIndex, const QIcon &AIcon)
{
	TabBarItem *item = FItems.value(AIndex);
	if (item)
		item->setIcon(AIcon);
}

QString TabBar::tabIconKey(int AIndex) const
{
	TabBarItem *item = FItems.value(AIndex);
	return item!=NULL ? item->iconKey() : QString::null;
}

void TabBar::setTabIconKey(int AIndex, const QString &AIconKey)
{
	TabBarItem *item = FItems.value(AIndex);
	if (item)
		item->setIconKey(AIconKey);
}

QString TabBar::tabText(int AIndex) const
{
	TabBarItem *item = FItems.value(AIndex);
	return item!=NULL ? item->text() : QString::null;
}

void TabBar::setTabText(int AIndex, const QString &AText)
{
	TabBarItem *item = FItems.value(AIndex);
	if (item)
		item->setText(AText);
}

QString TabBar::tabToolTip(int AIndex) const
{
	TabBarItem *item = FItems.value(AIndex);
	return item!=NULL ? item->toolTip() : QString::null;
}

void TabBar::setTabToolTip(int AIndex, const QString &AToolTip)
{
	TabBarItem *item = FItems.value(AIndex);
	if (item)
		item->setToolTip(AToolTip);
}

bool TabBar::tabsClosable() const
{
	return FTabsCloseable;
}

void TabBar::setTabsClosable(bool ACloseable)
{
	foreach(TabBarItem *item, FItems)
		item->setCloseable(ACloseable);
	FTabsCloseable = ACloseable;
}

int TabBar::tabAt(const QPoint &APosition) const
{
	int index = -1;
	for (int i=0; index<0 && i<FItems.count(); i++)
		if (FItems.at(i)->geometry().contains(APosition))
			index = i;
	return index;
}

int TabBar::addTab(const QString &AText, const QString &AToolTip)
{
	TabBarItem *item = new TabBarItem(this);
	item->setText(AText);
	item->setToolTip(AToolTip);
	item->setCloseable(FTabsCloseable);
	FItems.append(item);
	layout()->addWidget(item);
	connect(item,SIGNAL(closeButtonClicked()),SLOT(onCloseButtonClicked()));

	int index = FItems.indexOf(item);
	if (index == 0)
		setCurrentIndex(index);

	return index;
}

void TabBar::removeTab(int AIndex)
{
	if (AIndex>=0 && AIndex<FItems.count())
	{
		int activeIndex = FActiveIndex;
		if (AIndex == activeIndex)
			activeIndex = activeIndex>0 ? activeIndex-1 : activeIndex+1;
		if (AIndex < activeIndex)
			activeIndex--;
		delete FItems.takeAt(AIndex);
		setCurrentIndex(activeIndex);
	}
}

void TabBar::enterEvent(QEvent *AEvent)
{
	setFixedSize(size());
	FLayout->blockUpdate(true);
	QFrame::enterEvent(AEvent);
}

void TabBar::leaveEvent(QEvent *AEvent)
{
	setMinimumSize(QSize(0,0));
	setMaximumSize(QSize(QWIDGETSIZE_MAX,QWIDGETSIZE_MAX));
	FLayout->blockUpdate(false);
	QFrame::leaveEvent(AEvent);
}

void TabBar::mousePressEvent(QMouseEvent *AEvent)
{
	FPressedIndex = tabAt(AEvent->pos());
	QWidget::mousePressEvent(AEvent);
}

void TabBar::mouseReleaseEvent(QMouseEvent *AEvent)
{
	int index = tabAt(AEvent->pos());
	if (index>=0 && index == FPressedIndex)
	{
		if (AEvent->button() == Qt::LeftButton)
		{
			setCurrentIndex(index);
		}
		else if (AEvent->button() == Qt::MidButton)
		{
			emit tabCloseRequested(index);
		}
		else if (AEvent->button() == Qt::RightButton)
		{
			emit tabMenuRequested(index);
		}
	}
	FPressedIndex = -1;
	QWidget::mouseReleaseEvent(AEvent);
}

void TabBar::onCloseButtonClicked()
{
	TabBarItem *item = qobject_cast<TabBarItem *>(sender());
	if (item)
		emit tabCloseRequested(FItems.indexOf(item));
}

