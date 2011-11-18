#include "statuschanger.h"

#include <stdlib.h>

#include <QTimer>
#include <QSysInfo>
#include <QToolButton>
#include <definitions/resources.h>
#include <definitions/graphicseffects.h>
#include <utils/imagemanager.h>
#include <utils/graphicseffectsstorage.h>

#define MAX_TEMP_STATUS_ID                  -10
#define MAX_CUSTOM_STATUS_PER_SHOW          3

#define ADR_STREAMJID                       Action::DR_StreamJid
#define ADR_STATUS_CODE                     Action::DR_Parametr1

#define MAX_RECON_STEP                      2

static const struct {int base; int random; } ReconSteps[] = {
	{ 5, 5 },
	{ 10, 20 },
	{ 30, 60 }
};

StatusChanger::StatusChanger()
{
	FPluginManager = NULL;
	FPresencePlugin = NULL;
	FRosterPlugin = NULL;
	FMainWindowPlugin = NULL;
	FRostersView = NULL;
	FRostersModel = NULL;
	FTrayManager = NULL;
	FOptionsManager = NULL;
	FAccountManager = NULL;
	FNotifications = NULL;
	FVCardPlugin = NULL;
	FAvatars = NULL;
	FStatusIcons = NULL;

	FStatusMenu = NULL;
	FStatusWidget = NULL;
	FChangingPresence = NULL;
}

StatusChanger::~StatusChanger()
{
	delete FStatusMenu;
}

//IPlugin
void StatusChanger::pluginInfo(IPluginInfo *APluginInfo)
{
	APluginInfo->name = tr("Status Manager");
	APluginInfo->description = tr("Allows to change the status in Jabber network");
	APluginInfo->version = "1.0";
	APluginInfo->author = "Potapov S.A. aka Lion";
	APluginInfo->homePage = "http://contacts.rambler.ru";
	APluginInfo->dependences.append(PRESENCE_UUID);
}

bool StatusChanger::initConnections(IPluginManager *APluginManager, int &AInitOrder)
{
	AInitOrder = PIO_STATUSCHANGER;
	FPluginManager = APluginManager;

	IPlugin *plugin = APluginManager->pluginInterface("IPresencePlugin").value(0,NULL);
	if (plugin)
	{
		FPresencePlugin = qobject_cast<IPresencePlugin *>(plugin->instance());
		if (FPresencePlugin)
		{
			connect(FPresencePlugin->instance(),SIGNAL(presenceAdded(IPresence *)),
				SLOT(onPresenceAdded(IPresence *)));
			connect(FPresencePlugin->instance(),SIGNAL(presenceChanged(IPresence *, int, const QString &, int)),
				SLOT(onPresenceChanged(IPresence *, int, const QString &, int)));
			connect(FPresencePlugin->instance(),SIGNAL(presenceRemoved(IPresence *)),
				SLOT(onPresenceRemoved(IPresence *)));
		}
	}

	plugin = APluginManager->pluginInterface("IRosterPlugin").value(0,NULL);
	if (plugin)
	{
		FRosterPlugin = qobject_cast<IRosterPlugin *>(plugin->instance());
		if (FRosterPlugin)
		{
			connect(FRosterPlugin->instance(),SIGNAL(rosterOpened(IRoster *)),SLOT(onRosterOpened(IRoster *)));
			connect(FRosterPlugin->instance(),SIGNAL(rosterClosed(IRoster *)),SLOT(onRosterClosed(IRoster *)));
		}
	}

	plugin = APluginManager->pluginInterface("IMainWindowPlugin").value(0,NULL);
	if (plugin)
		FMainWindowPlugin = qobject_cast<IMainWindowPlugin *>(plugin->instance());

	plugin = APluginManager->pluginInterface("IRostersViewPlugin").value(0,NULL);
	if (plugin)
	{
		IRostersViewPlugin *rostersViewPlugin = qobject_cast<IRostersViewPlugin *>(plugin->instance());
		if (rostersViewPlugin)
		{
			FRostersView = rostersViewPlugin->rostersView();
		}
	}

	plugin = APluginManager->pluginInterface("IRostersModel").value(0,NULL);
	if (plugin)
	{
		FRostersModel = qobject_cast<IRostersModel *>(plugin->instance());
		if (FRostersModel)
		{
			connect(FRostersModel->instance(),SIGNAL(streamJidChanged(const Jid &, const Jid &)),
				SLOT(onStreamJidChanged(const Jid &, const Jid &)));
		}
	}

	plugin = APluginManager->pluginInterface("IAccountManager").value(0,NULL);
	if (plugin)
		FAccountManager = qobject_cast<IAccountManager *>(plugin->instance());

	plugin = APluginManager->pluginInterface("ITrayManager").value(0,NULL);
	if (plugin)
	{
		FTrayManager = qobject_cast<ITrayManager *>(plugin->instance());
		if (FTrayManager)
		{
			connect(FTrayManager->contextMenu(),SIGNAL(aboutToShow()),SLOT(onTrayContextMenuAboutToShow()));
			connect(FTrayManager->contextMenu(),SIGNAL(aboutToHide()),SLOT(onTrayContextMenuAboutToHide()));
		}
	}

	plugin = APluginManager->pluginInterface("IOptionsManager").value(0,NULL);
	if (plugin)
	{
		FOptionsManager = qobject_cast<IOptionsManager *>(plugin->instance());
		if (FOptionsManager)
		{
			connect(FOptionsManager->instance(),SIGNAL(profileOpened(const QString &)),SLOT(onProfileOpened(const QString &)),Qt::QueuedConnection);
		}
	}

	plugin = APluginManager->pluginInterface("IStatusIcons").value(0,NULL);
	if (plugin)
	{
		FStatusIcons = qobject_cast<IStatusIcons *>(plugin->instance());
		if (FStatusIcons)
		{
			connect(FStatusIcons->instance(),SIGNAL(defaultIconsChanged()),SLOT(onDefaultStatusIconsChanged()));
		}
	}

	plugin = APluginManager->pluginInterface("INotifications").value(0,NULL);
	if (plugin)
	{
		FNotifications = qobject_cast<INotifications *>(plugin->instance());
		if (FNotifications)
		{
			connect(FNotifications->instance(),SIGNAL(notificationActivated(int)), SLOT(onNotificationActivated(int)));
		}
	}

	plugin = APluginManager->pluginInterface("IVCardPlugin").value(0, NULL);
	if (plugin)
		FVCardPlugin = qobject_cast<IVCardPlugin *>(plugin->instance());

	plugin = APluginManager->pluginInterface("IAvatars").value(0, NULL);
	if (plugin)
		FAvatars = qobject_cast<IAvatars *>(plugin->instance());

	connect(Options::instance(),SIGNAL(optionsOpened()),SLOT(onOptionsOpened()));
	connect(Options::instance(),SIGNAL(optionsClosed()),SLOT(onOptionsClosed()));
	connect(APluginManager->instance(),SIGNAL(shutdownStarted()),SLOT(onShutdownStarted()));

	return FPresencePlugin!=NULL;
}

