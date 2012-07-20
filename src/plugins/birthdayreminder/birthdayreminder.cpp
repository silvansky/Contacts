#include "birthdayreminder.h"

#include <QSet>
#include <QStyle>
#include <QTimer>
#include <QPainter>
#include <QTextDocument>
#include <QDesktopServices>

#define NOTIFY_WITHIN_DAYS 7
#define NOTIFY_TIMEOUT     90000

BirthdayReminder::BirthdayReminder()
{
	FAvatars = NULL;
	FVCardPlugin = NULL;
	FRosterPlugin = NULL;
	FPresencePlugin = NULL;
	FRostersModel = NULL;
	FMetaContacts = NULL;
	FNotifications = NULL;
	FNotifications = NULL;
	FRostersViewPlugin = NULL;
	FMessageProcessor = NULL;

	FNotifyTimer.setSingleShot(false);
	FNotifyTimer.setInterval(NOTIFY_TIMEOUT);
	connect(&FNotifyTimer,SIGNAL(timeout()),SLOT(onShowNotificationTimer()));
}

BirthdayReminder::~BirthdayReminder()
{

}

void BirthdayReminder::pluginInfo(IPluginInfo *APluginInfo)
{
	APluginInfo->name = tr("Birthday Reminder");
	APluginInfo->description = tr("Reminds about birthdays of your friends");
	APluginInfo->version = "1.0";
	APluginInfo->author = "Potapov S.A. aka Lion";
	APluginInfo->homePage = "http://contacts.rambler.ru";
	APluginInfo->dependences.append(VCARD_UUID);
}

bool BirthdayReminder::initConnections(IPluginManager *APluginManager, int &AInitOrder)
{
	Q_UNUSED(AInitOrder);

	IPlugin *plugin = APluginManager->pluginInterface("IVCardPlugin").value(0,NULL);
	if (plugin)
	{
		FVCardPlugin = qobject_cast<IVCardPlugin *>(plugin->instance());
		if (FVCardPlugin)
		{
			connect(FVCardPlugin->instance(),SIGNAL(vcardReceived(const Jid &)),SLOT(onVCardReceived(const Jid &)));
		}
	}

	plugin = APluginManager->pluginInterface("IAvatars").value(0,NULL);
	if (plugin)
	{
		FAvatars = qobject_cast<IAvatars *>(plugin->instance());
	}

	plugin = APluginManager->pluginInterface("IRostersViewPlugin").value(0,NULL);
	if (plugin)
	{
		FRostersViewPlugin = qobject_cast<IRostersViewPlugin *>(plugin->instance());
		if (FRostersViewPlugin)
		{
			connect(FRostersViewPlugin->rostersView()->instance(),SIGNAL(labelToolTips(IRosterIndex *, int, QMultiMap<int,QString> &, ToolBarChanger*)),
				SLOT(onRosterLabelToolTips(IRosterIndex *, int, QMultiMap<int,QString> &, ToolBarChanger *)));
		}
	}

	plugin = APluginManager->pluginInterface("IRosterPlugin").value(0,NULL);
	if (plugin)
	{
		FRosterPlugin = qobject_cast<IRosterPlugin *>(plugin->instance());
		if (FRosterPlugin)
		{
			connect(FRosterPlugin->instance(),SIGNAL(rosterItemReceived(IRoster *, const IRosterItem &, const IRosterItem &)),
				SLOT(onRosterItemReceived(IRoster *, const IRosterItem &, const IRosterItem &)));
		}
	}

	plugin = APluginManager->pluginInterface("IMetaContacts").value(0,NULL);
	if (plugin)
	{
		FMetaContacts = qobject_cast<IMetaContacts *>(plugin->instance());
	}

	plugin = APluginManager->pluginInterface("IPresencePlugin").value(0,NULL);
	if (plugin)
	{
		FPresencePlugin = qobject_cast<IPresencePlugin *>(plugin->instance());
	}

	plugin = APluginManager->pluginInterface("IRostersModel").value(0,NULL);
	if (plugin)
	{
		FRostersModel = qobject_cast<IRostersModel *>(plugin->instance());
	}

	plugin = APluginManager->pluginInterface("IMessageProcessor").value(0,NULL);
	if (plugin)
	{
		FMessageProcessor = qobject_cast<IMessageProcessor *>(plugin->instance());
	}

	plugin = APluginManager->pluginInterface("INotifications").value(0,NULL);
	if (plugin)
	{
		FNotifications = qobject_cast<INotifications *>(plugin->instance());
		if (FNotifications)
		{
			connect(FNotifications->instance(),SIGNAL(notificationActivated(int)), SLOT(onNotificationActivated(int)));
			connect(FNotifications->instance(),SIGNAL(notificationRemoved(int)), SLOT(onNotificationRemoved(int)));
			connect(FNotifications->instance(),SIGNAL(notificationTest(const QString &, ushort)),SLOT(onNotificationTest(const QString &, ushort)));
		}
	}

	plugin = APluginManager->pluginInterface("IMainWindowPlugin").value(0,NULL);
	if (plugin)
	{
		FMainWindowPlugin = qobject_cast<IMainWindowPlugin *>(plugin->instance());
		if (FMainWindowPlugin)
		{
			connect(FMainWindowPlugin->mainWindow()->noticeWidget()->instance(),SIGNAL(noticeWidgetReady()),SLOT(onInternalNoticeReady()));
			connect(FMainWindowPlugin->mainWindow()->noticeWidget()->instance(),SIGNAL(noticeRemoved(int)),SLOT(onInternalNoticeRemoved(int)));
		}
	}

	connect(Options::instance(),SIGNAL(optionsOpened()),SLOT(onOptionsOpened()));
	connect(Options::instance(),SIGNAL(optionsClosed()),SLOT(onOptionsClosed()));

	return FVCardPlugin!=NULL;
}

