#include "consoleplugin.h"

ConsolePlugin::ConsolePlugin()
{
	FPluginManager = NULL;
	FMainWindowPlugin = NULL;
#ifdef Q_WS_MAC
	FMacIntegration = NULL;
#endif
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
		FMainWindowPlugin = qobject_cast<IMainWindowPlugin *>(plugin->instance());

#ifdef Q_WS_MAC
	plugin = APluginManager->pluginInterface("IMacIntegration").value(0,NULL);
	if (plugin)
		FMacIntegration = qobject_cast<IMacIntegration *>(plugin->instance());
#endif

	return FMainWindowPlugin!=NULL;
}

bool ConsolePlugin::initObjects()
{
	if (FMainWindowPlugin)
	{
		Action *action = new Action(FMainWindowPlugin->mainWindow()->mainMenu());
		action->setText(tr("XML Console"));
		//action->setIcon(RSR_STORAGE_MENUICONS,MNI_CONSOLE);
		connect(action,SIGNAL(triggered(bool)),SLOT(onShowXMLConsole(bool)));
#ifdef Q_WS_MAC
		FMacIntegration->windowMenu()->addAction(action, 510);
#else
		FMainWindowPlugin->mainWindow()->mainMenu()->addAction(action,AG_MMENU_CONSOLE_SHOW,true);
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

void ConsolePlugin::onShowXMLConsole(bool)
{
	ConsoleWidget *widget = new ConsoleWidget(FPluginManager,NULL);
	FCleanupHandler.add(widget);
	widget->show();
}

Q_EXPORT_PLUGIN2(plg_console, ConsolePlugin)
