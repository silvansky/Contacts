#include "tabwidget.h"

#include <QVBoxLayout>
#include <QMouseEvent>

TabWidget::TabWidget(QWidget *AParent) : QWidget(AParent)
{
	setLayout(new QVBoxLayout);
	layout()->setMargin(0);
	layout()->setSpacing(0);
	layout()->addWidget(FStack = new QStackedWidget(this));
	layout()->addWidget(FTabBar = new TabBar(this));

	connect(FTabBar,SIGNAL(tabMenuRequested(int)),SIGNAL(tabMenuRequested(int)));
	connect(FTabBar,SIGNAL(tabCloseRequested(int)),SIGNAL(tabCloseRequested(int)));
	connect(FTabBar,SIGNAL(currentChanged(int)),SLOT(onCurrentTabChanged(int)));
}

TabWidget::~TabWidget()
{
	while (count()>0)
		removeTab(0);
}

int TabWidget::count() const
{
	return FTabBar->count();
}

int TabWidget::currentIndex() const
{
	return FTabBar->currentIndex();
}

void TabWidget::setCurrentIndex(int AIndex)
{
	FTabBar->setCurrentIndex(AIndex);
}

QWidget *TabWidget::currentWidget() const
{
	return FStack->currentWidget();
}

void TabWidget::setCurrentWidget(QWidget *AWidget)
{
	FTabBar->setCurrentIndex(indexOf(AWidget));
}

QWidget *TabWidget::widget(int AIndex) const
{
	return FStack->widget(AIndex);
}

int TabWidget::indexOf(QWidget *AWidget) const
{
	return FStack->indexOf(AWidget);
}

QIcon TabWidget::tabIcon(int AIndex) const
{
	return FTabBar->tabIcon(AIndex);
}

void TabWidget::setTabIcon(int AIndex, const QIcon &AIcon)
{
	FTabBar->setTabIcon(AIndex, AIcon);
}

QString TabWidget::tabIconKey(int AIndex) const
{
	return FTabBar->tabIconKey(AIndex);
}

void TabWidget::setTabIconKey(int AIndex, const QString &AIconKey)
{
	FTabBar->setTabIconKey(AIndex, AIconKey);
}

QString TabWidget::tabText(int AIndex) const
{
	return FTabBar->tabText(AIndex);
}

void TabWidget::setTabText(int AIndex, const QString &AText)
{
	FTabBar->setTabText(AIndex, AText);
}

QString TabWidget::tabToolTip(int AIndex) const
{
	return FTabBar->tabToolTip(AIndex);
}

void TabWidget::setTabToolTip(int AIndex, const QString &AToolTip)
{
	FTabBar->setTabToolTip(AIndex, AToolTip);
}

bool TabWidget::tabsClosable() const
{
	return FTabBar->tabsClosable();
}

void TabWidget::setTabsClosable(bool ACloseable)
{
	FTabBar->setTabsClosable(ACloseable);
}

int TabWidget::addTab(QWidget *AWidget, const QString &ALabel, const QString &AToolTip)
{
	FStack->addWidget(AWidget);
	return FTabBar->addTab(ALabel,AToolTip);
}

void TabWidget::removeTab(int AIndex)
{
	if (AIndex>=0 && AIndex<count())
	{
		FStack->removeWidget(widget(AIndex));
		FTabBar->removeTab(AIndex);
	}
}

void TabWidget::onCurrentTabChanged(int AIndex)
{
	FStack->setCurrentIndex(AIndex);
	emit currentChanged(AIndex);
}