bool BirthdayReminder::initObjects()
{
	if (FRostersModel)
	{
		FRostersModel->insertDefaultDataHolder(this);
	}
	if (FNotifications)
	{
		INotificationType notifyType;
		notifyType.order = OWO_NOTIFICATIONS_BIRTHDAY;
		notifyType.title = tr("Birthdays");
		notifyType.kindMask = INotification::PopupWindow|INotification::SoundPlay;
		notifyType.kindDefs = 0;
		FNotifications->registerNotificationType(NNT_BIRTHDAY_REMIND,notifyType);
	}

	QIcon cake = IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getIcon(MNI_BIRTHDAYREMINDER_AVATAR_CAKE);
	FAvatarCake = cake.pixmap(cake.availableSizes().value(0));

	return true;
}

bool BirthdayReminder::startPlugin()
{
	FNotifyTimer.start();
	return true;
}

int BirthdayReminder::rosterDataOrder() const
{
	return RDHO_BIRTHDAY_AVATAR;
}

QList<int> BirthdayReminder::rosterDataRoles() const
{
	static QList<int> roles = QList<int>() << RDR_AVATAR_IMAGE << RDR_AVATAR_IMAGE_LARGE;
	return roles;
}

QList<int> BirthdayReminder::rosterDataTypes() const
{
	QList<int> types = QList<int>() << RIT_CONTACT << RIT_METACONTACT;
	return types;
}

QVariant BirthdayReminder::rosterData(const IRosterIndex *AIndex, int ARole) const
{
	QVariant data;
	if (ARole==RDR_AVATAR_IMAGE || ARole==RDR_AVATAR_IMAGE_LARGE)
	{
		QList<Jid> metaItems;
		if (AIndex->type() == RIT_METACONTACT)
		{
			foreach(QString itemJid, AIndex->data(RDR_METACONTACT_ITEMS).toStringList())
				metaItems.append(itemJid);
		}
		else if (AIndex->type() == RIT_CONTACT)
		{
			metaItems.append(AIndex->data(RDR_PREP_BARE_JID).toString());
		}

		foreach(Jid contactJid, metaItems)
		{
			if (FUpcomingBirthdays.value(contactJid.bare(),-1) == 0)
			{
				static bool blocked = false;
				if (!blocked)
				{
					blocked = true;
					QImage avatar = AIndex->data(ARole).value<QImage>();
					if (!avatar.isNull())
						data = avatarWithCake(contactJid,avatar);
					blocked = false;
				}
				break;
			}
		}
	}
	return data;
}

