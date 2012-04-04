#include "notifications.h"

#include <QIcon>
#include <QImage>
#include <QProcess>
#include <QVBoxLayout>
#include <utils/imagemanager.h>
#include <definitions/notificationtypes.h>
#include "notifykindswidgets.h"
#include "systemnotifypreferences.h"

#ifdef DEBUG_ENABLED
# include <QDebug>
#endif

#define FIRST_KIND                      0x0001
#define LAST_KIND                       0x8000
#define UNDEFINED_KINDS                 0xFFFF

#define TEST_NOTIFY_TIMEOUT             10000

#define ADR_NOTIFYID                    Action::DR_Parametr1

Notifications::Notifications()
{
	FAvatars = NULL;
	FGateways = NULL;
	FRosterPlugin = NULL;
	FMetaContacts = NULL;
	FStatusIcons = NULL;
	FStatusChanger = NULL;
	FTrayManager = NULL;
	FRostersModel = NULL;
	FRostersViewPlugin = NULL;
	FOptionsManager = NULL;
	FSystemIntegration = NULL;
	FMainWindow = NULL;

	FNotifyId = 0;
	FTestNotifyId = -1;
	FActivateAll = NULL;

#ifdef USE_PHONON
	FMediaObject = NULL;
	FAudioOutput = NULL;
#else
	FSound = NULL;
#endif

	FTestNotifyTimer.setSingleShot(true);
	FTestNotifyTimer.setInterval(TEST_NOTIFY_TIMEOUT);
	connect(&FTestNotifyTimer,SIGNAL(timeout()),SLOT(onTestNotificationTimerTimedOut()));
}

Notifications::~Notifications()
{
	delete FActivateAll;
#ifdef USE_PHONON
	delete FMediaObject;
	delete FAudioOutput;
#else
	delete FSound;
#endif
}

void Notifications::pluginInfo(IPluginInfo *APluginInfo)
{
	APluginInfo->name = tr("Notifications Manager");
	APluginInfo->description = tr("Allows other modules to notify the user of the events");
	APluginInfo->version = "1.0";
	APluginInfo->author = "Potapov S.A. aka Lion";
	APluginInfo->homePage = "http://contacts.rambler.ru";
}

