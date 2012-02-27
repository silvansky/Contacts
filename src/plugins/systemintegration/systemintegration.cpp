#include "systemintegration.h"

SystemIntegration::SystemIntegration()
{
	impl = NULL;
}

SystemIntegration::~SystemIntegration()
{}

void SystemIntegration::pluginInfo(IPluginInfo *APluginInfo)
{
	APluginInfo->name = tr("System Integration");
	APluginInfo->description = tr("Allow integration in Operating System environment");
	APluginInfo->version = "1.0";
	APluginInfo->author = "Valentine Gorshkov aka Silvansky";
	APluginInfo->homePage = "http://contacts.rambler.ru";
}

bool SystemIntegration::initConnections(IPluginManager *APluginManager, int &AInitOrder)
{
	// TODO: load implementation plugin
	return true;
}

bool SystemIntegration::initObjects()
{
	return true;
}

bool SystemIntegration::initSettings()
{
	return true;
}

bool SystemIntegration::startPlugin()
{
	return true;
}

bool SystemIntegration::isGlobalMenuPresent() const
{
	return impl ? impl->isGlobalMenuPresent() : false;
}

bool SystemIntegration::isDockMenuPresent() const
{
	return impl ? impl->isDockMenuPresent() : false;
}

void SystemIntegration::addAction(MenuActionRole role, Action * action, int group)
{
	// TODO
}

void SystemIntegration::removeAction(Action * action)
{
	// TODO
}

bool SystemIntegration::isDockPresent() const
{
	return impl ? impl->isDockPresent() : false;
}

void SystemIntegration::setDockBadge(const QString & badge)
{
	if (impl)
		impl->setDockBadge(badge);
}

void SystemIntegration::setDockOverlayImage(const QImage & image, Qt::Alignment alignment, bool showAppIcon)
{
	if (impl)
		impl->setDockOverlayImage(image, alignment, showAppIcon);
}

void SystemIntegration::requestUserAttention()
{
	if (impl)
		impl->requestUserAttention();
}

bool SystemIntegration::isSystemNotificationsSettingsAccessible() const
{
	return impl ? impl->isSystemNotificationsSettingsAccessible() : false;
}

void SystemIntegration::postSystemNotify(const QImage & icon, const QString & title, const QString & text, const QString & type, int id)
{
	if (impl)
		impl->postSystemNotify(icon, title, text, type, id);
}

void SystemIntegration::showSystemNotificationsSettings()
{
	if (impl)
		impl->showSystemNotificationsSettings();
}

Q_EXPORT_PLUGIN2(plg_systemintegration, SystemIntegration)