bool BirthdayReminder::setRosterData(IRosterIndex *AIndex, int ARole, const QVariant &AValue)
{
	Q_UNUSED(AIndex);
	Q_UNUSED(ARole);
	Q_UNUSED(AValue);
	return false;
}

QDate BirthdayReminder::contactBithday(const Jid &AContactJid) const
{
	return FBirthdays.value(AContactJid.bare());
}

int BirthdayReminder::contactBithdayDaysLeft(const Jid &AContactJid) const
{
	QDate birthday = contactBithday(AContactJid);
	if (birthday.isValid())
	{
		QDate curDate = QDate::currentDate();
		if (curDate.month()<birthday.month() || (curDate.month()==birthday.month() && curDate.day()<=birthday.day()))
			birthday.setDate(curDate.year(),birthday.month(),birthday.day());
		else
			birthday.setDate(curDate.year()+1,birthday.month(),birthday.day());
		return curDate.daysTo(birthday);
	}
	return -1;
}

QImage BirthdayReminder::avatarWithCake(const Jid &AContactJid, const QImage &AAvatar) const
{
	QImage avatar = AAvatar;
	if (FAvatars && avatar.isNull())
		avatar = FAvatars->avatarImage(AContactJid,false,false);

	if (!FAvatarCake.isNull())
	{
		QRect cakeRect = QStyle::alignedRect(Qt::LeftToRight,Qt::AlignLeft|Qt::AlignBottom,FAvatarCake.size().boundedTo(avatar.size()/2),avatar.rect());
		QPainter painter(&avatar);
		painter.setOpacity(0.8);
		painter.drawPixmap(cakeRect,FAvatarCake);
	}

	return avatar;
}

IInternalNotice BirthdayReminder::internalNoticeTemplate() const
{
	IInternalNotice notice;
	notice.priority = INP_DEFAULT;
	notice.iconKey = MNI_BIRTHDAYREMINDER_NOTICE;
	notice.iconStorage = RSR_STORAGE_MENUICONS;
	notice.caption = tr("Friends birthdays!");
	notice.message = tr("Enable a reminder to not miss the birthdays of your friends!");
	return notice;
}

Jid BirthdayReminder::findContactStream(const Jid &AContactJid) const
{
	if (FRostersModel && FRosterPlugin)
	{
		foreach(Jid streamJid, FRostersModel->streams())
		{
			IRoster *roster = FRosterPlugin->findRoster(streamJid);
			if (roster && roster->rosterItem(AContactJid).isValid)
				return streamJid;
		}
	}
	return Jid::null;
}

void BirthdayReminder::updateBirthdaysStates()
{
	if (FNotifyDate != QDate::currentDate())
	{
		FNotifiedContacts.clear();
		FNotifyDate = QDate::currentDate();

		foreach(Jid contactJid, FBirthdays.keys()) {
			updateBirthdayState(contactJid); }
	}
}

bool BirthdayReminder::updateBirthdayState(const Jid &AContactJid)
{
	bool notify = false;
	int daysLeft = contactBithdayDaysLeft(AContactJid);

	bool isStateChanged = false;
	if (daysLeft>=0 && daysLeft<=NOTIFY_WITHIN_DAYS)
	{
		notify = true;
		isStateChanged = !FUpcomingBirthdays.contains(AContactJid);
		FUpcomingBirthdays.insert(AContactJid,daysLeft);
	}
	else
	{
		isStateChanged = FUpcomingBirthdays.contains(AContactJid);
		FUpcomingBirthdays.remove(AContactJid);
	}

	if (FRostersModel && isStateChanged)
	{
		IMetaRoster *mroster = FMetaContacts!=NULL ? FMetaContacts->findMetaRoster(findContactStream(AContactJid)) : NULL;
		QString metaId = mroster!=NULL && mroster->isEnabled() ? mroster->itemMetaContact(AContactJid) : QString::null;

		QMultiMap<int, QVariant> findData;
		if (!metaId.isEmpty())
		{
			findData.insert(RDR_TYPE,RIT_METACONTACT);
			findData.insert(RDR_META_ID,metaId);
		}
		else
		{
			findData.insert(RDR_TYPE,RIT_CONTACT);
			findData.insert(RDR_PREP_BARE_JID,AContactJid.pBare());
		}
		foreach(IRosterIndex *index, FRostersModel->rootIndex()->findChilds(findData,true))
		{
			emit rosterDataChanged(index,RDR_AVATAR_IMAGE); 
			emit rosterDataChanged(index,RDR_AVATAR_IMAGE_LARGE); 
		}
	}

	return notify;
}

