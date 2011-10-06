#include "notifications.h"

#include <QProcess>
#include <QVBoxLayout>
#include <utils/imagemanager.h>
#include "notifykindswidgets.h"

#define TEST_NOTIFY_TIMEOUT             10000

#define ADR_NOTIFYID                    Action::DR_Parametr1

Notifications::Notifications()
{
	FAvatars = NULL;
	FRosterPlugin = NULL;
	FMetaContacts = NULL;
	FStatusIcons = NULL;
	FStatusChanger = NULL;
	FTrayManager = NULL;
	FRostersModel = NULL;
	FRostersViewPlugin = NULL;
	FOptionsManager = NULL;

	FNotifyId = 0;
	FTestNotifyId = -1;
	FActivateAll = NULL;

#ifdef QT_PHONON_LIB
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
#ifdef QT_PHONON_LIB
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

	if (FRostersModel && FRostersViewPlugin && (record.notification.kinds & INotification::RosterNotify)>0 &&
		Options::node(OPV_NOTIFICATIONS_KINDENABLED_ITEM,QString::number(INotification::RosterNotify)).value().toBool())
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

	if (!blockPopupAndSound && (record.notification.kinds & INotification::PopupWindow)>0 &&
		Options::node(OPV_NOTIFICATIONS_KINDENABLED_ITEM,QString::number(INotification::PopupWindow)).value().toBool())
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

	if (FTrayManager)
	{
		QString toolTip = record.notification.data.value(NDR_TRAY_TOOLTIP).toString();

		if ((record.notification.kinds & INotification::TrayNotify)>0 &&
			Options::node(OPV_NOTIFICATIONS_KINDENABLED_ITEM,QString::number(INotification::TrayNotify)).value().toBool())
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

	if (!blockPopupAndSound && (record.notification.kinds & INotification::SoundPlay)>0 &&
		Options::node(OPV_NOTIFICATIONS_KINDENABLED_ITEM,QString::number(INotification::SoundPlay)).value().toBool())
	{
		QString soundName = record.notification.data.value(NDR_SOUND_FILE).toString();
		QString soundFile = FileStorage::staticStorage(RSR_STORAGE_SOUNDS)->fileFullName(soundName);
		if (!soundFile.isEmpty())
		{
#ifdef QT_PHONON_LIB
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
#else
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
#	ifdef Q_WS_X11
			else
			{
				QProcess::startDetached(Options::node(OPV_NOTIFICATIONS_SOUND_COMMAND).value().toString(),QStringList()<<soundFile);
			}
#	endif
#endif
		}
	}

	if ((record.notification.kinds & INotification::ShowMinimized)>0 &&
		Options::node(OPV_NOTIFICATIONS_KINDENABLED_ITEM,QString::number(INotification::ShowMinimized)).value().toBool())
	{
		QWidget *widget = qobject_cast<QWidget *>((QWidget *)record.notification.data.value(NDR_SHOWMINIMIZED_WIDGET).toLongLong());
		if (widget)
		{
			ITabPage *page = qobject_cast<ITabPage *>(widget);
			if (page)
#ifdef Q_WS_MAC
				page->showTabPage();
#else
				page->showMinimizedTabPage();
#endif
			else if (widget->isWindow() && !widget->isVisible())
				widget->showMinimized();
		}
	}

	if ((record.notification.kinds & INotification::AlertWidget)>0 &&
		Options::node(OPV_NOTIFICATIONS_KINDENABLED_ITEM,QString::number(INotification::AlertWidget)).value().toBool())
	{
		QWidget *widget = qobject_cast<QWidget *>((QWidget *)record.notification.data.value(NDR_ALERT_WIDGET).toLongLong());
		if (widget)
			WidgetManager::alertWidget(widget);
	}

	if ((record.notification.kinds & INotification::TabPageNotify)>0 &&
		Options::node(OPV_NOTIFICATIONS_KINDENABLED_ITEM,QString::number(INotification::TabPageNotify)).value().toBool())
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

	if ((record.notification.kinds & INotification::AutoActivate)>0 &&
		Options::node(OPV_NOTIFICATIONS_KINDENABLED_ITEM,QString::number(INotification::AutoActivate)).value().toBool())
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

ushort Notifications::notificationKinds(const QString &ATypeId) const
{
	if (FNotifyTypes.contains(ATypeId))
	{
		INotificationType notifyType = FNotifyTypes.value(ATypeId);
		return Options::node(OPV_NOTIFICATIONS_TYPEKINDS_ITEM,ATypeId).value().toInt() ^ notifyType.kindDefs;
	}
	return 0;
}

void Notifications::setNotificationKinds(const QString &ATypeId, ushort AKinds)
{
	if (FNotifyTypes.contains(ATypeId))
	{
		INotificationType notifyType = FNotifyTypes.value(ATypeId);
		Options::node(OPV_NOTIFICATIONS_TYPEKINDS_ITEM,ATypeId).setValue((AKinds & notifyType.kindMask) ^ notifyType.kindDefs);
	}
}

void Notifications::removeNotificationType(const QString &ATypeId)
{
	FNotifyTypes.remove(ATypeId);
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
	return ImageManager::roundSquared(avatar, 36, 2);
}

QIcon Notifications::contactIcon(const Jid &AStreamJid, const Jid &AContactJid) const
{
	return FStatusIcons!=NULL ? FStatusIcons->iconByJid(AStreamJid,AContactJid) : QIcon();
}

QString Notifications::contactName(const Jid &AStreamJId, const Jid &AContactJid) const
{
	IRoster *roster = FRosterPlugin ? FRosterPlugin->findRoster(AStreamJId) : NULL;
	QString name = roster ? roster->rosterItem(AContactJid).name : AContactJid.node();
	if (name.isEmpty())
		name = AContactJid.bare();
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

void Notifications::activateAllNotifications()
{
	bool chatActivated = false;
	foreach(int notifyId, FNotifyRecords.keys())
	{
		const NotifyRecord &record = FNotifyRecords.value(notifyId);
		if (record.notification.kinds & INotification::TabPageNotify)
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

Q_EXPORT_PLUGIN2(plg_notifications, Notifications)
