#include "macintegrationplugin.h"

#include "macintegration_p.h"

extern void qt_mac_set_dock_menu(QMenu *); // Qt internal function

MacIntegrationPlugin::MacIntegrationPlugin()
{
}

void MacIntegrationPlugin::pluginInfo(IPluginInfo *APluginInfo)
{
	APluginInfo->name = tr("Mac OS X Integration");
	APluginInfo->description = tr("Allow integration in Mac OS X Dock and Menu Bar");
	APluginInfo->version = "1.0";
	APluginInfo->author = "Valentine Gorshkov aka Silvansky";
	APluginInfo->homePage = "http://contacts.rambler.ru";
}

bool MacIntegrationPlugin::initConnections(IPluginManager *APluginManager, int &AInitOrder)
{
	Q_UNUSED(APluginManager)
	Q_UNUSED(AInitOrder)
	return true;
}

bool MacIntegrationPlugin::initObjects()
{
	_dockMenu = new Menu();
	_menuBar = new QMenuBar(NULL); // this must be the first parentless QMenuBar

	qt_mac_set_dock_menu(_dockMenu); // setting dock menu

	connect(MacIntegrationPrivate::instance(), SIGNAL(dockClicked()), SIGNAL(dockClicked()));

	return true;
}

Menu * MacIntegrationPlugin::dockMenu()
{
	return _dockMenu;
}

QMenuBar * MacIntegrationPlugin::menuBar()
{
	return _menuBar;
}

Q_EXPORT_PLUGIN2(plg_macintegration, MacIntegrationPlugin)

