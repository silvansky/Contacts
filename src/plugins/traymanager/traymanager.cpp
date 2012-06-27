#include "traymanager.h"

#include <QSysInfo>
#include <QApplication>

#define BLINK_VISIBLE_TIME      750
#define BLINK_INVISIBLE_TIME    250

TrayManager::TrayManager()
{
	FPluginManager = NULL;
	FMainWindowPlugin = NULL;

	FActiveNotify = -1;
	FIconHidden = false;

	QPixmap empty(16,16);
	empty.fill(Qt::transparent);
	FEmptyIcon.addPixmap(empty);

	FContextMenu = new Menu;
	FSystemIcon.setContextMenu(FContextMenu);
	FSystemIcon.setIcon(IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getIcon(MNI_MAINWINDOW_LOGO16));

	FBlinkTimer.setSingleShot(true);
	connect(&FBlinkTimer,SIGNAL(timeout()),SLOT(onBlinkTimerTimeout()));

	FTriggerTimer.setSingleShot(true);
	connect(&FTriggerTimer,SIGNAL(timeout()),SLOT(onTriggerTimerTimeout()));

	connect(&FSystemIcon,SIGNAL(messageClicked()), SIGNAL(messageClicked()));
	connect(&FSystemIcon,SIGNAL(activated(QSystemTrayIcon::ActivationReason)), SLOT(onTrayIconActivated(QSystemTrayIcon::ActivationReason)));
}

TrayManager::~TrayManager()
{
	FSystemIcon.hide();
	while (FNotifyOrder.count() > 0)
		removeNotify(FNotifyOrder.first());
	delete FContextMenu;
}

void TrayManager::pluginInfo(IPluginInfo *APluginInfo)
{
	APluginInfo->name = tr("Tray Icon");
	APluginInfo->description = tr("Allows other modules to access the icon and context menu in the tray");
	APluginInfo->version = "1.0";
	APluginInfo->author = "Potapov S.A. aka Lion";
	APluginInfo->homePage = "http://contacts.rambler.ru";
}

bool TrayManager::initConnections(IPluginManager *APluginManager, int &AInitOrder)
{
	Q_UNUSED(AInitOrder);
	FPluginManager = APluginManager;

	IPlugin *plugin = FPluginManager->pluginInterface("IMainWindowPlugin").value(0,NULL);
	if (plugin)
	{
		FMainWindowPlugin = qobject_cast<IMainWindowPlugin *>(plugin->instance());
	}

	connect(FPluginManager->instance(),SIGNAL(shutdownStarted()),SLOT(onShutdownStarted()));

	connect(Options::instance(),SIGNAL(optionsOpened()),SLOT(onOptionsOpened()));
	connect(Options::instance(),SIGNAL(optionsChanged(const OptionsNode &)),SLOT(onOptionsChanged(const OptionsNode &)));

	return true;
}

bool TrayManager::initObjects()
{
	Action *action = new Action(FContextMenu);
	action->setText(tr("Exit"));
	connect(action,SIGNAL(triggered()),FPluginManager->instance(),SLOT(quit()));
	FContextMenu->addAction(action,AG_TMTM_TRAYMANAGER_QUIT);
	return true;
}

bool TrayManager::startPlugin()
{
	updateTrayVisibility();
	return true;
}

QRect TrayManager::geometry() const
{
	return FSystemIcon.geometry();
}

Menu *TrayManager::contextMenu() const
{
	return FContextMenu;
}

QIcon TrayManager::icon() const
{
	return FIcon;
}

void TrayManager::setIcon(const QIcon &AIcon)
{
	FIcon = AIcon;
	if (FActiveNotify < 0)
		FSystemIcon.setIcon(FIcon);
	else
		updateTray();
}

QString TrayManager::toolTip() const
{
	return FToolTip;
}

void TrayManager::setToolTip(const QString &AToolTip)
{
	FToolTip = AToolTip;
	if (FActiveNotify < 0)
		FSystemIcon.setToolTip(AToolTip);
	else
		updateTray();
}

int TrayManager::activeNotify() const
{
	return FActiveNotify;
}

QList<int> TrayManager::notifies() const
{
	return FNotifyOrder;
}

ITrayNotify TrayManager::notifyById(int ANotifyId) const
{
	return FNotifyItems.value(ANotifyId);
}

int TrayManager::appendNotify(const ITrayNotify &ANotify)
{
	int notifyId = qrand();
	while (notifyId<=0 || FNotifyItems.contains(notifyId))
		notifyId = qrand();
	FNotifyOrder.append(notifyId);
	FNotifyItems.insert(notifyId,ANotify);
	updateTray();
	emit notifyAppended(notifyId);
	return notifyId;
}