bool StatusChanger::initObjects()
{
	FStatusMenu = new Menu;
	FStatusMenu->setObjectName("schangerMainMenu");

	//Action *customStatus = new Action(FMainMenu);
	//customStatus->setText(tr("My Status..."));
	//connect(customStatus,SIGNAL(triggered(bool)), SLOT(onCustomStatusAction(bool)));
	//FMainMenu->addAction(customStatus,AG_SCSM_STATUSCHANGER_CUSTOM_ACTIONS,false);

	//Action *clearCustomStatus = new Action(FMainMenu);
	//clearCustomStatus->setText(tr("Clear My Statuses"));
	//connect(clearCustomStatus,SIGNAL(triggered(bool)), SLOT(onClearCustomStatusAction(bool)));
	//FMainMenu->addAction(clearCustomStatus,AG_SCSM_STATUSCHANGER_CUSTOM_ACTIONS,false);

	createDefaultStatus();
	setMainStatusId(STATUS_OFFLINE);

	if (FMainWindowPlugin)
	{
		ToolBarChanger *changer = FMainWindowPlugin->mainWindow()->statusToolBarChanger();
		FStatusWidget = new StatusWidget(this, FAvatars, FVCardPlugin, FMainWindowPlugin, changer->toolBar());
		changer->insertWidget(FStatusWidget);
		FStatusMenu->setStyleSheet(FStatusWidget->styleSheet());
	}

	if (FNotifications)
	{
		INotificationType notifyType;
		notifyType.order = OWO_NOTIFICATIONS_CONNECTION;
		notifyType.kindMask = INotification::PopupWindow|INotification::SoundPlay;
		notifyType.kindDefs = notifyType.kindMask;
		FNotifications->registerNotificationType(NNT_CONNECTION_STATE,notifyType);
	}

	return true;
}

bool StatusChanger::initSettings()
{
	Options::setDefaultValue(OPV_STATUS_SHOW,IPresence::Online);
	Options::setDefaultValue(OPV_STATUS_TEXT,nameByShow(IPresence::Online));
	Options::setDefaultValue(OPV_STATUS_PRIORITY,0);
	Options::setDefaultValue(OPV_STATUSES_MAINSTATUS,STATUS_ONLINE);
	Options::setDefaultValue(OPV_ACCOUNT_AUTOCONNECT,false);
	Options::setDefaultValue(OPV_ACCOUNT_AUTORECONNECT,true);
	Options::setDefaultValue(OPV_ACCOUNT_STATUS_ISMAIN,true);
	Options::setDefaultValue(OPV_ACCOUNT_STATUS_LASTONLINE,STATUS_MAIN_ID);
	return true;
}

bool StatusChanger::startPlugin()
{
	qsrand(QDateTime::currentDateTime().toTime_t());
	updateMainMenu();
	return true;
}

//IStatusChanger
Menu *StatusChanger::statusMenu() const
{
	return FStatusMenu;
}

int StatusChanger::mainStatus() const
{
	return FStatusItems.value(STATUS_MAIN_ID).code;
}

void StatusChanger::setMainStatus(int AStatusId)
{
	setStreamStatus(Jid::null, AStatusId);
}

int StatusChanger::streamStatus(const Jid &AStreamJid) const
{
	QMap<IPresence *, int>::const_iterator it = FCurrentStatus.constBegin();
	while (it!=FCurrentStatus.constEnd())
	{
		if (it.key()->streamJid() == AStreamJid)
			return it.value();
		it++;
	}
	return !AStreamJid.isValid() ? mainStatus() : STATUS_NULL_ID;
}

