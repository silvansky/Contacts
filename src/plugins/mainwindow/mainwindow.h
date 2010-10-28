#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <interfaces/imainwindow.h>
#include <definations/resources.h>
#include <definations/menuicons.h>
#include <definations/stylesheets.h>
#include <definations/toolbargroups.h>
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
	virtual Menu *mainMenu() const { return FMainMenu; }
	virtual QVBoxLayout *mainLayout() const { return FMainLayout; }
	virtual QStackedWidget *upperWidget() const { return FUpperWidget; }
	virtual QStackedWidget *rostersWidget() const { return FRostersWidget; }
	virtual QStackedWidget *bottomWidget() const { return FBottomWidget; }
	virtual IInternalNoticeWidget *noticeWidget() const {return FNoticeWidget; }
	virtual ToolBarChanger *topToolBarChanger() const { return FTopToolBarChanger; }
	virtual ToolBarChanger *leftToolBarChanger() const { return FLeftToolBarChanger; }
	virtual ToolBarChanger *statusToolBarChanger() const { return FStatusToolBarChanger; }
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