void TrayManager::removeNotify(int ANotifyId)
{
	if (FNotifyItems.contains(ANotifyId))
	{
		FNotifyItems.remove(ANotifyId);
		FNotifyOrder.removeAll(ANotifyId);
		updateTray();
		emit notifyRemoved(ANotifyId);
	}
}

void TrayManager::showMessage(const QString &ATitle, const QString &AMessage, QSystemTrayIcon::MessageIcon AIcon, int ATimeout)
{
	FSystemIcon.showMessage(ATitle,AMessage,AIcon,ATimeout);
	emit messageShown(ATitle,AMessage,AIcon,ATimeout);
}

void TrayManager::updateTray()
{
	QString trayToolTip;
	for (int i=0; i<10 && i<FNotifyOrder.count(); i++)
	{
		QString notifyToolTip = FNotifyItems.value(FNotifyOrder.at(i)).toolTip;
		if (!notifyToolTip.isEmpty())
			trayToolTip += notifyToolTip + '\n';
	}
	if (trayToolTip.isEmpty())
		trayToolTip = FToolTip;
	else
		trayToolTip.chop(1);
	FSystemIcon.setToolTip(trayToolTip);

	int notifyId = !FNotifyOrder.isEmpty() ? FNotifyOrder.last() : -1;
	if (notifyId != FActiveNotify)
	{
		FIconHidden = false;
		FBlinkTimer.stop();
		FActiveNotify = notifyId;

		if (FActiveNotify > 0)
		{
			const ITrayNotify &notify = FNotifyItems.value(notifyId);
			if (notify.blink)
				FBlinkTimer.start(BLINK_VISIBLE_TIME);
			if (!notify.iconKey.isEmpty() && !notify.iconStorage.isEmpty())
				IconStorage::staticStorage(notify.iconStorage)->insertAutoIcon(&FSystemIcon,notify.iconKey);
			else
				FSystemIcon.setIcon(notify.icon);
		}
		else
		{
			FSystemIcon.setIcon(FIcon);
		}

		emit activeNotifyChanged(notifyId);
	}
	updateTrayVisibility();
}

void TrayManager::updateTrayVisibility()
{
#ifndef Q_WS_MAC
	if (FMainWindowPlugin && !FMainWindowPlugin->isMinimizeToTray() && FNotifyItems.isEmpty())
		FSystemIcon.hide();
	else if (!FSystemIcon.isVisible())
		FSystemIcon.show();
#endif
}

void TrayManager::onTrayIconActivated(QSystemTrayIcon::ActivationReason AReason)
{
	if (AReason != QSystemTrayIcon::Trigger)
	{
		if (VersionParser(qVersion()) >= VersionParser("4.6.0"))
			FTriggerTimer.stop();
		emit notifyActivated(FActiveNotify,AReason);
	}
	else if (!FTriggerTimer.isActive())
	{
		FTriggerTimer.start(qApp->doubleClickInterval());
	}
	else
	{
		FTriggerTimer.stop();
	}
}

void TrayManager::onBlinkTimerTimeout()
{
	const ITrayNotify &notify = FNotifyItems.value(FActiveNotify);
	if (FIconHidden)
	{
		if (!notify.iconKey.isEmpty() && !notify.iconStorage.isEmpty())
			IconStorage::staticStorage(notify.iconStorage)->insertAutoIcon(&FSystemIcon,notify.iconKey);
		else
			FSystemIcon.setIcon(notify.icon);
		FBlinkTimer.start(BLINK_VISIBLE_TIME);
	}
	else
	{
		IconStorage::staticStorage(notify.iconStorage)->removeAutoIcon(&FSystemIcon);
		FSystemIcon.setIcon(FEmptyIcon);
		FBlinkTimer.start(BLINK_INVISIBLE_TIME);
	}
	FIconHidden = !FIconHidden;
}

void TrayManager::onTriggerTimerTimeout()
{
	emit notifyActivated(FActiveNotify,QSystemTrayIcon::Trigger);
}

void TrayManager::onShutdownStarted()
{
	FSystemIcon.hide();
}

void TrayManager::onOptionsOpened()
{
	updateTrayVisibility();
}

void TrayManager::onOptionsChanged(const OptionsNode &ANode)
{
	Q_UNUSED(ANode);
	updateTrayVisibility();
}

Q_EXPORT_PLUGIN2(plg_traymanager, TrayManager)