void StatusChanger::setStreamStatus(const Jid &AStreamJid, int AStatusId)
{
	if (FStatusItems.contains(AStatusId))
	{
		StatusItem status = FStatusItems.value(AStatusId);

		bool isSwitchOffline = false;
		bool isSwitchOnline = false;
		bool isChangeMainStatus = !AStreamJid.isValid() && AStatusId != STATUS_MAIN_ID;

		if (isChangeMainStatus)
		{
			StatusItem curStatus = FStatusItems.value(visibleMainStatusId());
			if (status.show==IPresence::Offline || status.show==IPresence::Error)
				isSwitchOffline = true;
			else if (curStatus.show==IPresence::Offline || curStatus.show==IPresence::Error)
				isSwitchOnline = true;
			setMainStatusId(AStatusId);
		}

		QMap<IPresence *, int>::const_iterator it = FCurrentStatus.constBegin();
		while (it != FCurrentStatus.constEnd())
		{
			int newStatusId = AStatusId;
			StatusItem newStatus = status;
			IPresence *presence = it.key();

			bool acceptStatus = presence->streamJid()==AStreamJid;
			acceptStatus |= isChangeMainStatus && FMainStatusStreams.contains(presence);
			acceptStatus |= FCurrentStatus.count() == 1;

			if (!acceptStatus && isSwitchOnline && !presence->xmppStream()->isOpen())
			{
				newStatusId = FLastOnlineStatus.value(presence, STATUS_MAIN_ID);
				newStatus = FStatusItems.value(FStatusItems.contains(newStatusId) ? newStatusId : STATUS_MAIN_ID);
				acceptStatus = true;
			}
			else if (!acceptStatus && isSwitchOffline)
			{
				acceptStatus = true;
			}

			if (acceptStatus)
			{
				if (newStatusId == STATUS_MAIN_ID)
					FMainStatusStreams += presence;
				else if (presence->streamJid() == AStreamJid)
					FMainStatusStreams -= presence;

				emit statusAboutToBeChanged(presence->streamJid(), newStatus.code);

				FChangingPresence = presence;
				if (!presence->setPresence(newStatus.show, newStatus.text, newStatus.priority))
				{
					FChangingPresence = NULL;
					if (newStatus.show!=IPresence::Offline && newStatus.show!=IPresence::Error && !presence->xmppStream()->isOpen())
					{
						LogDetaile(QString("[StatusChanger] Opening XMPP stream '%1'").arg(presence->streamJid().full()));
						if (presence->xmppStream()->open())
						{
							setStreamStatusId(presence, STATUS_CONNECTING_ID);
							FLastOnlineStatus.insert(presence,newStatusId);
							FConnectStatus.insert(presence,FMainStatusStreams.contains(presence) ? STATUS_MAIN_ID : newStatus.code);
						}
						else
						{
							LogError(QString("[StatusChanger] Failed to open XMPP stream '%1'").arg(presence->streamJid().bare()));
						}
					}
				}
				else
				{
					FChangingPresence = NULL;
					setStreamStatusId(presence, FMainStatusStreams.contains(presence) ? STATUS_MAIN_ID : newStatus.code);

					if (newStatus.show==IPresence::Offline || newStatus.show==IPresence::Error)
					{
						LogDetaile(QString("[StatusChanger] Closing XMPP stream '%1'").arg(presence->streamJid().bare()));
						presence->xmppStream()->close();
					}
					else
					{
						FLastOnlineStatus.insert(presence,newStatusId);
					}

					emit statusChanged(presence->streamJid(), newStatus.code);
				}
			}
			it++;
		}
		updateMainMenu();
	}
}

QString StatusChanger::statusItemName(int AStatusId) const
{
	if (FStatusItems.contains(AStatusId))
		return FStatusItems.value(AStatusId).name;
	return QString();
}

int StatusChanger::statusItemShow(int AStatusId) const
{
	if (FStatusItems.contains(AStatusId))
		return FStatusItems.value(AStatusId).show;
	return -1;
}

QString StatusChanger::statusItemText(int AStatusId) const
{
	if (FStatusItems.contains(AStatusId))
		return FStatusItems.value(AStatusId).text;
	return QString();
}

int StatusChanger::statusItemPriority(int AStatusId) const
{
	if (FStatusItems.contains(AStatusId))
		return FStatusItems.value(AStatusId).priority;
	return 0;
}

QList<int> StatusChanger::statusItems() const
{
	return FStatusItems.keys();
}

QList<int> StatusChanger::activeStatusItems() const
{
	QList<int> active;
	foreach (int statusId, FCurrentStatus)
		active.append(statusId > STATUS_NULL_ID ? statusId: FStatusItems.value(statusId).code);
	return active;
}

QList<int> StatusChanger::statusByShow(int AShow) const
{
	QList<int> statuses;
	for (QMap<int, StatusItem>::const_iterator it = FStatusItems.constBegin(); it!=FStatusItems.constEnd(); it++)
	{
		if (it.key()>STATUS_NULL_ID && it->show==AShow)
			statuses.append(it->code);
	}
	return statuses;
}

int StatusChanger::statusByName(const QString &AName) const
{
	for (QMap<int, StatusItem>::const_iterator it = FStatusItems.constBegin(); it!=FStatusItems.constEnd(); it++)
	{
		if (it->name.toLower() == AName.toLower())
			return it->code;
	}
	return STATUS_NULL_ID;
}

