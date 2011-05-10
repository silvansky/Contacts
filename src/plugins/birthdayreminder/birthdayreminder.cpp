#include "birthdayreminder.h"

#include <QSet>
#include <QStyle>
#include <QTimer>
#include <QPainter>
#include <QTextDocument>
#include <QDesktopServices>

#define NOTIFY_WITHIN_DAYS 7
#define NOTIFY_TIMEOUT     90000

#define ADR_CONTACT_JID    Action::DR_Parametr1

BirthdayReminder::BirthdayReminder()
{
	FAvatars = NULL;
	FVCardPlugin = NULL;
	FRosterPlugin = NULL;
	FPresencePlugin = NULL;
	FRostersModel = NULL;
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
	APluginInfo->homePage = "http://friends.rambler.ru";
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
			connect(FNotifications->instance(),SIGNAL(notificationTest(const QString &, uchar)),SLOT(onNotificationTest(const QString &, uchar)));
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
		uchar kindMask = INotification::PopupWindow|INotification::PlaySoundNotification|INotification::TestNotify;
		FNotifications->insertNotificator(NID_BIRTHDAY_REMIND,OWO_NOTIFICATIONS_BIRTHDAY,tr("Birthdays"),kindMask,0);
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
	static QList<int> roles = QList<int>() << RDR_AVATAR_IMAGE;
	return roles;
}

QList<int> BirthdayReminder::rosterDataTypes() const
{
	QList<int> types = QList<int>() << RIT_CONTACT;
	return types;
}