bool Notifications::initConnections(IPluginManager *APluginManager, int &AInitOrder)
{
	Q_UNUSED(AInitOrder);
	IPlugin *plugin = APluginManager->pluginInterface("ITrayManager").value(0,NULL);
	if (plugin)
	{
		FTrayManager = qobject_cast<ITrayManager *>(plugin->instance());
		if (FTrayManager)
		{
			connect(FTrayManager->instance(),SIGNAL(notifyActivated(int, QSystemTrayIcon::ActivationReason)),
				SLOT(onTrayNotifyActivated(int, QSystemTrayIcon::ActivationReason)));
			connect(FTrayManager->instance(),SIGNAL(notifyRemoved(int)),SLOT(onTrayNotifyRemoved(int)));
		}
	}

	plugin = APluginManager->pluginInterface("IRostersViewPlugin").value(0,NULL);
	if (plugin)
	{
		FRostersViewPlugin = qobject_cast<IRostersViewPlugin *>(plugin->instance());
		if (FRostersViewPlugin)
		{
			connect(FRostersViewPlugin->rostersView()->instance(),SIGNAL(notifyActivated(int)),
				SLOT(onRosterNotifyActivated(int)));
			connect(FRostersViewPlugin->rostersView()->instance(),SIGNAL(notifyTimeout(int)),
				SLOT(onRosterNotifyTimeout(int)));
			connect(FRostersViewPlugin->rostersView()->instance(),SIGNAL(notifyRemoved(int)),
				SLOT(onRosterNotifyRemoved(int)));
		}
	}

	plugin = APluginManager->pluginInterface("IRostersModel").value(0,NULL);
	if (plugin)
		FRostersModel = qobject_cast<IRostersModel *>(plugin->instance());

	plugin = APluginManager->pluginInterface("IMetaContacts").value(0,NULL);
	if (plugin)
		FMetaContacts = qobject_cast<IMetaContacts *>(plugin->instance());

	plugin = APluginManager->pluginInterface("IAvatars").value(0,NULL);
	if (plugin)
		FAvatars = qobject_cast<IAvatars *>(plugin->instance());

	plugin = APluginManager->pluginInterface("IGateways").value(0,NULL);
	if (plugin)
		FGateways = qobject_cast<IGateways *>(plugin->instance());

	plugin = APluginManager->pluginInterface("IRosterPlugin").value(0,NULL);
	if (plugin)
		FRosterPlugin = qobject_cast<IRosterPlugin *>(plugin->instance());

	plugin = APluginManager->pluginInterface("IStatusIcons").value(0,NULL);
	if (plugin)
		FStatusIcons = qobject_cast<IStatusIcons *>(plugin->instance());

	plugin = APluginManager->pluginInterface("IStatusChanger").value(0,NULL);
	if (plugin)
		FStatusChanger = qobject_cast<IStatusChanger *>(plugin->instance());

	plugin = APluginManager->pluginInterface("IOptionsManager").value(0,NULL);
	if (plugin)
		FOptionsManager = qobject_cast<IOptionsManager *>(plugin->instance());

	plugin = APluginManager->pluginInterface("ISystemIntegration").value(0,NULL);
	if (plugin)
	{
		FSystemIntegration = qobject_cast<ISystemIntegration *>(plugin->instance());
		if (FSystemIntegration)
		{
			connect(FSystemIntegration->instance(), SIGNAL(systemNotificationClicked(int)), SLOT(onSystemNotifyClicked(int)));
			connect(FSystemIntegration->instance(), SIGNAL(dockClicked()), SLOT(onDockClicked()));
		}
	}

	plugin = APluginManager->pluginInterface("IMainWindowPlugin").value(0,NULL);
	if (plugin)
	{
		FMainWindow = qobject_cast<IMainWindowPlugin *>(plugin->instance());
	}

	connect(this, SIGNAL(notificationAppended(int,const INotification &)), SLOT(onNotifyCountChanged()));
	connect(this, SIGNAL(notificationRemoved(int)), SLOT(onNotifyCountChanged()));

	return true;
}

bool Notifications::initObjects()
{
	FActivateAll = new Action(this);
	FActivateAll->setVisible(false);
	FActivateAll->setText(tr("New messages"));
	connect(FActivateAll,SIGNAL(triggered(bool)),SLOT(onTrayActionTriggered(bool)));

	if (FTrayManager)
		FTrayManager->contextMenu()->addAction(FActivateAll,AG_TMTM_NOTIFICATIONS_ACTIVATE,false);

	return true;
}

bool Notifications::initSettings()
{
	Options::setDefaultValue(OPV_NOTIFICATIONS_NONOTIFYIFAWAY,true);
	Options::setDefaultValue(OPV_NOTIFICATIONS_NONOTIFYIFDND,true);
	Options::setDefaultValue(OPV_NOTIFICATIONS_NONOTIFYIFFULLSCREEN,true);
	Options::setDefaultValue(OPV_NOTIFICATIONS_TYPEKINDS_ITEM,0);
	Options::setDefaultValue(OPV_NOTIFICATIONS_KINDENABLED_ITEM,true);
	Options::setDefaultValue(OPV_NOTIFICATIONS_SOUND_COMMAND,QString("aplay"));

	if (FOptionsManager)
	{
		FOptionsManager->insertServerOption(OPV_NOTIFICATIONS_TYPEKINDS_ROOT);
		FOptionsManager->insertServerOption(OPV_NOTIFICATIONS_NONOTIFYIFAWAY);
		FOptionsManager->insertServerOption(OPV_NOTIFICATIONS_NONOTIFYIFDND);
		FOptionsManager->insertServerOption(OPV_NOTIFICATIONS_NONOTIFYIFFULLSCREEN);

		IOptionsDialogNode dnode = { ONO_NOTIFICATIONS, OPN_NOTIFICATIONS, tr("Notifications"), MNI_NOTIFICATIONS_OPTIONS };
		FOptionsManager->insertOptionsDialogNode(dnode);
		FOptionsManager->insertOptionsHolder(this);
	}
	return true;
}