int StatusChanger::addStatusItem(const QString &AName, int AShow, const QString &AText, int APriority)
{
	int statusId = statusByName(AName);
	if (statusId==STATUS_NULL_ID && !AName.isEmpty())
	{
		statusId = qrand();
		while(statusId<=STATUS_MAX_STANDART_ID || FStatusItems.contains(statusId))
			statusId = (statusId > STATUS_MAX_STANDART_ID) ? statusId+1 : STATUS_MAX_STANDART_ID+1;
		StatusItem status;
		status.code = statusId;
		status.name = AName;
		status.show = AShow;
		status.text = AText;
		status.priority = APriority;
		status.lastActive = QDateTime::currentDateTime();
		FStatusItems.insert(statusId,status);
		createStatusActions(statusId);
		emit statusItemAdded(statusId);
		removeRedundantCustomStatuses();
	}
	else if (statusId > STATUS_NULL_ID)
	{
		updateStatusItem(statusId,AName,AShow,AText,APriority);
	}
	return statusId;
}

void StatusChanger::updateStatusItem(int AStatusId, const QString &AName, int AShow, const QString &AText, int APriority)
{
	if (FStatusItems.contains(AStatusId) && !AName.isEmpty())
	{
		StatusItem &status = FStatusItems[AStatusId];
		if (status.name == AName || statusByName(AName) == STATUS_NULL_ID)
		{
			status.name = AName;
			status.show = AShow;
			status.text = AText;
			status.priority = APriority;
			updateStatusActions(AStatusId);
			emit statusItemChanged(AStatusId);
			resendUpdatedStatus(AStatusId);
		}
	}
}

void StatusChanger::removeStatusItem(int AStatusId)
{
	if (AStatusId>STATUS_MAX_STANDART_ID && FStatusItems.contains(AStatusId))
	{
		if (mainStatus() == AStatusId)
		{
			int newStatusId = statusByShow(FStatusItems.value(AStatusId).show).value(0);
			setMainStatus(newStatusId);
		}
		for (QMap<IPresence *, int>::const_iterator it = FCurrentStatus.constBegin(); it!=FCurrentStatus.constEnd(); it++)
		{
			if(it.value() == AStatusId)
			{
				int newStatusId = statusByShow(FStatusItems.value(AStatusId).show).value(0);
				setStreamStatus(it.key()->streamJid(), newStatusId);
			}
		}
		if (!activeStatusItems().contains(AStatusId))
		{
			emit statusItemRemoved(AStatusId);
			removeStatusActions(AStatusId);
			FStatusItems.remove(AStatusId);
		}
	}
}

QIcon StatusChanger::iconByShow(int AShow) const
{
	return FStatusIcons != NULL ? FStatusIcons->iconByStatus(AShow,"",false) : QIcon();
}

QString StatusChanger::nameByShow(int AShow) const
{
	switch (AShow)
	{
	case IPresence::Offline:
		return tr("Offline");
	case IPresence::Online:
		return tr("Online");
	case IPresence::Chat:
		return tr("Chat");
	case IPresence::Away:
		return tr("Away");
	case IPresence::ExtendedAway:
		return tr("Extended Away");
	case IPresence::DoNotDisturb:
		return tr("Do not disturb");
	case IPresence::Invisible:
		return tr("Invisible");
	case IPresence::Error:
		return tr("Error");
	default:
		return tr("Unknown Status");
	}
}

void StatusChanger::createDefaultStatus()
{
	StatusItem status;
	status.code = STATUS_ONLINE;
	status.name = nameByShow(IPresence::Online);
	status.show = IPresence::Online;
	status.priority = 30;
	FStatusItems.insert(status.code,status);
	createStatusActions(status.code);

	status.code = STATUS_AWAY;
	status.name = nameByShow(IPresence::Away);
	status.show = IPresence::Away;
	status.priority = 20;
	FStatusItems.insert(status.code,status);
	createStatusActions(status.code);

	status.code = STATUS_DND;
	status.name = nameByShow(IPresence::DoNotDisturb);
	status.show = IPresence::DoNotDisturb;
	status.priority = 15;
	FStatusItems.insert(status.code,status);
	createStatusActions(status.code);

	status.code = STATUS_OFFLINE;
	status.name = nameByShow(IPresence::Offline);
	status.show = IPresence::Offline;
	status.priority = 0;
	FStatusItems.insert(status.code,status);
	createStatusActions(status.code);

	status.code = STATUS_ERROR_ID;
	status.name = nameByShow(IPresence::Error);
	status.show = IPresence::Error;
	status.priority = 0;
	FStatusItems.insert(status.code,status);

	status.code = STATUS_CONNECTING_ID;
	status.name = tr("Connecting...");
	status.show = IPresence::Offline;
	status.priority = 0;
	FStatusItems.insert(status.code,status);
}

void StatusChanger::setMainStatusId(int AStatusId)
{
	if (FStatusItems.contains(AStatusId))
	{
		FStatusItems[STATUS_MAIN_ID] = FStatusItems.value(AStatusId);
	}
}

void StatusChanger::setStreamStatusId(IPresence *APresence, int AStatusId)
{
	if (FStatusItems.contains(AStatusId))
	{
		FCurrentStatus[APresence] = AStatusId;
		if (AStatusId > MAX_TEMP_STATUS_ID)
			removeTempStatus(APresence);

		FStatusItems[FStatusItems.value(AStatusId).code].lastActive = QDateTime::currentDateTime();

		IRosterIndex *index = FRostersView && FRostersModel ? FRostersModel->streamRoot(APresence->streamJid()) : NULL;
		if (index)
		{
			if (APresence->show() == IPresence::Error)
				FRostersView->insertFooterText(FTO_ROSTERSVIEW_STATUS,APresence->status(),index);
			else
				FRostersView->removeFooterText(FTO_ROSTERSVIEW_STATUS,index);
		}
		updateStatusNotification(APresence);
	}
}

