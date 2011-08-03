#ifndef TABWINDOW_H
#define TABWINDOW_H

#include <QMainWindow>
#include <QPushButton>
#include <definitions/optionvalues.h>
#include <definitions/resources.h>
#include <definitions/menuicons.h>
#include <definitions/customborder.h>
#include <definitions/stylesheets.h>
#include <definitions/actiongroups.h>
#include <interfaces/imessagewidgets.h>
#include <utils/options.h>
#include <utils/stylestorage.h>
#include <utils/widgetmanager.h>
#include <utils/customborderstorage.h>
#include "ui_tabwindow.h"

class TabWindow :
	public QMainWindow,
	public ITabWindow
{
	Q_OBJECT;
	Q_INTERFACES(ITabWindow);
public:
	TabWindow(IMessageWidgets *AMessageWidgets, const QUuid &AWindowId);
	virtual ~TabWindow();
	virtual QMainWindow *instance() { return this; }
	virtual void showWindow();
	virtual void showMinimizedWindow();
	virtual QUuid windowId() const;
	virtual QString windowName() const;
	virtual Menu *windowMenu() const;
	virtual void addTabPage(ITabPage *APage);
	virtual bool hasTabPage(ITabPage *APage) const;
	virtual ITabPage *currentTabPage() const;
	virtual void setCurrentTabPage(ITabPage *APage);
	virtual void detachTabPage(ITabPage *APage);
	virtual void removeTabPage(ITabPage *APage);
signals:
	void currentTabPageChanged(ITabPage *APage);
	void tabPageAdded(ITabPage *APage);
	void tabPageRemoved(ITabPage *APage);
	void tabPageDetached(ITabPage *APage);
	void windowChanged();
	void windowDestroyed();
protected:
	void createActions();
	void saveWindowStateAndGeometry();
	void loadWindowStateAndGeometry();
	void clearTabs();
	void updateWindow();
	void updateTab(int AIndex);
protected slots:
	void onTabChanged(int AIndex);
	void onTabMenuRequested(int AIndex);
	void onTabCloseRequested(int AIndex);
	void onTabPageShow();
	void onTabPageShowMinimized();
	void onTabPageClose();
	void onTabPageChanged();
	void onTabPageDestroyed();
	void onTabPageNotifierChanged();
	void onTabPageNotifierActiveNotifyChanged(int ANotifyId);
	void onTabWindowNameChanged(const QUuid &AWindowId, const QString &AName);
	void onTabMenuActionTriggered(bool);
	void onWindowMenuActionTriggered(bool);
private:
	Ui::TabWindowClass ui;
private:
	IMessageWidgets *FMessageWidgets;
private:
	Menu *FWindowMenu;
	Action *FNextTab;
	Action *FPrevTab;
	Action *FCloseTab;
	Action *FCloseAllTabs;
	Action *FCloseWindow;
private:
	QUuid FWindowId;
	QString FLastClosedTab;
	CustomBorderContainer *FBorder;
};

#endif // TABWINDOW_H
