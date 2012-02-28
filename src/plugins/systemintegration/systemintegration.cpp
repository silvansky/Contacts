#include "systemintegration.h"

#include <interfaces/imacintegration.h>
#include <utils/log.h>

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
	Q_UNUSED(AInitOrder)
	if (APluginManager)
	{
		connect(APluginManager->instance(), SIGNAL(aboutToQuit()), SLOT(onAboutToQuit()));
#ifdef Q_WS_MAC
		IPlugin *plugin = APluginManager->pluginInterface("IMacIntegration").value(0,NULL);
		if (plugin)
		{
			IMacIntegration * macIntegration = qobject_cast<IMacIntegration *>(plugin->instance());
			if ((impl = qobject_cast<ISystemIntegrationImplementation*>(macIntegration->instance())))
			{
				LogDetail("[SystemIntegration]: Loaded Mac OS X Integration plugin");
				plugin->initConnections(APluginManager, AInitOrder);
				impl->init();
			}
			else
			{
				LogError("[SystemIntegration]: Failed to load Mac OS X Integration plugin! Interface ISystemIntegrationImplementation not implemented");
			}
		}
		else
			LogError("[SystemIntegration]: Failed to load Mac OS X Integration plugin! IMacIntegration plugin not found");
#endif
	}
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
	switch (role)
	{
	case ApplicationRole:
		action->setMenuRole(QAction::ApplicationSpecificRole);
		if (impl && impl->fileMenu())
			impl->fileMenu()->addAction(action, group);
		break;
	case SettingsRole:
		action->setMenuRole(QAction::PreferencesRole);
		if (impl && impl->fileMenu())
			impl->fileMenu()->addAction(action, group);
		break;
	case FileRole:
		if (impl && impl->fileMenu())
			impl->fileMenu()->addAction(action, group);
		break;
	case EditRole:
		if (impl && impl->editMenu())
			impl->editMenu()->addAction(action, group);
		break;
	case ViewRole:
		if (impl && impl->viewMenu())
			impl->viewMenu()->addAction(action, group);
		break;
	case StatusRole:
		if (impl && impl->statusMenu())
			impl->statusMenu()->addAction(action, group);
		break;
	case WindowRole:
		if (impl && impl->windowMenu())
			impl->windowMenu()->addAction(action, group);
		break;
	case HelpRole:
		if (impl && impl->helpMenu())
			impl->helpMenu()->addAction(action, group);
		break;
	case DockRole:
		if (impl && impl->dockMenu())
			impl->dockMenu()->addAction(action, group);
		break;
	default:
		break;
	}
}

void SystemIntegration::removeAction(MenuActionRole role, Action * action)
{
	switch (role)
	{
	case ApplicationRole:
		action->setMenuRole(QAction::ApplicationSpecificRole);
		if (impl && impl->fileMenu())
			impl->fileMenu()->removeAction(action);
		break;
	case SettingsRole:
		action->setMenuRole(QAction::PreferencesRole);
		if (impl && impl->fileMenu())
			impl->fileMenu()->removeAction(action);
		break;
	case FileRole:
		if (impl && impl->fileMenu())
			impl->fileMenu()->removeAction(action);
		break;
	case EditRole:
		if (impl && impl->editMenu())
			impl->editMenu()->removeAction(action);
		break;
	case ViewRole:
		if (impl && impl->viewMenu())
			impl->viewMenu()->removeAction(action);
		break;
	case StatusRole:
		if (impl && impl->statusMenu())
			impl->statusMenu()->removeAction(action);
		break;
	case WindowRole:
		if (impl && impl->windowMenu())
			impl->windowMenu()->removeAction(action);
		break;
	case HelpRole:
		if (impl && impl->helpMenu())
			impl->helpMenu()->removeAction(action);
		break;
	case DockRole:
		if (impl && impl->dockMenu())
			impl->dockMenu()->removeAction(action);
		break;
	default:
		break;
	}

}

bool SystemIntegration::isDockPresent() const
{
	return (impl && impl->isDockPresent());
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

bool SystemIntegration::isRequestUserAttentionPresent() const
{
	return (impl && impl->isRequestUserAttentionPresent());
}

void SystemIntegration::requestUserAttention()
{
	if (impl)
		impl->requestUserAttention();
}

bool SystemIntegration::isSystemNotificationsAccessible() const
{
	return (impl && impl->isSystemNotificationsAccessible());
}

QString SystemIntegration::systemNotificationsSystemName() const
{
	return impl->systemNotificationsSystemName();
}

bool SystemIntegration::isSystemNotificationsSettingsAccessible() const
{
	return (impl && impl->isSystemNotificationsSettingsAccessible());
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

void SystemIntegration::onAboutToQuit()
{
	if (impl)
		impl->finalize();
}

Q_EXPORT_PLUGIN2(plg_systemintegration, SystemIntegration)