Action *StatusChanger::createStatusAction(int AStatusId, const Jid &AStreamJid, QObject *AParent) const
{
	Action *action = new Action(AParent);
	if (AStreamJid.isValid())
		action->setData(ADR_STREAMJID,AStreamJid.full());
	action->setData(ADR_STATUS_CODE,AStatusId);
	connect(action,SIGNAL(triggered(bool)),SLOT(onSetStatusByAction(bool)));
	updateStatusAction(AStatusId,action);
	return action;
}

void StatusChanger::updateStatusAction(int AStatusId, Action *AAction) const
{
	StatusItem status = FStatusItems.value(AStatusId);
	AAction->setText(status.name);

	QIcon shadowedIcon;
	QIcon srcIcon = iconByShow(status.show);
	QGraphicsDropShadowEffect *shadow = qobject_cast<QGraphicsDropShadowEffect *>(GraphicsEffectsStorage::staticStorage(RSR_STORAGE_GRAPHICSEFFECTS)->getFirstEffect(GFX_STATUSICONS));
	if (shadow)
	{
		QImage img = srcIcon.pixmap(srcIcon.availableSizes().value(0)).toImage();
		QImage shadowedImage = ImageManager::addShadow(img, shadow->color(), shadow->offset().toPoint());
		shadowedIcon.addPixmap(QPixmap::fromImage(shadowedImage));
		AAction->setIcon(shadowedIcon);
	}
	else
	{
		AAction->setIcon(srcIcon);
	}

	int sortShow = status.show != IPresence::Offline ? status.show : 100;
	AAction->setData(Action::DR_SortString,QString("%1-%2").arg(sortShow,5,10,QChar('0')).arg(status.name));
}

void StatusChanger::createStatusActions(int AStatusId)
{
	int group = AG_SCSM_STATUSCHANGER_DEFAULT_STATUS;
	FStatusMenu->addAction(createStatusAction(AStatusId,Jid::null,FStatusMenu),group,true);
}

void StatusChanger::updateStatusActions(int AStatusId)
{
	QMultiHash<int, QVariant> findData;
	findData.insert(ADR_STATUS_CODE,AStatusId);
	QList<Action *> actionList = FStatusMenu->findActions(findData,true);
	foreach (Action *action, actionList)
		updateStatusAction(AStatusId,action);
}

void StatusChanger::removeStatusActions(int AStatusId)
{
	QMultiHash<int, QVariant> data;
	data.insert(ADR_STATUS_CODE,AStatusId);
	qDeleteAll(FStatusMenu->findActions(data,true));
}

int StatusChanger::visibleMainStatusId() const
{
	int statusId = STATUS_OFFLINE;

	bool isOnline = false;
	QMap<IPresence *, int>::const_iterator it = FCurrentStatus.constBegin();
	while ((!isOnline || statusId!=STATUS_MAIN_ID) && it!=FCurrentStatus.constEnd())
	{
		if (it.key()->xmppStream()->isOpen())
		{
			isOnline = true;
			statusId = it.value();
		}
		else if (!isOnline && it.value()==STATUS_CONNECTING_ID)
		{
			isOnline = true;
			statusId = STATUS_CONNECTING_ID;
		}
		else if (!isOnline && statusId!=STATUS_MAIN_ID)
		{
			statusId = it.value();
		}
		it++;
	}

	return statusId;
}

void StatusChanger::updateMainMenu()
{
	int statusId = visibleMainStatusId();

	QIcon shadowedIcon;
	QIcon srcIcon = (statusId != STATUS_CONNECTING_ID) ? iconByShow(statusItemShow(statusId)) : IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getIcon(MNI_SCHANGER_CONNECTING);
	QGraphicsDropShadowEffect * shadow = qobject_cast<QGraphicsDropShadowEffect *>(GraphicsEffectsStorage::staticStorage(RSR_STORAGE_GRAPHICSEFFECTS)->getFirstEffect(GFX_STATUSICONS));
	if (shadow)
	{
		QImage img = srcIcon.pixmap(srcIcon.availableSizes().value(0)).toImage();
		QImage shadowedImage = ImageManager::addShadow(img, shadow->color(), shadow->offset().toPoint());
		shadowedIcon.addPixmap(QPixmap::fromImage(shadowedImage));
		FStatusMenu->setIcon(shadowedIcon);
	}
	else
		FStatusMenu->setIcon(srcIcon);

	FStatusMenu->setTitle(statusItemName(statusId));
	FStatusMenu->menuAction()->setVisible(!FCurrentStatus.isEmpty());


	if (FTrayManager)
	{
		if (statusId != STATUS_CONNECTING_ID)
		{
			IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->removeAutoIcon(FTrayManager->instance());
			FTrayManager->setIcon(iconByShow(statusItemShow(statusId)));
		}
		else
		{
			IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->insertAutoIcon(FTrayManager->instance(),MNI_SCHANGER_CONNECTING);
		}

		QString trayToolTip = QString(tr("Contacts - %1")).arg(statusItemName(visibleMainStatusId()));
		FTrayManager->setToolTip(trayToolTip);
	}
}

int StatusChanger::createTempStatus(IPresence *APresence, int AShow, const QString &AText, int APriority)
{
	removeTempStatus(APresence);

	StatusItem status;
	status.name = nameByShow(AShow).append('*');
	status.show = AShow;
	status.text = AText;
	status.priority = APriority;
	status.code = MAX_TEMP_STATUS_ID;
	while (FStatusItems.contains(status.code))
		status.code--;
	FStatusItems.insert(status.code,status);
	FTempStatus.insert(APresence,status.code);
	return status.code;
}