void BirthdayReminder::setContactBithday(const Jid &AContactJid, const QDate &ABirthday)
{
	Jid contactJid = AContactJid.bare();
	if (ABirthday.isValid())
		FBirthdays.insert(contactJid,ABirthday);
	else
		FBirthdays.remove(contactJid);
	updateBirthdayState(contactJid);
}

void BirthdayReminder::onShowNotificationTimer()
{
	if (FNotifications && FNotifications->notifications().isEmpty())
	{
		INotification notify;
		notify.kinds = FNotifications->enabledTypeNotificationKinds(NNT_BIRTHDAY_REMIND);
		if ((notify.kinds & (INotification::PopupWindow|INotification::SoundPlay))>0)
		{
			updateBirthdaysStates();
			notify.typeId = NNT_BIRTHDAY_REMIND;

			QSet<QString> notifiedMetaContacts;
			QSet<Jid> notifyList = FUpcomingBirthdays.keys().toSet() - FNotifiedContacts.toSet();
			foreach(Jid contactJid, notifyList)
			{
				Jid streamJid = findContactStream(contactJid);
				IMetaRoster *mroster = FMetaContacts!=NULL ? FMetaContacts->findMetaRoster(streamJid) : NULL;
				QString metaId = mroster!=NULL && mroster->isEnabled() ? mroster->itemMetaContact(contactJid) : QString::null;
				if (metaId.isEmpty() || !notifiedMetaContacts.contains(metaId))
				{
					notifiedMetaContacts += metaId;

					notify.data.insert(NDR_POPUP_TITLE,!metaId.isEmpty() ? FMetaContacts->metaContactName(mroster->metaContact(metaId)) : FNotifications->contactName(streamJid,contactJid));
					notify.data.insert(NDR_POPUP_IMAGE,FNotifications->contactAvatar(streamJid,contactJid));
					notify.data.insert(NDR_POPUP_STYLEKEY,STS_NOTIFICATION_NOTIFYWIDGET);

					QDate	birthday = contactBithday(contactJid);
					int daysLeft = FUpcomingBirthdays.value(contactJid);
					QString text = daysLeft>0 ? tr("Birthday in %n day(s), %1","",daysLeft).arg(birthday.toString(Qt::SystemLocaleLongDate)) : tr("Birthday today!");
					notify.data.insert(NDR_POPUP_TEXT,text);

					Action *action = new Action(NULL);
					action->setText(tr("Congratulate with postcard"));
					action->setData(Action::DR_UserDefined + 1, "birthday");
					connect(action,SIGNAL(triggered()),SLOT(onCongratulateWithPostcard()));

					notify.actions.clear();
					notify.actions.append(action);
					FNotifies.insert(FNotifications->appendNotification(notify),contactJid);
				}
				FNotifiedContacts.append(contactJid);
			}
		}
	}
}

void BirthdayReminder::onCongratulateWithPostcard()
{
	QDesktopServices::openUrl(QUrl("http://cards.rambler.ru"));
}

