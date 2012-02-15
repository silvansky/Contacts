#include "consoleplugin.h"

ConsolePlugin::ConsolePlugin()
{
	FPluginManager = NULL;
	FMainWindowPlugin = NULL;
#ifdef Q_WS_MAC
	FMacIntegration = NULL;
#endif
	showConsoleShortcut = NULL;
}

ConsolePlugin::~ConsolePlugin()
{

}

void ConsolePlugin::pluginInfo(IPluginInfo *APluginInfo)
{
	APluginInfo->name = tr("Console");
	APluginInfo->description = tr("Allows to view XML stream between the client and server");
	APluginInfo->version = "1.0";
	APluginInfo->author = "Potapov S.A. aka Lion";
	APluginInfo->homePage = "http://contacts.rambler.ru";
	APluginInfo->dependences.append(XMPPSTREAMS_UUID);
	APluginInfo->dependences.append(MAINWINDOW_UUID);
#ifdef Q_WS_MAC
	APluginInfo->dependences.append(MACINTEGRATION_UUID);
#endif
}

bool ConsolePlugin::initConnections(IPluginManager *APluginManager, int &/*AInitOrder*/)
{
	FPluginManager = APluginManager;

	IPlugin *plugin = APluginManager->pluginInterface("IMainWindowPlugin").value(0,NULL);
	if (plugin)
	{
		FMainWindowPlugin = qobject_cast<IMainWindowPlugin *>(plugin->instance());
	}

#ifdef Q_WS_MAC
	plugin = APluginManager->pluginInterface("IMacIntegration").value(0,NULL);
	if (plugin)
		FMacIntegration = qobject_cast<IMacIntegration *>(plugin->instance());
#endif

	return
		FMainWindowPlugin
#ifdef Q_WS_MAC
		&& FMacIntegration
#endif
			;
}

bool ConsolePlugin::initObjects()
{
	if (FMainWindowPlugin)
	{
		showConsoleShortcut = new QShortcut(FMainWindowPlugin->mainWindow()->instance());
		showConsoleShortcut->setKey(QKeySequence("Ctrl+Alt+Shift+C"));
		showConsoleShortcut->setEnabled(true);
		connect(showConsoleShortcut, SIGNAL(activated()), SLOT(onShowXMLConsole()));
#ifdef DEBUG_ENABLED
		Action *action = new Action(FMainWindowPlugin->mainWindow()->mainMenu());
		action->setText(tr("XML Console"));
		connect(action,SIGNAL(triggered()),SLOT(onShowXMLConsole()));
# ifdef Q_WS_MAC
		if (FMacIntegration)
			FMacIntegration->windowMenu()->addAction(action, 510);
# else
		FMainWindowPlugin->mainWindow()->mainMenu()->addAction(action,AG_MMENU_CONSOLE_SHOW,true);
# endif
#endif
	}
	return true;
}

bool ConsolePlugin::initSettings()
{
	Options::setDefaultValue(OPV_CONSOLE_CONTEXT_NAME,tr("Default Context"));
	Options::setDefaultValue(OPV_CONSOLE_CONTEXT_WORDWRAP,false);
	Options::setDefaultValue(OPV_CONSOLE_CONTEXT_HIGHLIGHTXML,Qt::Checked);
	return true;
}

void ConsolePlugin::onShowXMLConsole()
{
	ConsoleWidget *widget = new ConsoleWidget(FPluginManager,NULL);
	FCleanupHandler.add(widget);
	widget->show();
}

Q_EXPORT_PLUGIN2(plg_console, ConsolePlugin)
