#ifndef MAINWINDOWPLUGIN_H
#define MAINWINDOWPLUGIN_H

#include <QTime>
#include <definitions/actiongroups.h>
#include <definitions/version.h>
#include <definitions/resources.h>
#include <definitions/menuicons.h>
#include <definitions/optionnodes.h>
#include <definitions/optionwidgetorders.h>
#include <definitions/optionvalues.h>
#include <interfaces/ipluginmanager.h>
#include <interfaces/imainwindow.h>
#include <interfaces/ioptionsmanager.h>
#include <interfaces/itraymanager.h>
#include <utils/widgetmanager.h>
#include <utils/action.h>
#include <utils/options.h>
#include <utils/customborderstorage.h>
#include "mainwindow.h"

class MainWindowPlugin :
	public QObject,
	public IPlugin,
	public IOptionsHolder,
	public IMainWindowPlugin
{
	Q_OBJECT
	Q_INTERFACES(IPlugin IOptionsHolder IMainWindowPlugin)
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
	virtual CustomBorderContainer * mainWindowBorder() const;
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
	void onMainWindowClosed();
	void onShutdownStarted();
private:
	IPluginManager *FPluginManager;
	IOptionsManager *FOptionsManager;
	ITrayManager *FTrayManager;
private:
	Action *FOpenAction;
	MainWindow *FMainWindow;
	CustomBorderContainer *FMainWindowBorder;
	QTime FActivationChanged;
};

#endif // MAINWINDOW_H