QVariant BirthdayReminder::rosterData(const IRosterIndex *AIndex, int ARole) const
{
	QVariant data;
	if (ARole == RDR_AVATAR_IMAGE)
	{
		Jid contactJid = AIndex->data(RDR_FULL_JID).toString();
		if (FUpcomingBirthdays.value(contactJid.bare(),-1) == 0)
		{
			static bool blocked = false;
			if (!blocked)
			{
				blocked = true;
				QImage avatar = AIndex->data(RDR_AVATAR_IMAGE).value<QImage>();
				if (!avatar.isNull())
				{
					data = avatarWithCake(contactJid,avatar);
				}
				blocked = false;
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

	if (!FAvatarCake.isNull() && FUpcomingBirthdays.value(AContactJid.bare(),-1)==0)
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
			IRoster *roster = FRosterPlugin->getRoster(streamJid);
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

	bool isAvatarChanged = false;
	if (daysLeft>=0 && daysLeft<=NOTIFY_WITHIN_DAYS)
	{
		notify = true;
		if ((daysLeft==0 || FUpcomingBirthdays.value(AContactJid,-1)==0) && FUpcomingBirthdays.value(AContactJid,-1)!=daysLeft)
			isAvatarChanged = true;
		FUpcomingBirthdays.insert(AContactJid,daysLeft);
	}
	else
	{
		if (FUpcomingBirthdays.value(AContactJid,-1) == 0)
			isAvatarChanged = true;
		FUpcomingBirthdays.remove(AContactJid);
	}

	if  (FRostersModel && isAvatarChanged)
	{
		QMultiMap<int, QVariant> findData;
		findData.insert(RDR_TYPE, RIT_CONTACT);
		findData.insert(RDR_PREP_BARE_JID,AContactJid.pBare());
		FRostersModel->rootIndex()->findChilds(findData,true);
		foreach(IRosterIndex *index, FRostersModel->rootIndex()->findChilds(findData,true))
		{
			emit rosterDataChanged(index,RDR_AVATAR_IMAGE);
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
		notify.kinds = FNotifications->notificatorKinds(NID_BIRTHDAY_REMIND);
		if ((notify.kinds & (INotification::PopupWindow|INotification::PlaySoundNotification))>0)
		{
			updateBirthdaysStates();
			notify.notificatior = NID_BIRTHDAY_REMIND;
			QSet<Jid> notifyList = FUpcomingBirthdays.keys().toSet() - FNotifiedContacts.toSet();
			foreach(Jid contactJid, notifyList)
			{
				Jid streamJid = findContactStream(contactJid);

				notify.data.insert(NDR_POPUP_CAPTION,tr("Birthday of"));
				notify.data.insert(NDR_POPUP_IMAGE,FNotifications->contactAvatar(contactJid.full()));
				notify.data.insert(NDR_POPUP_TITLE,FNotifications->contactName(streamJid,contactJid));
				notify.data.insert(NDR_POPUP_STYLEKEY,STS_NOTIFICATION_NOTIFYWIDGET);

				QDate	birthday = contactBithday(contactJid);
				int daysLeft = FUpcomingBirthdays.value(contactJid);
				QString text = daysLeft>0 ? tr("Birthday in %n day(s),<br> %1","",daysLeft).arg(birthday.toString(Qt::SystemLocaleLongDate)) : tr("Birthday <b>today</b>!");
				notify.data.insert(NDR_POPUP_TEXT,text);

				Action *action = new Action(NULL);
				action->setText(tr("Congratulate with postcard"));
				action->setData(ADR_CONTACT_JID, contactJid.bare());
				action->setData(Action::DR_UserDefined + 1, "birthday");
				connect(action,SIGNAL(triggered()),SLOT(onCongratulateWithPostcard()));
				notify.actions.append(action);

				FNotifiedContacts.append(contactJid);
				FNotifies.insert(FNotifications->appendNotification(notify),contactJid);
			}
		}
	}
}

void BirthdayReminder::onCongratulateWithPostcard()
{
	QDesktopServices::openUrl(QUrl("http://id.rambler.ru"));
}

void BirthdayReminder::onNotificationActivated(int ANotifyId)
{
	if (FNotifies.contains(ANotifyId))
	{
		if (FMessageProcessor)
		{
			Jid contactJid = FNotifies.value(ANotifyId);
			Jid streamJid = findContactStream(contactJid);
			IPresence *presence = FPresencePlugin!=NULL ? FPresencePlugin->getPresence(streamJid) : NULL;
			QList<IPresenceItem> presences = presence!=NULL ? presence->presenceItems(contactJid) : QList<IPresenceItem>();
			FMessageProcessor->createWindow(streamJid, !presences.isEmpty() ? presences.first().itemJid : contactJid, Message::Chat, IMessageHandler::SM_SHOW);
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

void BirthdayReminder::onNotificationTest(const QString &ANotificatorId, uchar AKinds)
{
	if (ANotificatorId == NID_BIRTHDAY_REMIND)
	{
		INotification notify;
		notify.kinds = AKinds;
		notify.notificatior = ANotificatorId;
		if (AKinds & INotification::PopupWindow)
		{
			Jid contactJid = "vasilisa@rambler/friends";
			notify.data.insert(NDR_POPUP_CAPTION,tr("Birthday of"));
			notify.data.insert(NDR_POPUP_IMAGE,FNotifications->contactAvatar(contactJid.full()));
			notify.data.insert(NDR_POPUP_TITLE,tr("Vasilisa Premudraya"));
			notify.data.insert(NDR_POPUP_TEXT,tr("Birthday <b>today</b>!"));
			notify.data.insert(NDR_POPUP_STYLEKEY,STS_NOTIFICATION_NOTIFYWIDGET);

			Action *action = new Action(NULL);
			action->setText(tr("Congratulate with postcard"));
			action->setData(Action::DR_UserDefined + 1, "birthday");
			notify.actions.append(action);
		}
		if (AKinds & INotification::PlaySoundNotification)
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
		if ((FNotifications->notificatorKinds(NID_BIRTHDAY_REMIND) & (INotification::PopupWindow|INotification::PlaySoundNotification)) == 0)
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

	FNotifications->setNotificatorKinds(NID_BIRTHDAY_REMIND,INotification::PopupWindow|INotification::PlaySoundNotification);

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
			tip += QString("<a href='%1'>%2</a>").arg("http://id.rambler.ru").arg(tr("Congratulate with postcard"));
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
