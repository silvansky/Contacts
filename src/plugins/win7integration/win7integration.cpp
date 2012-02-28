#include "win7integration.h"

Win7Integration::Win7Integration()
{

}

Win7Integration::~Win7Integration()
{

}

void Win7Integration::pluginInfo(IPluginInfo *APluginInfo)
{
	APluginInfo->name = tr("Windows 7 Integration");
	APluginInfo->description = tr("Allow integration in Windows 7 environment");
	APluginInfo->version = "1.0";
	APluginInfo->author = "Valentine Gorshkov aka Silvansky";
	APluginInfo->homePage = "http://contacts.rambler.ru";
}

Q_EXPORT_PLUGIN2(plg_win7integration, Win7Integration)