QMultiMap<int, IOptionsWidget *> Notifications::optionsWidgets(const QString &ANodeId, QWidget *AParent)
{
	QMultiMap<int, IOptionsWidget *> widgets;
	if (FOptionsManager && ANodeId == OPN_NOTIFICATIONS)
	{
		if (!(FSystemIntegration && FSystemIntegration->isSystemNotificationsSettingsAccessible()))
		{
			QMultiMap<int, IOptionsWidget *> kindsWidgets;

			widgets.insertMulti(OWO_NOTIFICATIONS_ITEM_OPTIONS,FOptionsManager->optionsHeaderWidget(QString::null,tr("Method of notification"),AParent));
			foreach(QString id, FNotifyTypes.keys())
			{
				INotificationType notifyType = FNotifyTypes.value(id);
				if (!notifyType.title.isEmpty())
				{
					NotifyKindsWidget *widget = new NotifyKindsWidget(this,id,notifyType.title,notifyType.kindMask,notifyType.kindDefs,AParent);
					connect(widget,SIGNAL(notificationTest(const QString &, ushort)),SIGNAL(notificationTest(const QString &, ushort)));
					kindsWidgets.insertMulti(notifyType.order, widget);
				}
			}

			NotifyKindsWidgets *kindsWidgetsContainer = new NotifyKindsWidgets(AParent);
			foreach (IOptionsWidget *widget, kindsWidgets)
				kindsWidgetsContainer->addWidget(widget);

			widgets.insertMulti(OWO_NOTIFICATIONS_ITEM_OPTIONS, kindsWidgetsContainer);
		}
		else
		{
			widgets.insertMulti(OWO_NOTIFICATIONS_SYSTEM_NOTIFY_PREFS, FOptionsManager->optionsHeaderWidget(QString::null,tr("%1 Notifications").arg(FSystemIntegration->systemNotificationsSystemName()),AParent));
			SystemNotifyPreferences * notifyPrefs = new SystemNotifyPreferences;
			connect(notifyPrefs, SIGNAL(showPreferences()), SLOT(onShowSystemNotificationsSettings()));
			widgets.insertMulti(OWO_NOTIFICATIONS_SYSTEM_NOTIFY_PREFS, notifyPrefs);
		}

		widgets.insertMulti(OWO_NOTIFICATIONS_IF_STATUS,FOptionsManager->optionsHeaderWidget(QString::null,tr("Disable all popup windows and sounds"),AParent));
		widgets.insertMulti(OWO_NOTIFICATIONS_IF_STATUS,FOptionsManager->optionsNodeWidget(Options::node(OPV_NOTIFICATIONS_NONOTIFYIFAWAY),tr("If status is 'Away'"),AParent));
		widgets.insertMulti(OWO_NOTIFICATIONS_IF_STATUS,FOptionsManager->optionsNodeWidget(Options::node(OPV_NOTIFICATIONS_NONOTIFYIFDND),tr("If status is 'Busy'"),AParent));

		widgets.insertMulti(OWO_NOTIFICATIONS_FULLSCREEN,FOptionsManager->optionsHeaderWidget(QString::null,tr("Full screen mode"),AParent));
		widgets.insertMulti(OWO_NOTIFICATIONS_FULLSCREEN,FOptionsManager->optionsNodeWidget(Options::node(OPV_NOTIFICATIONS_NONOTIFYIFFULLSCREEN),
			tr("Disable all popup windows when watching fullscreen movies or games"),AParent));
	}
	return widgets;
}

QList<int> Notifications::notifications() const
{
	return FNotifyRecords.keys();
}

INotification Notifications::notificationById(int ANotifyId) const
{
	return FNotifyRecords.value(ANotifyId).notification;
}

