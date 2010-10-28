#ifndef IMAINWINDOW_H
#define IMAINWINDOW_H

#include <QToolBar>
#include <QVBoxLayout>
#include <QMainWindow>
#include <QStackedWidget>
#include <utils/menu.h>
#include <utils/toolbarchanger.h>

#define MAINWINDOW_UUID "{a11424f5-279b-4e53-a1eb-ca938683c2ab}"

struct IInternalNotice
{
	IInternalNotice() {
		priority = -1;
	}
	int priority;
	QIcon icon;
	QString iconKey;
	QString iconStorage;
	QString caption;
	QString message;
	QList<Action *> actions;
};

class IInternalNoticeWidget 
{
public:
	virtual QWidget *instance() = 0;
	virtual bool isEmpty() const =0;
	virtual int activeNotice() const =0;
	virtual QList<int> noticeQueue() const =0;
	virtual IInternalNotice noticeById(int ANoticeId) const =0;
	virtual int insertNotice(const IInternalNotice &ANotice) =0;
	virtual void removeNotice(int ANoticeId) =0;
protected:
	virtual void noticeWidgetReady() =0;
	virtual void noticeInserted(int ANoticeId) =0;
	virtual void noticeActivated(int ANoticeId) =0;
	virtual void noticeRemoved(int ANoticeId) =0;
};

class IMainWindow
{
public:
	virtual QMainWindow *instance() =0;
	virtual Menu *mainMenu() const = 0;
	virtual QVBoxLayout *mainLayout() const =0;
	virtual QStackedWidget *upperWidget() const = 0;
	virtual QStackedWidget *rostersWidget() const = 0;
	virtual QStackedWidget *bottomWidget() const = 0;
	virtual IInternalNoticeWidget *noticeWidget() const =0;
	virtual ToolBarChanger *topToolBarChanger() const =0;
	virtual ToolBarChanger *leftToolBarChanger() const =0;
	virtual ToolBarChanger *statusToolBarChanger() const =0;
};

class IMainWindowPlugin
{
public:
	virtual QObject *instance() = 0;
	virtual IMainWindow *mainWindow() const = 0;
	virtual void showMainWindow() const =0;
};

Q_DECLARE_INTERFACE(IInternalNoticeWidget,"Virtus.Plugin.IInternalNoticeWidget/1.0")
Q_DECLARE_INTERFACE(IMainWindow,"Virtus.Plugin.IMainWindow/1.0")
Q_DECLARE_INTERFACE(IMainWindowPlugin,"Virtus.Plugin.IMainWindowPlugin/1.0")

#endif