void StatusChanger::removeTempStatus(IPresence *APresence)
{
	if (FTempStatus.contains(APresence))
		if (!activeStatusItems().contains(FTempStatus.value(APresence)))
			FStatusItems.remove(FTempStatus.take(APresence));
}

void StatusChanger::autoReconnect(IPresence *APresence)
{
	IAccount *account = FAccountManager!=NULL ? FAccountManager->accountByStream(APresence->streamJid()) : NULL;
	if (account && account->optionsNode().value("auto-reconnect").toBool())
	{
		int statusId = FLastOnlineStatus.value(APresence, STATUS_MAIN_ID);
		int statusShow = statusItemShow(statusId);
		if (statusShow!=IPresence::Offline && statusShow!=IPresence::Error)
		{
			int reconStep = qMin(FReconnectStep.value(APresence,0),MAX_RECON_STEP);
			int reconSecs = ReconSteps[reconStep].base + qRound(ReconSteps[reconStep].random * qrand() / (RAND_MAX + 1.0));
			FReconnectStep[APresence] = reconStep+1;

			LogDetaile(QString("[StatusChanger] Starting auto reconnection of '%1' after %2 seconds").arg(APresence->streamJid().full()).arg(reconSecs));
			FPendingReconnect.insert(APresence,QPair<QDateTime,int>(QDateTime::currentDateTime().addSecs(reconSecs),statusId));
			QTimer::singleShot(reconSecs*1000+100,this,SLOT(onReconnectTimer()));
		}
	}
}

void StatusChanger::resendUpdatedStatus(int AStatusId)
{
	if (FStatusItems[STATUS_MAIN_ID].code == AStatusId)
		setMainStatus(AStatusId);

	for (QMap<IPresence *, int>::const_iterator it = FCurrentStatus.constBegin(); it != FCurrentStatus.constEnd(); it++)
		if (it.value() == AStatusId)
			setStreamStatus(it.key()->streamJid(), AStatusId);
}

void StatusChanger::removeAllCustomStatuses()
{
	foreach (int statusId, FStatusItems.keys())
		if (statusId > STATUS_MAX_STANDART_ID)
			removeStatusItem(statusId);
}

void StatusChanger::removeRedundantCustomStatuses()
{
	QMap<int, QMap<QDateTime, int> > statuses;
	for (QMap<int, StatusItem>::const_iterator it = FStatusItems.constBegin(); it!=FStatusItems.constEnd(); it++)
	{
		if (it.key() > STATUS_MAX_STANDART_ID)
		{
			if (it->lastActive.isValid())
				statuses[it->show].insert(it->lastActive,it->code);
			else
				statuses[it->show].insert(QDateTime::fromTime_t(0),it->code);
		}
	}
	foreach(int show, statuses.keys())
	{
		QList<int> showStatuses = statuses.value(show).values();
		while(showStatuses.count() > MAX_CUSTOM_STATUS_PER_SHOW)
		{
			removeStatusItem(showStatuses.takeFirst());
		}
	}
}

void StatusChanger::updateStatusNotification(IPresence *APresence)
{
	if (FNotifications && FCurrentStatus.value(APresence)!=STATUS_CONNECTING_ID)
	{
		int notifyId = FConnectNotifyId.value(APresence,0);
		bool isFailed = APresence->show()==IPresence::Error && notifyId>=0;
		bool isRestored = APresence->show()!=IPresence::Error && APresence->show()!=IPresence::Offline && notifyId<0;
		if (isFailed || isRestored)
		{
			removeStatusNotification(APresence);

			INotification notify;
			notify.kinds = FNotifications->notificationKinds(NNT_CONNECTION_STATE);
			if (notify.kinds > 0)
			{
				notify.typeId = NNT_CONNECTION_STATE;
				notify.data.insert(NDR_ICON,FStatusIcons!=NULL ? FStatusIcons->iconByStatus(IPresence::Error,QString::null,false) : QIcon());
				notify.data.insert(NDR_POPUP_TITLE,isFailed ? tr("Temporary connection failure") : tr("Connection restored"));
				notify.data.insert(NDR_POPUP_NOTICE,isFailed ? tr("Problem") : tr("Problem resolved"));
				notify.data.insert(NDR_POPUP_IMAGE, IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getImage(isFailed ? MNI_SCHANGER_CONNECTION_ERROR : MNI_SCHANGER_CONNECTION_RESTORE));
				notify.data.insert(NDR_POPUP_TEXT,isFailed ? Qt::escape(APresence->status()) : QString());
				notify.data.insert(NDR_POPUP_STYLEKEY,isFailed ? STS_SCHANGER_NOTIFYWIDGET_CONNECTION_ERROR : STS_SCHANGER_NOTIFYWIDGET_CONNECTION_RESTORE);
				notify.data.insert(NDR_SOUND_FILE,isFailed ? SDF_SCHANGER_CONNECTION_ERROR : SDF_SCHANGER_CONNECTION_RESTORE);
				notify.data.insert(NDR_POPUP_CAN_ACTIVATE, false);

				notifyId = FNotifications->appendNotification(notify);
				FConnectNotifyId.insert(APresence, isFailed ? 0-notifyId : notifyId);
			}
		}
		else if (APresence->show()==IPresence::Offline && notifyId<0)
		{
			removeStatusNotification(APresence);
		}
	}
}