int Notifications::appendNotification(const INotification &ANotification)
{
	NotifyRecord record;
	int notifyId = ++FNotifyId;
	record.notification = ANotification;
	emit notificationAppend(notifyId, record.notification);

	bool isAway = FStatusChanger ? FStatusChanger->statusItemShow(STATUS_MAIN_ID) == IPresence::Away : false;
	bool isDND = FStatusChanger ? FStatusChanger->statusItemShow(STATUS_MAIN_ID) == IPresence::DoNotDisturb : false;

	bool blockPopupAndSound = isAway && Options::node(OPV_NOTIFICATIONS_NONOTIFYIFAWAY).value().toBool();
	blockPopupAndSound |= isDND && Options::node(OPV_NOTIFICATIONS_NONOTIFYIFDND).value().toBool();
	blockPopupAndSound |= SystemManager::isFullScreenMode() && Options::node(OPV_NOTIFICATIONS_NONOTIFYIFFULLSCREEN).value().toBool();

	QIcon icon;
	QString iconKey = record.notification.data.value(NDR_ICON_KEY).toString();
	QString iconStorage = record.notification.data.value(NDR_ICON_STORAGE).toString();
	if (!iconKey.isEmpty() && !iconStorage.isEmpty())
		icon = IconStorage::staticStorage(iconStorage)->getIcon(iconKey);
	else
		icon = qvariant_cast<QIcon>(record.notification.data.value(NDR_ICON));

	int replaceNotifyId = record.notification.data.value(NDR_REPLACE_NOTIFY).toInt();
	if (!FNotifyRecords.contains(replaceNotifyId))
		replaceNotifyId = -1;

	if (FRostersModel && FRostersViewPlugin && (record.notification.kinds & INotification::RosterNotify)>0)
	{
		bool createIndex = record.notification.data.value(NDR_ROSTER_CREATE_INDEX).toBool();
		Jid streamJid = record.notification.data.value(NDR_STREAM_JID).toString();
		Jid contactJid = record.notification.data.value(NDR_CONTACT_JID).toString();
		QList<IRosterIndex *> indexes = FRostersModel->getContactIndexList(streamJid,contactJid,createIndex);
		if (!indexes.isEmpty())
		{
			IRostersNotify rnotify;
			rnotify.icon = icon;
			rnotify.order = record.notification.data.value(NDR_ROSTER_ORDER).toInt();
			rnotify.flags = record.notification.data.value(NDR_ROSTER_FLAGS).toInt();
			rnotify.timeout = record.notification.data.value(NDR_ROSTER_TIMEOUT).toInt();
			rnotify.hookClick = record.notification.data.value(NDR_ROSTER_HOOK_CLICK).toBool();
			rnotify.footer = record.notification.data.value(NDR_ROSTER_FOOTER).toString();
			rnotify.background = record.notification.data.value(NDR_ROSTER_BACKGROUND).value<QBrush>();
			record.rosterId = FRostersViewPlugin->rostersView()->insertNotify(rnotify,indexes);
		}
	}

	if (!blockPopupAndSound && (record.notification.kinds & INotification::PopupWindow)>0)
	{
		if (FSystemIntegration && FSystemIntegration->isSystemNotificationsSettingsAccessible())
		{
			QImage icon = qvariant_cast<QImage>(record.notification.data.value(NDR_POPUP_IMAGE));
			QString title = record.notification.data.value(NDR_POPUP_TITLE).toString();
			QString text = record.notification.data.value(NDR_POPUP_TEXT).toString();
			QString type = record.notification.typeId;
			int id = notifyId;
			FSystemIntegration->postSystemNotify(icon, title, text, type, id);
		}
		else
		{
			if (replaceNotifyId > 0)
			{
				NotifyRecord &replRecord = FNotifyRecords[replaceNotifyId];
				record.popupWidget = replRecord.popupWidget;
				replRecord.popupWidget = NULL;
			}
			if (record.popupWidget.isNull())
			{
				record.popupWidget = new NotifyWidget(record.notification);
				connect(record.popupWidget,SIGNAL(showOptions()), SLOT(onWindowNotifyOptions()));
				connect(record.popupWidget,SIGNAL(notifyActivated()),SLOT(onWindowNotifyActivated()));
				connect(record.popupWidget,SIGNAL(notifyRemoved()),SLOT(onWindowNotifyRemoved()));
				connect(record.popupWidget,SIGNAL(windowDestroyed()),SLOT(onWindowNotifyDestroyed()));
				if (!record.popupWidget->appear())
				{
					record.popupWidget->deleteLater();
					record.popupWidget = NULL;
				}
			}
			else
			{
				record.popupWidget->appendNotification(record.notification);
			}
			foreach(Action *action, record.notification.actions)
			{
				record.popupWidget->appendAction(action);
			}
		}
	}

	if (FTrayManager)
	{
		QString toolTip = record.notification.data.value(NDR_TRAY_TOOLTIP).toString();

		if ((record.notification.kinds & INotification::TrayNotify)>0)
		{
			ITrayNotify notify;
			notify.blink = true;
			notify.icon = icon;
			notify.iconKey = iconKey;
			notify.iconStorage = iconStorage;
			notify.toolTip = toolTip;
			record.trayId = FTrayManager->appendNotify(notify);
		}
	}

	if (!blockPopupAndSound && (record.notification.kinds & INotification::SoundPlay)>0)
	{
#ifndef Q_WS_MAC // disabling sounds on Mac, user should deine sounds in Growl
		QString soundName = record.notification.data.value(NDR_SOUND_FILE).toString();
		QString soundFile = FileStorage::staticStorage(RSR_STORAGE_SOUNDS)->fileFullName(soundName);
		if (!soundFile.isEmpty())
		{
# ifdef USE_PHONON
			if (!FMediaObject)
			{
				FMediaObject = new Phonon::MediaObject(this);
				FAudioOutput = new Phonon::AudioOutput(Phonon::NotificationCategory, this);
				Phonon::createPath(FMediaObject, FAudioOutput);
			}
			if (FMediaObject->state() != Phonon::PlayingState)
			{
				FMediaObject->setCurrentSource(soundFile);
				FMediaObject->play();
			}
# else // phonon
			if (QSound::isAvailable())
			{
				if (!FSound || (FSound && FSound->isFinished()))
				{
					delete FSound;
					FSound = new QSound(soundFile);
					FSound->setLoops(1);
					FSound->play();
				}
			}
#  ifdef Q_WS_X11
			else
			{
				QProcess::startDetached(Options::node(OPV_NOTIFICATIONS_SOUND_COMMAND).value().toString(),QStringList()<<soundFile);
			}
#  endif // x11
# endif // phonon
		}
#endif // mac
	}

	if ((record.notification.kinds & INotification::ShowMinimized)>0)
	{
		QWidget *widget = qobject_cast<QWidget *>((QWidget *)record.notification.data.value(NDR_SHOWMINIMIZED_WIDGET).toLongLong());
		if (widget)
		{
			ITabPage *page = qobject_cast<ITabPage *>(widget);
			if (page)
			{
				if (!FSystemIntegration->isRequestUserAttentionPresent())
					page->showMinimizedTabPage();
			}
			else if (widget->isWindow() && !widget->isVisible())
				widget->showMinimized();
		}
	}

	if ((record.notification.kinds & INotification::AlertWidget)>0)
	{
		QWidget *widget = qobject_cast<QWidget *>((QWidget *)record.notification.data.value(NDR_ALERT_WIDGET).toLongLong());
		if (widget)
		{
			if (FSystemIntegration->isRequestUserAttentionPresent())
				FSystemIntegration->requestUserAttention();
			else
				WidgetManager::alertWidget(widget);
		}
	}

	if ((record.notification.kinds & INotification::TabPageNotify)>0)
	{
		ITabPage *page = qobject_cast<ITabPage *>((QWidget *)record.notification.data.value(NDR_TABPAGE_WIDGET).toLongLong());
		if (page && page->tabPageNotifier())
		{
			ITabPageNotify notify;
			notify.icon = icon;
			notify.iconKey = iconKey;
			notify.iconStorage = iconStorage;
			notify.toolTip = record.notification.data.value(NDR_TABPAGE_TOOLTIP).toString();
			notify.priority = record.notification.data.value(NDR_TABPAGE_PRIORITY).toInt();
			notify.blink = record.notification.data.value(NDR_TABPAGE_ICONBLINK).toBool();
			notify.count = record.notification.data.value(NDR_TABPAGE_NOTIFYCOUNT).toInt();
			notify.styleKey = record.notification.data.value(NDR_TABPAGE_STYLEKEY).toString();
			record.tabPageId = page->tabPageNotifier()->insertNotify(notify);
			record.tabPageNotifier = page->tabPageNotifier()->instance();
		}
	}

	if ((record.notification.kinds & INotification::AutoActivate)>0)
	{
		FDelayedActivations.append(notifyId);
		QTimer::singleShot(0,this,SLOT(onActivateDelayedActivations()));
	}

	if (replaceNotifyId > 0)
	{
		FDelayedReplaces.append(replaceNotifyId);
		QTimer::singleShot(0,this,SLOT(onActivateDelayedReplaces()));
	}

	if ((record.notification.flags & INotification::TestNotify)>0)
	{
		removeNotification(FTestNotifyId);
		FTestNotifyId = notifyId;
		FTestNotifyTimer.start();
	}

	FActivateAll->setVisible(true);

	FNotifyRecords.insert(notifyId,record);
	emit notificationAppended(notifyId, record.notification);

	return notifyId;
}

