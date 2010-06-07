#ifndef TABBAR_H
#define TABBAR_H

#include <QFrame>
#include <QMouseEvent>
#include "tabbarlayout.h"
#include "tabbaritem.h"

class TabBar : 
			public QFrame
{
	Q_OBJECT;
public:
	TabBar(QWidget *AParent = NULL);
	~TabBar();
	int count() const;
	int currentIndex() const;
	void setCurrentIndex(int AIndex);
	QIcon tabIcon(int AIndex) const;
	void setTabIcon(int AIndex, const QIcon &AIcon);
	QString tabIconKey(int AIndex) const;
	void setTabIconKey(int AIndex, const QString &AIconKey);
	QString tabText(int AIndex) const;
	void setTabText(int AIndex, const QString &AText);
	QString tabToolTip(int AIndex) const;
	void setTabToolTip(int AIndex, const QString &AToolTip);
	bool tabsClosable() const;
	void setTabsClosable(bool ACloseable);
	int tabAt(const QPoint &APosition) const;
	int addTab(const QString &AText, const QString &AToolTip = QString::null);
	void removeTab(int AIndex);
signals:
	void currentChanged(int AIndex);
	void tabMenuRequested(int AIndex);
	void tabCloseRequested(int AIndex);
protected slots:
	void onCloseButtonClicked();
protected:
	virtual void enterEvent(QEvent *AEvent);
	virtual void leaveEvent(QEvent *AEvent);
	virtual void mousePressEvent(QMouseEvent *AEvent);
	virtual void mouseReleaseEvent(QMouseEvent *AEvent);
	virtual void mouseMoveEvent(QMouseEvent *AEvent);
	virtual void dragEnterEvent(QDragEnterEvent *AEvent);
	virtual void dragMoveEvent(QDragMoveEvent *AEvent);
	virtual void dragLeaveEvent(QDragLeaveEvent *AEvent);
private:
	int FPressedIndex;
	QPoint FPressedPos;
	QPoint FDragCenterDistance;
private:
	int FActiveIndex;
	bool FTabsCloseable;
	TabBarLayout *FLayout;
	QList<TabBarItem *> FItems;
};

#endif // TABBAR_H