void StatusChanger::removeStatusNotification(IPresence *APresence)
{
	if (FNotifications && FConnectNotifyId.contains(APresence))
	{
		FNotifications->removeNotification(qAbs(FConnectNotifyId.take(APresence)));
	}
}

void StatusChanger::onSetStatusByAction(bool)
{
	Action *action = qobject_cast<Action *>(sender());
	if (action)
	{
		QString streamJid = action->data(ADR_STREAMJID).toString();
		int statusId = action->data(ADR_STATUS_CODE).toInt();
		setStreamStatus(streamJid, statusId);
	}
}

void StatusChanger::onPresenceAdded(IPresence *APresence)
{
	FCurrentStatus.insert(APresence,STATUS_OFFLINE);

	IAccount *account = FAccountManager!=NULL ? FAccountManager->accountByStream(APresence->streamJid()) : NULL;
	if (account)
	{
		if (account->optionsNode().value("status.is-main").toBool())
			FMainStatusStreams += APresence;
		FLastOnlineStatus.insert(APresence, account->optionsNode().value("status.last-online").toInt());
	}

	if (FStatusWidget)
		FStatusWidget->setStreamJid(APresence->streamJid());

	updateMainMenu();
}

void StatusChanger::onPresenceChanged(IPresence *APresence, int AShow, const QString &AText, int APriority)
{
	if (FCurrentStatus.contains(APresence))
	{
		if (AShow == IPresence::Error)
		{
			autoReconnect(APresence);
			setStreamStatusId(APresence, STATUS_ERROR_ID);
			updateMainMenu();
		}
		else if (FChangingPresence != APresence)
		{
			StatusItem status = FStatusItems.value(FCurrentStatus.value(APresence));
			if (status.name.isEmpty() || status.show!=AShow || status.priority!=APriority || status.text!=AText)
			{
				setStreamStatusId(APresence, createTempStatus(APresence,AShow,AText,APriority));
				updateMainMenu();
			}
		}
		FConnectStatus.remove(APresence);
	}
}

void StatusChanger::onPresenceRemoved(IPresence *APresence)
{
	IAccount *account = FAccountManager!=NULL ? FAccountManager->accountByStream(APresence->streamJid()) : NULL;
	if (account)
	{
		bool isMainStatus = FMainStatusStreams.contains(APresence);
		account->optionsNode().setValue(isMainStatus,"status.is-main");
		if (!isMainStatus && account->optionsNode().value("auto-connect").toBool() && FLastOnlineStatus.contains(APresence))
			account->optionsNode().setValue(FLastOnlineStatus.value(APresence),"status.last-online");
		else
			account->optionsNode().setValue(QVariant(),"status.last-online");
	}

	removeStatusNotification(APresence);
	removeTempStatus(APresence);

	FMainStatusStreams -= APresence;
	FCurrentStatus.remove(APresence);
	FConnectStatus.remove(APresence);
	FLastOnlineStatus.remove(APresence);
	FPendingReconnect.remove(APresence);

	updateMainMenu();
}

void StatusChanger::onRosterOpened(IRoster *ARoster)
{
	IPresence *presence = FPresencePlugin->findPresence(ARoster->streamJid());
	if (FConnectStatus.contains(presence))
	{
		LogDetaile(QString("[StatusChanger] Sending initial presence of stream '%1'").arg(ARoster->streamJid().full()));
		setStreamStatus(presence->streamJid(), FConnectStatus.value(presence));
	}
	FReconnectStep.remove(presence);
}

void StatusChanger::onRosterClosed(IRoster *ARoster)
{
	IPresence *presence = FPresencePlugin->findPresence(ARoster->streamJid());
	if (FShutdownList.contains(presence))
	{
		FShutdownList.removeAll(presence);
		FPluginManager->continueShutdown();
	}
	else if (FConnectStatus.contains(presence))
	{
		setStreamStatus(presence->streamJid(), FConnectStatus.value(presence));
	}
}

void StatusChanger::onStreamJidChanged(const Jid &ABefour, const Jid &AAfter)
{
	QMultiHash<int,QVariant> data;
	data.insert(ADR_STREAMJID,ABefour.full());
	QList<Action *> actionList = FStatusMenu->findActions(data,true);
	foreach (Action *action, actionList)
		action->setData(ADR_STREAMJID,AAfter.full());

	if (FStatusWidget && FStatusWidget->streamJid()==ABefour)
		FStatusWidget->setStreamJid(AAfter);
}

void StatusChanger::onDefaultStatusIconsChanged()
{
	foreach (StatusItem status, FStatusItems)
		updateStatusActions(status.code);
	updateMainMenu();
}

