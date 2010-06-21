#ifndef TABWINDOW_H
#define TABWINDOW_H

#include <QMainWindow>
#include <QPushButton>
#include <definations/optionvalues.h>
#include <definations/resources.h>
#include <definations/menuicons.h>
#include <definations/stylesheets.h>
#include <definations/actiongroups.h>
#include <interfaces/imessagewidgets.h>
#include <utils/options.h>
#include <utils/stylestorage.h>
#include <utils/widgetmanager.h>
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
	virtual QUuid windowId() const;
	virtual QString windowName() const;
	virtual Menu *windowMenu() const;
	virtual void addTabPage(ITabPage *APage);
	virtual bool hasTabPage(ITabPage *APage) const;
	virtual ITabPage *currentTabPage() const;
	virtual void setCurrentTabPage(ITabPage *APage);
	virtual void detachTabPage(ITabPage *APage);
	virtual void removeTabPage(ITabPage *APage);
	virtual void clear();
signals:
	void currentTabPageChanged(ITabPage *APage);
	void tabPageAdded(ITabPage *APage);
	void tabPageRemoved(ITabPage *APage);
	void tabPageDetached(ITabPage *APage);
	void windowChanged();
	void windowDestroyed();
protected:
	void initialize();
	void createActions();
	void saveWindowState();
	void loadWindowState();
	void updateWindow();
	void updateTab(int AIndex);
protected slots:
	void onTabChanged(int AIndex);
	void onTabMenuRequested(int AIndex);
	void onTabCloseRequested(int AIndex);
	void onTabPageShow();
	void onTabPageClose();
	void onTabPageChanged();
	void onTabPageDestroyed();
	void onTabPageNotifierChanged();
	void onTabPageNotifierActiveNotifyChanged(int ANotifyId);
	void onTabWindowAppended(const QUuid &AWindowId, const QString &AName);
	void onTabWindowNameChanged(const QUuid &AWindowId, const QString &AName);
	void onTabWindowDeleted(const QUuid &AWindowId);
	void onTabMenuActionTriggered(bool);
	void onWindowMenuActionTriggered(bool);
	void onOptionsChanged(const OptionsNode &ANode);
private:
	Ui::TabWindowClass ui;
private:
	IMessageWidgets *FMessageWidgets;
private:
	Menu *FWindowMenu;
	Menu *FJoinMenu;
	Action *FCloseTab;
	Action *FCloseAllTabs;
	Action *FNextTab;
	Action *FPrevTab;
	Action *FNewTab;
	Action *FDetachWindow;
	Action *FShowCloseButtons;
	Action *FSetAsDefault;
	Action *FRenameWindow;
	Action *FCloseWindow;
	Action *FDeleteWindow;
private:
	QUuid FWindowId;
	QString FLastClosedTab;
};

#endif // TABWINDOW_H