void BirthdayReminder::onNotificationActivated(int ANotifyId)
{
	if (FNotifies.contains(ANotifyId))
	{
		if (FMessageProcessor)
		{
			Jid contactJid = FNotifies.value(ANotifyId);
			Jid streamJid = findContactStream(contactJid);
			IPresence *presence = FPresencePlugin!=NULL ? FPresencePlugin->findPresence(streamJid) : NULL;
			QList<IPresenceItem> presences = presence!=NULL ? presence->presenceItems(contactJid) : QList<IPresenceItem>();
			FMessageProcessor->createMessageWindow(streamJid, !presences.isEmpty() ? presences.first().itemJid : contactJid, Message::Chat, IMessageHandler::SM_SHOW);
		}
		FNotifications->removeNotification(ANotifyId);
	}
}

void BirthdayReminder::onNotificationRemoved(int ANotifyId)
{
	if (FNotifies.contains(ANotifyId))
	{
		FNotifies.remove(ANotifyId);
	}
}

void BirthdayReminder::onNotificationTest(const QString &ATypeId, ushort AKinds)
{
	if (ATypeId == NNT_BIRTHDAY_REMIND)
	{
		INotification notify;
		notify.kinds = AKinds;
		notify.typeId = ATypeId;
		notify.flags |= INotification::TestNotify;
		if (AKinds & INotification::PopupWindow)
		{
			Jid contactJid = "vasilisa@rambler/ramblercontacts";
         notify.data.insert(NDR_POPUP_IMAGE,FNotifications->contactAvatar(Jid::null,contactJid.full()));
			notify.data.insert(NDR_POPUP_TITLE,tr("Vasilisa Premudraya"));
			notify.data.insert(NDR_POPUP_TEXT,tr("Birthday today!"));
			notify.data.insert(NDR_POPUP_STYLEKEY,STS_NOTIFICATION_NOTIFYWIDGET);

			Action *action = new Action(NULL);
			action->setText(tr("Congratulate with postcard"));
			action->setData(Action::DR_UserDefined + 1, "birthday");
			notify.actions.append(action);
		}
		if (AKinds & INotification::SoundPlay)
		{
			notify.data.insert(NDR_SOUND_FILE,SDF_BITHDAY_REMIND);
		}
		if (!notify.data.isEmpty())
		{
			FNotifications->appendNotification(notify);
		}
	}
}

void BirthdayReminder::onInternalNoticeReady()
{
	IInternalNoticeWidget *widget = FMainWindowPlugin->mainWindow()->noticeWidget();
	if (FNotifications && widget->isEmpty())
	{
		if ((FNotifications->enabledTypeNotificationKinds(NNT_BIRTHDAY_REMIND) & (INotification::PopupWindow|INotification::SoundPlay)) == 0)
		{
			int showCount = Options::node(OPV_BIRTHDAY_NOTICE_SHOWCOUNT).value().toInt();
			QDateTime showLast = Options::node(OPV_BIRTHDAY_NOTICE_SHOWLAST).value().toDateTime();
			if (showCount < 2 && (!showLast.isValid() || showLast.daysTo(QDateTime::currentDateTime())>=60))
			{
				IInternalNotice notice = internalNoticeTemplate();

				Action *action = new Action(this);
				action->setText(tr("Enable reminder"));
				connect(action,SIGNAL(triggered()),SLOT(onInternalNoticeActionTriggered()));
				notice.actions.append(action);

				FInternalNoticeId = widget->insertNotice(notice);
				Options::node(OPV_BIRTHDAY_NOTICE_SHOWCOUNT).setValue(showCount+1);
				Options::node(OPV_BIRTHDAY_NOTICE_SHOWLAST).setValue(QDateTime::currentDateTime());
			}
		}
	}
}

void BirthdayReminder::onInternalNoticeActionTriggered()
{
	IInternalNotice notice = internalNoticeTemplate();
	notice.message += "<br><br>";
	notice.message += QString("<span align='center' style='color:green;'>%1</span>").arg(tr("Reminder enabled"));

	IInternalNoticeWidget *widget = FMainWindowPlugin->mainWindow()->noticeWidget();
	FInternalNoticeId = widget->insertNotice(notice);

	FNotifications->setTypeNotificationKinds(NNT_BIRTHDAY_REMIND,INotification::PopupWindow|INotification::SoundPlay);

	QTimer::singleShot(2000,this,SLOT(onInternalNoticeRemove()));
}