void StatusChanger::onOptionsOpened()
{
	removeAllCustomStatuses();
	/*foreach (QString ns, Options::node(OPV_STATUSES_ROOT).childNSpaces("status"))
	{
		int statusId = ns.toInt();
		OptionsNode soptions = Options::node(OPV_STATUS_ITEM, ns);
		QString statusName = soptions.value("name").toString();
		if (statusId > STATUS_MAX_STANDART_ID)
		{
			if (!statusName.isEmpty() && statusByName(statusName)==STATUS_NULL_ID)
			{
				StatusItem status;
				status.code = statusId;
				status.name = nameByShow(status.show);//statusName;
				status.show = (IPresence::Show)soptions.value("show").toInt();
				status.text = soptions.value("text").toString();
				status.priority = soptions.value("priority").toInt();
				status.lastActive = soptions.value("last-active").toDateTime();
				FStatusItems.insert(status.code,status);
				createStatusActions(status.code);
			}
		}
		else if (statusId > STATUS_NULL_ID && FStatusItems.contains(statusId))
		{
			StatusItem &status = FStatusItems[statusId];
			if (!statusName.isEmpty())
				status.name = statusName;
			status.text = soptions.hasValue("text") ? soptions.value("text").toString() : QString::null;
			status.priority = soptions.hasValue("priority") ? soptions.value("priority").toInt() : status.priority;
			updateStatusActions(statusId);
		}
	}*/
	removeRedundantCustomStatuses();

	QString commonStatusText = statusItemText(STATUS_ONLINE);
	foreach(int statusId, statusItems())
		if (statusId>STATUS_NULL_ID && statusItemText(statusId)!=commonStatusText)
			updateStatusItem(statusId,statusItemName(statusId),statusItemShow(statusId),commonStatusText,statusItemPriority(statusId));

	setMainStatusId(Options::node(OPV_STATUSES_MAINSTATUS).value().toInt());
}

void StatusChanger::onOptionsClosed()
{
	/*QList<QString> oldNS = Options::node(OPV_STATUSES_ROOT).childNSpaces("status");
	foreach (StatusItem status, FStatusItems)
	{
		if (status.code > STATUS_NULL_ID)
		{
			OptionsNode soptions = Options::node(OPV_STATUS_ITEM, QString::number(status.code));
			if (status.code > STATUS_MAX_STANDART_ID)
			{
				soptions.setValue(status.show,"show");
				soptions.setValue(status.lastActive,"last-active");
			}
			soptions.setValue(status.name,"name");
			soptions.setValue(status.text,"text");
			soptions.setValue(status.priority,"priority");
		}
		oldNS.removeAll(QString::number(status.code));
	}

	foreach(QString ns, oldNS)
		Options::node(OPV_STATUSES_ROOT).removeChilds("status",ns);*/

	Options::node(OPV_STATUSES_MAINSTATUS).setValue(FStatusItems.value(STATUS_MAIN_ID).code);

	setMainStatusId(STATUS_OFFLINE);
	removeAllCustomStatuses();
}

void StatusChanger::onProfileOpened(const QString &AProfile)
{
	Q_UNUSED(AProfile);
	foreach(IPresence *presence, FCurrentStatus.keys())
	{
		IAccount *account = FAccountManager!=NULL ? FAccountManager->accountByStream(presence->streamJid()) : NULL;
		if (account!=NULL && account->optionsNode().value("auto-connect").toBool())
		{
			int statusId = !FMainStatusStreams.contains(presence) ? FLastOnlineStatus.value(presence, STATUS_MAIN_ID) : STATUS_MAIN_ID;
			if (!FStatusItems.contains(statusId))
				statusId = STATUS_MAIN_ID;
			setStreamStatus(presence->streamJid(), statusId);
		}
	}
}

void StatusChanger::onShutdownStarted()
{
	FShutdownList.clear();
	foreach(IPresence *presence, FCurrentStatus.keys())
	{
		if (presence->isOpen())
		{
			FPluginManager->delayShutdown();
			FShutdownList.append(presence);
			presence->xmppStream()->close();
		}
	}
}

void StatusChanger::onReconnectTimer()
{
	QMap<IPresence *,QPair<QDateTime,int> >::iterator it = FPendingReconnect.begin();
	while (it != FPendingReconnect.end())
	{
		if (it.value().first <= QDateTime::currentDateTime())
		{
			IPresence *presence = it.key();
			int statusId = FStatusItems.contains(it.value().second) ? it.value().second : STATUS_MAIN_ID;
			it = FPendingReconnect.erase(it);
			if (presence->show() == IPresence::Error)
				setStreamStatus(presence->streamJid(), statusId);
		}
		else
		{
			it++;
		}
	}
}

void StatusChanger::onCustomStatusAction(bool)
{
	if (FCustomStatusDialog.isNull())
		FCustomStatusDialog = new CustomStatusDialog(this,Jid::null);
	FCustomStatusDialog->show();
}

void StatusChanger::onClearCustomStatusAction(bool)
{
	removeAllCustomStatuses();
}

void StatusChanger::onTrayContextMenuAboutToShow()
{
	if (FStatusMenu->menuAction()->isVisible())
		foreach(Action *action, FStatusMenu->groupActions(AG_SCSM_STATUSCHANGER_CUSTOM_STATUS)+FStatusMenu->groupActions(AG_SCSM_STATUSCHANGER_DEFAULT_STATUS)) {
			FTrayManager->contextMenu()->addAction(action,AG_TMTM_STATUSCHANGER_CHANGESTATUS,true); }
}

void StatusChanger::onTrayContextMenuAboutToHide()
{
	foreach(Action *action, FTrayManager->contextMenu()->groupActions(AG_TMTM_STATUSCHANGER_CHANGESTATUS)) {
		FTrayManager->contextMenu()->removeAction(action); }
}

void StatusChanger::onNotificationActivated(int ANotifyId)
{
	if (FConnectNotifyId.values().contains(ANotifyId) || FConnectNotifyId.values().contains(0-ANotifyId))
	{
		if (FMainWindowPlugin)
			FMainWindowPlugin->showMainWindow();
		FNotifications->removeNotification(ANotifyId);
	}
}

Q_EXPORT_PLUGIN2(plg_statuschanger, StatusChanger)
