#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <interfaces/imainwindow.h>
#include <definitions/resources.h>
#include <definitions/menuicons.h>
#include <definitions/stylesheets.h>
#include <definitions/toolbargroups.h>
#include <utils/stylestorage.h>
#include "noticewidget.h"

class MainWindow :
			public QMainWindow,
			public IMainWindow
{
	Q_OBJECT;
	Q_INTERFACES(IMainWindow);
public:
	MainWindow(QWidget *AParent = NULL, Qt::WindowFlags AFlags = 0);
	~MainWindow();
	//IMainWindow
	virtual QMainWindow *instance() { return this; }
	virtual bool isActive() const;
	virtual Menu *mainMenu() const;
	virtual QVBoxLayout *mainLayout() const;
	virtual QStackedWidget *upperWidget() const;
	virtual QStackedWidget *rostersWidget() const;
	virtual QStackedWidget *bottomWidget() const;
	virtual IInternalNoticeWidget *noticeWidget() const;
	virtual ToolBarChanger *topToolBarChanger() const;
	virtual ToolBarChanger *leftToolBarChanger() const;
	virtual ToolBarChanger *statusToolBarChanger() const;
public:
	virtual QMenu *createPopupMenu();
protected:
	void createLayouts();
	void createToolBars();
	void createMenus();
protected:
	void keyPressEvent(QKeyEvent *AEvent);
protected slots:
	void onStackedWidgetChanged(int AIndex);
	void onInternalNoticeChanged(int ANoticeId);
private:
	Menu *FMainMenu;
	ToolBarChanger *FTopToolBarChanger;
	ToolBarChanger *FLeftToolBarChanger;
	ToolBarChanger *FStatusToolBarChanger;
private:
	QVBoxLayout *FMainLayout;
	QStackedWidget *FUpperWidget;
	QStackedWidget *FRostersWidget;
	QStackedWidget *FBottomWidget;
	InternalNoticeWidget *FNoticeWidget;
};

#endif // MAINWINDOW_H