void Notifications::activateNotification(int ANotifyId)
{
	if (FNotifyRecords.contains(ANotifyId))
	{
		if (FTestNotifyId == ANotifyId)
			removeNotification(FTestNotifyId);
		else
			emit notificationActivated(ANotifyId);
	}
}

void Notifications::removeNotification(int ANotifyId)
{
	if (FNotifyRecords.contains(ANotifyId))
	{
		NotifyRecord record = FNotifyRecords.take(ANotifyId);
		if (FRostersViewPlugin && record.rosterId!=0)
		{
			FRostersViewPlugin->rostersView()->removeNotify(record.rosterId);
		}
		if (!record.tabPageNotifier.isNull())
		{
			ITabPageNotifier *notifier =  qobject_cast<ITabPageNotifier *>(record.tabPageNotifier);
			if (notifier)
				notifier->removeNotify(record.tabPageId);
		}
		if (FTrayManager && record.trayId!=0)
		{
			FTrayManager->removeNotify(record.trayId);
		}
		if (record.popupWidget != NULL)
		{
			record.popupWidget->deleteLater();
		}
		if (FNotifyRecords.isEmpty())
		{
			FActivateAll->setVisible(false);
		}
		qDeleteAll(record.notification.actions);
		emit notificationRemoved(ANotifyId);
	}
}

