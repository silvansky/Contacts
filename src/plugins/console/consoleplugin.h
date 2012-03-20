#ifndef CONSOLEPLUGIN_H
#define CONSOLEPLUGIN_H

#include <QObjectCleanupHandler>
#include <QShortcut>
#include <definitions/actiongroups.h>
#include <definitions/resources.h>
#include <definitions/menuicons.h>
#include <interfaces/ipluginmanager.h>
#include <interfaces/ixmppstreams.h>
#include <interfaces/imainwindow.h>
#include <interfaces/isystemintegration.h>
#include <utils/iconstorage.h>
#include <utils/action.h>
#include "consolewidget.h"

#define CONSOLE_UUID  "{89daf2bb-29bb-49b7-94df-8134452d1103}"

class ConsolePlugin :
			public QObject,
			public IPlugin
{
    Q_OBJECT
    Q_INTERFACES(IPlugin)
public:
	ConsolePlugin();
	~ConsolePlugin();
	//IPlugin
	virtual QObject *instance() { return this; }
	virtual QUuid pluginUuid() const { return CONSOLE_UUID; }
	virtual void pluginInfo(IPluginInfo *APluginInfo);
	virtual bool initConnections(IPluginManager *APluginManager, int &AInitOrder);
	virtual bool initObjects();
	virtual bool initSettings();
	virtual bool startPlugin() { return true; }
protected slots:
	void onShowXMLConsole();
private:
	IPluginManager *FPluginManager;
	IMainWindowPlugin *FMainWindowPlugin;
	ISystemIntegration * FSystemIntegration;
private:
	QObjectCleanupHandler FCleanupHandler;
	QShortcut * showConsoleShortcut;
};

#endif // CONSOLEPLUGIN_H
