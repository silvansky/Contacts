#include "consoleplugin.h"

ConsolePlugin::ConsolePlugin()
{
	FPluginManager = NULL;
	FMainWindowPlugin = NULL;
	FSystemIntegration = NULL;
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
	APluginInfo->dependences.append(SYSTEMINTEGRATION_UUID);
}

bool ConsolePlugin::initConnections(IPluginManager *APluginManager, int &/*AInitOrder*/)
{
	FPluginManager = APluginManager;

	IPlugin *plugin = APluginManager->pluginInterface("IMainWindowPlugin").value(0,NULL);
	if (plugin)
	{
		FMainWindowPlugin = qobject_cast<IMainWindowPlugin *>(plugin->instance());
	}

	plugin = APluginManager->pluginInterface("ISystemIntegration").value(0,NULL);
	if (plugin)
		FSystemIntegration = qobject_cast<ISystemIntegration *>(plugin->instance());

	return FMainWindowPlugin && FSystemIntegration;
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
		if (FSystemIntegration && FSystemIntegration->isGlobalMenuPresent())
			FSystemIntegration->addAction(ISystemIntegration::WindowRole, action, 510);
		else
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

void ConsolePlugin::onShowXMLConsole()
{
	ConsoleWidget *widget = new ConsoleWidget(FPluginManager,NULL);
	FCleanupHandler.add(widget->window());
	widget->window()->show();
}

Q_EXPORT_PLUGIN2(plg_console, ConsolePlugin)