void Notifications::registerNotificationType(const QString &ATypeId, const INotificationType &AType)
{
	if (!FNotifyTypes.contains(ATypeId))
	{
		FNotifyTypes.insert(ATypeId,AType);
	}
}

QList<QString> Notifications::notificationTypes() const
{
	return FNotifyTypes.keys();
}

INotificationType Notifications::notificationType(const QString &ATypeId) const
{
	return FNotifyTypes.value(ATypeId);
}

void Notifications::removeNotificationType(const QString &ATypeId)
{
	FNotifyTypes.remove(ATypeId);
}

ushort Notifications::enabledNotificationKinds() const
{
	ushort kinds = 0;
	for (ushort kind=FIRST_KIND; kind>0; kind=kind<<1)
	{
		if (Options::node(OPV_NOTIFICATIONS_KINDENABLED_ITEM,QString::number(kind)).value().toBool())
			kinds |= kind;
	}
	return kinds;
}

void Notifications::setEnabledNotificationKinds(ushort AKinds)
{
	for (ushort kind=FIRST_KIND; kind>0; kind=kind<<1)
		Options::node(OPV_NOTIFICATIONS_KINDENABLED_ITEM,QString::number(kind)).setValue((AKinds & kind)>0 ? true : false);
}

ushort Notifications::enabledTypeNotificationKinds(const QString &ATypeId) const
{
	return typeNotificationKinds(ATypeId) & enabledNotificationKinds();
}

ushort Notifications::typeNotificationKinds(const QString &ATypeId) const
{
	if (FNotifyTypes.contains(ATypeId))
	{
		INotificationType notifyType = FNotifyTypes.value(ATypeId);
		return Options::node(OPV_NOTIFICATIONS_TYPEKINDS_ITEM,ATypeId).value().toInt() ^ notifyType.kindDefs;
	}
	return 0;
}

