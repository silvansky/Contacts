#ifndef MAINWINDOWPLUGIN_H
#define MAINWINDOWPLUGIN_H

#include <QTime>
#include <definations/actiongroups.h>
#include <definations/version.h>
#include <definations/resources.h>
#include <definations/menuicons.h>
#include <definations/optionnodes.h>
#include <definations/optionwidgetorders.h>
#include <definations/optionvalues.h>
#include <interfaces/ipluginmanager.h>
#include <interfaces/imainwindow.h>
#include <interfaces/ioptionsmanager.h>
#include <interfaces/itraymanager.h>
#include <utils/widgetmanager.h>
#include <utils/action.h>
#include <utils/options.h>
#include "mainwindow.h"

class MainWindowPlugin :
			public QObject,
			public IPlugin,
			public IOptionsHolder,
			public IMainWindowPlugin
{
	Q_OBJECT;
	Q_INTERFACES(IPlugin IOptionsHolder IMainWindowPlugin);
public:
	MainWindowPlugin();
	~MainWindowPlugin();
	virtual QObject* instance() { return this; }
	//IPlugin
	virtual QUuid pluginUuid() const { return MAINWINDOW_UUID; }
	virtual void pluginInfo(IPluginInfo *APluginInfo);
	virtual bool initConnections(IPluginManager *APluginManager, int &AInitOrder);
	virtual bool initObjects();
	virtual bool initSettings();
	virtual bool startPlugin();
	//IOptionsHolder
	virtual QMultiMap<int, IOptionsWidget *> optionsWidgets(const QString &ANodeId, QWidget *AParent);
	//IMainWindowPlugin
	virtual IMainWindow *mainWindow() const;
	virtual void showMainWindow() const;
protected:
	void updateTitle();
	void correctWindowPosition() const;
protected:
	bool eventFilter(QObject *AWatched, QEvent *AEvent);
protected slots:
	void onOptionsOpened();
	void onOptionsClosed();
	void onOptionsChanged(const OptionsNode &ANode);
	void onProfileRenamed(const QString &AProfile, const QString &ANewName);
	void onTrayNotifyActivated(int ANotifyId, QSystemTrayIcon::ActivationReason AReason);
	void onShowMainWindowByAction(bool);
private:
	IPluginManager *FPluginManager;
	IOptionsManager *FOptionsManager;
	ITrayManager *FTrayManager;
private:
	Action *FOpenAction;
	MainWindow *FMainWindow;
	QTime FActivationChanged;
};

#endif // MAINWINDOW_H
