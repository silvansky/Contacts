#ifndef TABWIDGET_H
#define TABWIDGET_H

#include <QStackedWidget>
#include "tabbar.h"

class TabWidget :
			public QWidget
{
	Q_OBJECT
public:
	TabWidget(QWidget *AParent = NULL);
	virtual ~TabWidget();
	int count() const;
	int currentIndex() const;
	void setCurrentIndex(int AIndex);
	QWidget *currentWidget() const;
	void setCurrentWidget(QWidget *AWidget);
	QWidget *widget(int AIndex) const;
	int indexOf(QWidget *AWidget) const;
	QIcon tabIcon(int AIndex) const;
	void setTabIcon(int AIndex, const QIcon &AIcon);
	QString tabIconKey(int AIndex) const;
	void setTabIconKey(int AIndex, const QString &AIconKey);
	QString tabText(int AIndex) const;
	void setTabText(int AIndex, const QString &AText);
	QString tabToolTip(int AIndex) const;
	void setTabToolTip(int AIndex, const QString &AToolTip);
	ITabPageNotify tabNotify(int AIndex) const;
	void setTabNotify(int AIndex, const ITabPageNotify &ANotify);
	bool tabsClosable() const;
	void setTabsClosable(bool ACloseable);
	int addTab(QWidget *AWidget, const QString &ALabel);
	void removeTab(int AIndex);
public slots:
	void showNextTab();
	void showPrevTab();
signals:
	void currentChanged(int AIndex);
	void tabMenuRequested(int AIndex);
	void tabCloseRequested(int AIndex);
protected slots:
	void onCurrentTabChanged(int AIndex);
private:
	TabBar *FTabBar;
	QStackedWidget *FStack;
};

#endif // TABWIDGET_H