void Notifications::setTypeNotificationKinds(const QString &ATypeId, ushort AKinds)
{
	if (FNotifyTypes.contains(ATypeId))
	{
		INotificationType notifyType = FNotifyTypes.value(ATypeId);
		Options::node(OPV_NOTIFICATIONS_TYPEKINDS_ITEM,ATypeId).setValue((AKinds & notifyType.kindMask) ^ notifyType.kindDefs);
	}
}

QImage Notifications::contactAvatar(const Jid &AStreamJid, const Jid &AContactJid) const
{
	QImage avatar;
	IMetaRoster *mroster = FMetaContacts!=NULL ? FMetaContacts->findMetaRoster(AStreamJid) : NULL;
	QString metaId = mroster!=NULL ? mroster->itemMetaContact(AContactJid) : QString::null;
	if (!metaId.isEmpty())
		avatar = mroster->metaAvatarImage(metaId,false,false);
	else if (FAvatars)
		avatar = FAvatars->avatarImage(AContactJid,false,false);
	return ImageManager::roundSquared(avatar, 36, 3);
}

QIcon Notifications::contactIcon(const Jid &AStreamJid, const Jid &AContactJid) const
{
	return FStatusIcons!=NULL ? FStatusIcons->iconByJid(AStreamJid,AContactJid) : QIcon();
}

QString Notifications::contactName(const Jid &AStreamJid, const Jid &AContactJid) const
{
	IRoster *roster = FRosterPlugin ? FRosterPlugin->findRoster(AStreamJid) : NULL;
	QString name = roster ? roster->rosterItem(AContactJid).name : QString::null;
	if (name.isEmpty())
	{
		Jid legacyJid = FGateways!=NULL ? FGateways->legacyIdFromUserJid(AStreamJid,AContactJid) : AContactJid;
		name = legacyJid.bare();
	}
	return name;
}

int Notifications::notifyIdByRosterId(int ARosterId) const
{
	QMap<int,NotifyRecord>::const_iterator it = FNotifyRecords.constBegin();
	for (; it!=FNotifyRecords.constEnd(); it++)
		if (it.value().rosterId == ARosterId)
			return it.key();
	return -1;
}

int Notifications::notifyIdByTrayId(int ATrayId) const
{
	QMap<int,NotifyRecord>::const_iterator it = FNotifyRecords.constBegin();
	for (; it!=FNotifyRecords.constEnd(); it++)
		if (it.value().trayId == ATrayId)
			return it.key();
	return -1;
}

int Notifications::notifyIdByWidget(NotifyWidget *AWidget) const
{
	QMap<int,NotifyRecord>::const_iterator it = FNotifyRecords.constBegin();
	for (; it!=FNotifyRecords.constEnd(); it++)
		if (it.value().popupWidget == AWidget)
			return it.key();
	return -1;
}

void Notifications::activateAllNotifications(uint kinds)
{
	foreach(int notifyId, FNotifyRecords.keys())
	{
		const NotifyRecord &record = FNotifyRecords.value(notifyId);
		if (record.notification.kinds & kinds)
			activateNotification(notifyId);
	}
}

void Notifications::activateFirstNotification(uint kinds)
{
	bool chatActivated = false;
	foreach(int notifyId, FNotifyRecords.keys())
	{
		const NotifyRecord &record = FNotifyRecords.value(notifyId);
		if (record.notification.kinds & kinds)
		{
			if (!chatActivated)
				activateNotification(notifyId);
			chatActivated = true;
		}
		else
			activateNotification(notifyId);
	}
}

void Notifications::removeAllNotifications()
{
	foreach(int notifyId, FNotifyRecords.keys())
		removeNotification(notifyId);
}

void Notifications::removeInvisibleNotification(int ANotifyId)
{
	NotifyRecord record = FNotifyRecords.value(ANotifyId);
	if (record.notification.flags & INotification::RemoveInvisible)
	{
		bool invisible = true;
		if (record.trayId != 0)
			invisible = false;
		if (record.rosterId != 0)
			invisible = false;
		if (record.tabPageId != 0)
			invisible = false;
		if (!record.popupWidget.isNull())
			invisible = false;
		if (invisible)
			removeNotification(ANotifyId);
	}
}