void BirthdayReminder::onInternalNoticeRemove()
{
	IInternalNoticeWidget *widget = FMainWindowPlugin->mainWindow()->noticeWidget();
	widget->removeNotice(FInternalNoticeId);
}

void BirthdayReminder::onInternalNoticeRemoved(int ANoticeId)
{
	if (ANoticeId>0 && ANoticeId==FInternalNoticeId)
	{
		FInternalNoticeId = -1;
	}
}

void BirthdayReminder::onRosterLabelToolTips(IRosterIndex *AIndex, int ALabelId, QMultiMap<int,QString> &AToolTips, ToolBarChanger *AToolBarChanger)
{
	Q_UNUSED(AToolBarChanger);
	if (ALabelId == RLID_DISPLAY)
	{
		QList<Jid> metaItems;
		if (AIndex->type() == RIT_METACONTACT)
		{
			foreach(QString itemJid, AIndex->data(RDR_METACONTACT_ITEMS).toStringList())
				metaItems.append(itemJid);
		}
		else if (AIndex->type() == RIT_CONTACT)
		{
			metaItems.append(AIndex->data(RDR_PREP_BARE_JID).toString());
		}

		int daysLeft = -1;
		foreach(Jid itemJid, metaItems)
		{
			int itemDaysLeft = contactBithdayDaysLeft(itemJid);
			if (daysLeft<0 || daysLeft<itemDaysLeft)
				daysLeft = itemDaysLeft;
		}

		if (daysLeft>=0 && daysLeft<=NOTIFY_WITHIN_DAYS)
		{
			QString tip = QString("<span style='color:green'>%1</span>");
			tip = tip.arg(daysLeft>0 ? tr("Birthday in %n day(s)!","",daysLeft) : tr("Birthday today!"));
			tip += "<br>";
			tip += QString("<a href='%1'>%2</a>").arg("http://cards.rambler.ru").arg(tr("Congratulate with postcard"));
			AToolTips.insert(RTTO_BIRTHDAY_NOTIFY,tip);
		}
	}
}

void BirthdayReminder::onVCardReceived(const Jid &AContactJid)
{
	if (findContactStream(AContactJid).isValid())
	{
		IVCard *vcard = FVCardPlugin->vcard(AContactJid);
		setContactBithday(AContactJid,DateTime(vcard->value(VVN_BIRTHDAY)).dateTime().date());
		vcard->unlock();
	}
}

void BirthdayReminder::onRosterItemReceived(IRoster *ARoster, const IRosterItem &AItem, const IRosterItem &ABefore)
{
	Q_UNUSED(ARoster);
	if (FVCardPlugin)
	{
		if (AItem.subscription!=SUBSCRIPTION_REMOVE && !ABefore.isValid)
		{
			if (FVCardPlugin->hasVCard(AItem.itemJid))
			{
				IVCard *vcard = FVCardPlugin->vcard(AItem.itemJid);
				setContactBithday(AItem.itemJid,DateTime(vcard->value(VVN_BIRTHDAY)).dateTime().date());
				vcard->unlock();
			}
		}
		else if (AItem.subscription == SUBSCRIPTION_REMOVE)
		{
			setContactBithday(AItem.itemJid,QDate());
		}
	}
}

void BirthdayReminder::onOptionsOpened()
{
	FNotifyDate = Options::fileValue("birthdays.notify.date").toDate();
	QStringList notified = Options::fileValue("birthdays.notify.notified").toStringList();

	FNotifiedContacts.clear();
	foreach(QString contactJid, notified)
		FNotifiedContacts.append(contactJid);
}

void BirthdayReminder::onOptionsClosed()
{
	QStringList notified;
	foreach (Jid contactJid, FNotifiedContacts)
		notified.append(contactJid.bare());

	Options::setFileValue(FNotifyDate,"birthdays.notify.date");
	Options::setFileValue(notified,"birthdays.notify.notified");
}

Q_EXPORT_PLUGIN2(plg_birthdayreminder, BirthdayReminder)