void Notifications::onActivateDelayedActivations()
{
	foreach(int notifyId, FDelayedActivations)
		activateNotification(notifyId);
	FDelayedActivations.clear();
}

void Notifications::onActivateDelayedReplaces()
{
	foreach(int notifyId, FDelayedReplaces)
		removeNotification(notifyId);
	FDelayedReplaces.clear();
}

void Notifications::onTrayActionTriggered(bool)
{
	Action *action = qobject_cast<Action *>(sender());
	if (action == FActivateAll)
		activateAllNotifications();
}

void Notifications::onRosterNotifyActivated(int ANotifyId)
{
	activateNotification(notifyIdByRosterId(ANotifyId));
}

void Notifications::onRosterNotifyTimeout(int ANotifyId)
{
	int notifyId = notifyIdByRosterId(ANotifyId);
	if (FNotifyRecords.contains(notifyId))
	{
		FNotifyRecords[notifyId].rosterId = 0;
		removeInvisibleNotification(notifyId);
	}
}

void Notifications::onRosterNotifyRemoved(int ANotifyId)
{
	int notifyId = notifyIdByRosterId(ANotifyId);
	if (FNotifyRecords.contains(notifyId))
		FNotifyRecords[notifyId].rosterId = 0;
}

void Notifications::onTrayNotifyActivated(int ANotifyId, QSystemTrayIcon::ActivationReason AReason)
{
	if (ANotifyId>0 && AReason==QSystemTrayIcon::DoubleClick)
		activateAllNotifications();
}

void Notifications::onTrayNotifyRemoved(int ANotifyId)
{
	int notifyId = notifyIdByTrayId(ANotifyId);
	if (FNotifyRecords.contains(notifyId))
		FNotifyRecords[notifyId].trayId = 0;
}

void Notifications::onWindowNotifyActivated()
{
	activateNotification(notifyIdByWidget(qobject_cast<NotifyWidget*>(sender())));
}

void Notifications::onWindowNotifyRemoved()
{
	removeNotification(notifyIdByWidget(qobject_cast<NotifyWidget*>(sender())));
}

void Notifications::onWindowNotifyOptions()
{
	if (FOptionsManager)
		FOptionsManager->showOptionsDialog(OPN_NOTIFICATIONS);
}

void Notifications::onWindowNotifyDestroyed()
{
	int notifyId = notifyIdByWidget(qobject_cast<NotifyWidget*>(sender()));
	if (FNotifyRecords.contains(notifyId))
	{
		FNotifyRecords[notifyId].popupWidget = NULL;
		removeInvisibleNotification(notifyId);
	}
}

void Notifications::onTestNotificationTimerTimedOut()
{
	removeNotification(FTestNotifyId);
}

void Notifications::onSystemNotifyClicked(int ANotifyId)
{
	activateNotification(ANotifyId);
}

void Notifications::onShowSystemNotificationsSettings()
{
	if (FSystemIntegration)
		FSystemIntegration->showSystemNotificationsSettings();
}

void Notifications::onNotifyCountChanged()
{
	if (FSystemIntegration)
	{
		int count = 0;
		foreach (int i, FNotifyRecords.keys())
		{
			if (FNotifyRecords.value(i).notification.kinds & INotification::DockBadge)
				count++;
		}
		FSystemIntegration->setDockBadge(count ? QString::number(count) : QString::null);
	}
}

void Notifications::onDockClicked()
{
	bool haveUnreadNotifications = false;
	foreach (int id, FNotifyRecords.keys())
		if (FNotifyRecords.value(id).notification.kinds & INotification::DockBadge)
		{
			haveUnreadNotifications = true;
			break;
		}
	activateFirstNotification(INotification::DockBadge);
	if (!haveUnreadNotifications && FMainWindow)
		if (!FMainWindow->mainWindow()->instance()->isVisible())
			FMainWindow->showMainWindow();
}


Q_EXPORT_PLUGIN2(plg_notifications, Notifications)
