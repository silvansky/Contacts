#include "gateways.h"

#include <QRegExp>
#include <QTextDocument>
#include <definitions/customborder.h>
#include <utils/customborderstorage.h>

#define ADR_STREAM_JID            Action::DR_StreamJid
#define ADR_SERVICE_JID           Action::DR_Parametr1
#define ADR_NEW_SERVICE_JID       Action::DR_Parametr2
#define ADR_LOG_IN                Action::DR_Parametr3

#define PST_GATEWAYS_SERVICES     "services"
#define PSN_GATEWAYS_KEEP         "virtus:gateways:keep"

#define GATEWAY_TIMEOUT           30000
#define KEEP_INTERVAL             120000

#define MAIL_NODE_PATTERN         "[a-zA-Z0-9_\\-\\.]+"

Gateways::Gateways()
{
	FPluginManager = NULL;
	FDiscovery = NULL;
	FXmppStreams = NULL;
	FStanzaProcessor = NULL;
	FRosterPlugin = NULL;
	FPresencePlugin = NULL;
	FRosterChanger = NULL;
	FRostersViewPlugin = NULL;
	FVCardPlugin = NULL;
	FPrivateStorage = NULL;
	FStatusIcons = NULL;
	FRegistration = NULL;
	FOptionsManager = NULL;
	FDataForms = NULL;
	FMainWindowPlugin = NULL;
	FNotifications = NULL;

	FInternalNoticeId = -1;

	FKeepTimer.setSingleShot(false);
	connect(&FKeepTimer,SIGNAL(timeout()),SLOT(onKeepTimerTimeout()));
}

Gateways::~Gateways()
{

}

void Gateways::pluginInfo(IPluginInfo *APluginInfo)
{
	APluginInfo->name = tr("Gateway Interaction");
	APluginInfo->description = tr("Allows to simplify the interaction with transports to other IM systems");
	APluginInfo->version = "1.0";
	APluginInfo->author = "Potapov S.A. aka Lion";
	APluginInfo->homePage = "http://contacts.rambler.ru";
	APluginInfo->dependences.append(STANZAPROCESSOR_UUID);
}

bool Gateways::initConnections(IPluginManager *APluginManager, int &AInitOrder)
{
	Q_UNUSED(AInitOrder);
	FPluginManager = APluginManager;

	IPlugin *plugin = APluginManager->pluginInterface("IServiceDiscovery").value(0,NULL);
	if (plugin)
	{
		FDiscovery = qobject_cast<IServiceDiscovery *>(plugin->instance());
		if (FDiscovery)
		{
			connect(FDiscovery->instance(),SIGNAL(discoInfoReceived(const IDiscoInfo &)),
				SLOT(onDiscoInfoChanged(const IDiscoInfo &)));
			connect(FDiscovery->instance(),SIGNAL(discoInfoRemoved(const IDiscoInfo &)),
				SLOT(onDiscoInfoChanged(const IDiscoInfo &)));
			connect(FDiscovery->instance(),SIGNAL(discoItemsReceived(const IDiscoItems &)),
				SLOT(onDiscoItemsReceived(const IDiscoItems &)));
		}
	}

	plugin = APluginManager->pluginInterface("IXmppStreams").value(0,NULL);
	if (plugin)
	{
		FXmppStreams = qobject_cast<IXmppStreams *>(plugin->instance());
		if (FXmppStreams)
		{
			connect(FXmppStreams->instance(),SIGNAL(opened(IXmppStream *)),SLOT(onXmppStreamOpened(IXmppStream *)));
			connect(FXmppStreams->instance(),SIGNAL(closed(IXmppStream *)),SLOT(onXmppStreamClosed(IXmppStream *)));
		}
	}

	plugin = APluginManager->pluginInterface("IStanzaProcessor").value(0,NULL);
	if (plugin)
		FStanzaProcessor = qobject_cast<IStanzaProcessor *>(plugin->instance());

	plugin = APluginManager->pluginInterface("IRosterPlugin").value(0,NULL);
	if (plugin)
	{
		FRosterPlugin = qobject_cast<IRosterPlugin *>(plugin->instance());
		if (FRosterPlugin)
		{
			connect(FRosterPlugin->instance(),SIGNAL(rosterOpened(IRoster *)),SLOT(onRosterOpened(IRoster *)));
			connect(FRosterPlugin->instance(),SIGNAL(rosterItemReceived(IRoster *, const IRosterItem &, const IRosterItem &)),
				SLOT(onRosterItemReceived(IRoster *, const IRosterItem &, const IRosterItem &)));
		}
	}

	plugin = APluginManager->pluginInterface("IPresencePlugin").value(0,NULL);
	if (plugin)
	{
		FPresencePlugin = qobject_cast<IPresencePlugin *>(plugin->instance());
		if (FPresencePlugin)
		{
			connect(FPresencePlugin->instance(),SIGNAL(presenceItemReceived(IPresence *, const IPresenceItem &, const IPresenceItem &)),
				SLOT(onPresenceItemReceived(IPresence *, const IPresenceItem &, const IPresenceItem &)));
		}
	}

	plugin = APluginManager->pluginInterface("IPrivateStorage").value(0,NULL);
	if (plugin)
	{
		FPrivateStorage = qobject_cast<IPrivateStorage *>(plugin->instance());
		if (FPrivateStorage)
		{
			connect(FPrivateStorage->instance(),SIGNAL(storageOpened(const Jid &)),SLOT(onPrivateStorateOpened(const Jid &)));
			connect(FPrivateStorage->instance(),SIGNAL(dataLoaded(const QString &, const Jid &, const QDomElement &)),
				SLOT(onPrivateStorageLoaded(const QString &, const Jid &, const QDomElement &)));
			connect(FPrivateStorage->instance(),SIGNAL(storageAboutToClose(const Jid &)),SLOT(onPrivateStorateAboutToClose(const Jid &)));
			connect(FPrivateStorage->instance(),SIGNAL(storageClosed(const Jid &)),SLOT(onPrivateStorateClosed(const Jid &)));
		}
	}

	plugin = APluginManager->pluginInterface("IRegistration").value(0,NULL);
	if (plugin)
	{
		FRegistration = qobject_cast<IRegistration *>(plugin->instance());
		if (FRegistration)
		{
			connect(FRegistration->instance(),SIGNAL(registerFields(const QString &, const IRegisterFields &)),
				SLOT(onRegisterFields(const QString &, const IRegisterFields &)));
			connect(FRegistration->instance(),SIGNAL(registerSuccess(const QString &)),
				SLOT(onRegisterSuccess(const QString &)));
			connect(FRegistration->instance(),SIGNAL(registerError(const QString &, const QString &, const QString &)),
				SLOT(onRegisterError(const QString &, const QString &, const QString &)));
		}
	}

	plugin = APluginManager->pluginInterface("IRosterChanger").value(0,NULL);
	if (plugin)
		FRosterChanger = qobject_cast<IRosterChanger *>(plugin->instance());

	plugin = APluginManager->pluginInterface("IRostersViewPlugin").value(0,NULL);
	if (plugin)
	{
		FRostersViewPlugin = qobject_cast<IRostersViewPlugin *>(plugin->instance());
	}

	plugin = APluginManager->pluginInterface("IVCardPlugin").value(0,NULL);
	if (plugin)
	{
		FVCardPlugin = qobject_cast<IVCardPlugin *>(plugin->instance());
		if (FVCardPlugin)
		{
			connect(FVCardPlugin->instance(),SIGNAL(vcardReceived(const Jid &)),SLOT(onVCardReceived(const Jid &)));
			connect(FVCardPlugin->instance(),SIGNAL(vcardError(const Jid &, const QString &)),SLOT(onVCardError(const Jid &, const QString &)));
		}
	}

	plugin = APluginManager->pluginInterface("IStatusIcons").value(0,NULL);
	if (plugin)
		FStatusIcons = qobject_cast<IStatusIcons *>(plugin->instance());

	plugin = APluginManager->pluginInterface("IOptionsManager").value(0,NULL);
	if (plugin)
		FOptionsManager = qobject_cast<IOptionsManager *>(plugin->instance());

	plugin = APluginManager->pluginInterface("IDataForms").value(0,NULL);
	if (plugin)
		FDataForms = qobject_cast<IDataForms *>(plugin->instance());

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

	plugin = APluginManager->pluginInterface("INotifications").value(0,NULL);
	if (plugin)
	{
		FNotifications = qobject_cast<INotifications *>(plugin->instance());
		if (FNotifications)
		{
			connect(FNotifications->instance(),SIGNAL(notificationActivated(int)), SLOT(onNotificationActivated(int)));
			connect(FNotifications->instance(),SIGNAL(notificationRemoved(int)), SLOT(onNotificationRemoved(int)));
		}
	}

	return FStanzaProcessor!=NULL;
}

bool Gateways::initObjects()
{
	static const QString JabberContactPattern = "^"JID_NODE_PATTERN"@"JID_DOMAIN_PATTERN"$";

	// !!    !!
	IGateServiceDescriptor sms;
	sms.id = GSID_SMS;
	sms.needGate = true;
	sms.needLogin = false;
	sms.autoLogin = true;
	sms.type = "sms";
	sms.name = tr("SMS");
	sms.iconKey = MNI_GATEWAYS_SERVICE_SMS;
	sms.loginLabel = tr("Phone");
	sms.homeContactPattern = "^\\+7\\d{10,10}$";
	sms.availContactPattern = sms.homeContactPattern;
	FGateDescriptors.append(sms);

	IGateServiceDescriptor icq;
	icq.id = GSID_ICQ;
	icq.needGate = true;
	icq.type = "icq";
	icq.name = tr("ICQ");
	icq.iconKey = MNI_GATEWAYS_SERVICE_ICQ;
	icq.loginLabel = tr("Login");
	icq.loginField = "username";
	icq.passwordField = "password";
	icq.homeContactPattern = "^\\d{5,10}$";
	icq.availContactPattern = icq.homeContactPattern;
	FGateDescriptors.append(icq);

	IGateServiceDescriptor magent;
	magent.id = GSID_MAGENT;
	magent.needGate = true;
	magent.type = "mrim";
	magent.name = tr("Agent@Mail.ru");
	magent.iconKey = MNI_GATEWAYS_SERVICE_MAGENT;
	magent.loginLabel = tr("E-mail");
	magent.domains << "mail.ru" << "inbox.ru" << "list.ru" << "bk.ru";
	magent.loginField = "username";
	magent.passwordField = "password";
	magent.domainSeparator = "@";
	magent.homeContactPattern = "^"MAIL_NODE_PATTERN"@(mail|inbox|bk|list)\\.ru$";
	magent.availContactPattern = magent.homeContactPattern;
	magent.linkedDescriptors.append(GSID_MAIL);
	FGateDescriptors.append(magent);

	IGateServiceDescriptor twitter;
	twitter.id = GSID_TWITTER;
	twitter.needGate = true;
	twitter.type = "twitter";
	twitter.name = tr("Twitter");
	twitter.iconKey = MNI_GATEWAYS_SERVICE_TWITTER;
	twitter.loginLabel = tr("Login");
	twitter.loginField = "username";
	twitter.passwordField = "password";
	twitter.homeContactPattern = "^@[a-zA-Z0-9_]+";
	twitter.availContactPattern = twitter.homeContactPattern;
	FGateDescriptors.append(twitter);

	IGateServiceDescriptor gtalk;
	gtalk.id = GSID_GTALK;
	gtalk.type = "xmpp";
	gtalk.prefix = "gmail";
	gtalk.name = tr("GTalk");
	gtalk.iconKey = MNI_GATEWAYS_SERVICE_GTALK;
	gtalk.loginLabel = tr("E-mail");
	gtalk.domains << "gmail.com" << "googlemail.com";
	gtalk.loginField = "username";
	gtalk.domainField = "server";
	gtalk.passwordField = "password";
	gtalk.domainSeparator = "@";
	gtalk.homeContactPattern = "^"MAIL_NODE_PATTERN"@(gmail|googlemail)\\.com$";
	gtalk.availContactPattern = JabberContactPattern;
	gtalk.linkedDescriptors.append(GSID_MAIL);
	FGateDescriptors.append(gtalk);

	IGateServiceDescriptor yonline;
	yonline.id = GSID_YONLINE;
	yonline.type = "xmpp";
	yonline.prefix = "yandex";
	yonline.name = tr("Ya.Online");
	yonline.iconKey = MNI_GATEWAYS_SERVICE_YONLINE;
	yonline.loginLabel = tr("E-mail");
	yonline.domains << "ya.ru" << "yandex.ru" << "yandex.net" << "yandex.com" << "yandex.by" << "yandex.kz" << "yandex.ua" << "yandex-co.ru" << "narod.ru";
	yonline.loginField = "username";
	yonline.domainField = "server";
	yonline.passwordField = "password";
	yonline.domainSeparator = "@";
	yonline.homeContactPattern = "^"MAIL_NODE_PATTERN"@(ya\\.ru|yandex\\.ru|yandex\\.net|yandex\\.com|yandex\\-co\\.ru|narod\\.ru|yandex\\.by|yandex\\.kz|yandex\\.ua)$";
	yonline.availContactPattern = JabberContactPattern;
	yonline.linkedDescriptors.append(GSID_MAIL);
	FGateDescriptors.append(yonline);

	IGateServiceDescriptor qip;
	qip.id = GSID_QIP;
	qip.type = "xmpp";
	qip.prefix = "qip";
	qip.name = tr("QIP");
	qip.iconKey = MNI_GATEWAYS_SERVICE_QIP;
	qip.loginLabel = tr("Login");
	qip.domains << "qip.ru" << "pochta.ru" << "fromru.com" << "front.ru" << "hotbox.ru"	<< "hotmail.ru"	<< "krovatka.su" << "land.ru"	<< "mail15.com"	<< "mail333.com"
		<< "newmail.ru"	<< "nightmail.ru"	<< "nm.ru"	<< "pisem.net"	<< "pochtamt.ru"	<< "pop3.ru"	<< "rbcmail.ru"	<< "smtp.ru"	<< "5ballov.ru"	<< "aeterna.ru"
		<< "ziza.ru"	<< "memori.ru"	<< "photofile.ru"	<< "fotoplenka.ru";
	qip.loginField = "username";
	qip.domainField = "server";
	qip.passwordField = "password";
	qip.domainSeparator = "@";
	qip.homeContactPattern = "^"MAIL_NODE_PATTERN"@((qip|pochta|front|hotbox|hotmail|land|newmail|nightmail|nm|pochtamt|pop3|rbcmail|smtp|5ballov|aeterna|ziza|memori|photofile|fotoplenka)\\.ru|(fromru|mail15|mail333)\\.com|(krovatka)\\.su|(pisem)\\.net)$";
	qip.availContactPattern = JabberContactPattern;
	qip.linkedDescriptors.append(GSID_MAIL);
	FGateDescriptors.append(qip);

	IGateServiceDescriptor vkontakte;
	vkontakte.id = GSID_VKONTAKTE;
	vkontakte.needGate = true;
	vkontakte.readOnly = true;
	vkontakte.type = "xmpp";
	vkontakte.prefix = "vk";
	vkontakte.name = tr("VKontakte");
	vkontakte.iconKey = MNI_GATEWAYS_SERVICE_VKONTAKTE;
	vkontakte.loginLabel = tr("E-mail or Login");
	vkontakte.loginField = "username";
	vkontakte.passwordField = "password";
	vkontakte.homeContactPattern = "^"MAIL_NODE_PATTERN"@vk\\.com$";
	vkontakte.availContactPattern = JabberContactPattern;
	vkontakte.blockedDescriptors.append(GSID_MAIL);
	FGateDescriptors.append(vkontakte);

	IGateServiceDescriptor odnoklasniki;
	odnoklasniki.id = GSID_ODNOKLASNIKI;
	odnoklasniki.needGate = true;
	odnoklasniki.readOnly = true;
	odnoklasniki.type = "xmpp";
	odnoklasniki.prefix = "ok";
	odnoklasniki.name = tr("Odnoklassniki");
	odnoklasniki.iconKey = MNI_GATEWAYS_SERVICE_ODNOKLASNIKI;
	odnoklasniki.loginLabel = tr("Login");
	odnoklasniki.domains << "odnoklassniki.ru";
	odnoklasniki.loginField = "username";
	odnoklasniki.passwordField = "password";
	odnoklasniki.domainSeparator = "@";
	odnoklasniki.homeContactPattern = "^"MAIL_NODE_PATTERN"@odnoklassniki\\.ru$";
	odnoklasniki.availContactPattern = JabberContactPattern;
	odnoklasniki.blockedDescriptors.append(GSID_MAIL);
	FGateDescriptors.append(odnoklasniki);

	IGateServiceDescriptor facebook;
	facebook.id = GSID_FACEBOOK;
	facebook.needGate = true;
	facebook.readOnly = true;
	facebook.type = "xmpp";
	facebook.prefix = "fb";
	facebook.name = tr("Facebook");
	facebook.iconKey = MNI_GATEWAYS_SERVICE_FACEBOOK;
	facebook.loginLabel = tr("Login");
	facebook.domains << "chat.facebook.com";
	facebook.loginField = "username";
	facebook.domainField = "server";
	facebook.passwordField = "password";
	facebook.domainSeparator = "@";
	facebook.homeContactPattern = "^"MAIL_NODE_PATTERN"@chat\\.facebook\\.com$";
	facebook.availContactPattern = JabberContactPattern;
	facebook.blockedDescriptors.append(GSID_MAIL);
	FGateDescriptors.append(facebook);

	IGateServiceDescriptor livejournal;
	livejournal.id = GSID_LIVEJOURNAL;
	livejournal.type = "xmpp";
	livejournal.prefix = "livejournal";
	livejournal.name = tr("LiveJournal");
	livejournal.iconKey = MNI_GATEWAYS_SERVICE_LIVEJOURNAL;
	livejournal.loginLabel = tr("Login");
	livejournal.domains << "livejournal.com";
	livejournal.loginField = "username";
	livejournal.domainField = "server";
	livejournal.passwordField = "password";
	livejournal.domainSeparator = "@";
	livejournal.homeContactPattern = "^"MAIL_NODE_PATTERN"@livejournal\\.com$";
	livejournal.availContactPattern = JabberContactPattern;
	livejournal.blockedDescriptors.append(GSID_MAIL);
	FGateDescriptors.append(livejournal);

	IGateServiceDescriptor rambler;
	rambler.id = GSID_RAMBLER;
	rambler.type = "xmpp";
	rambler.prefix = "rambler";
	rambler.name = tr("Rambler");
	rambler.iconKey = MNI_GATEWAYS_SERVICE_RAMBLER;
	rambler.loginLabel = tr("Login");
	rambler.domains << "rambler.ru" << "lenta.ru" << "myrambler.ru" << "autorambler.ru" << "ro.ru" << "r0.ru";
	rambler.loginField = "username";
	rambler.domainField = "server";
	rambler.passwordField = "password";
	rambler.domainSeparator = "@";
	rambler.homeContactPattern = "^"MAIL_NODE_PATTERN"@(rambler|lenta|myrambler|autorambler|ro|r0)\\.ru$";
	rambler.availContactPattern = JabberContactPattern;
	rambler.linkedDescriptors.append(GSID_MAIL);
	FGateDescriptors.append(rambler);

	IGateServiceDescriptor jabber;
	jabber.id = GSID_JABBER;
	jabber.type = "xmpp";
	jabber.prefix = "jabber";
	jabber.name = tr("Jabber");
	jabber.iconKey = MNI_GATEWAYS_SERVICE_JABBER;
	jabber.loginLabel = tr("Login");
	jabber.domains << "jabber.ru";
	jabber.loginField = "username";
	jabber.domainField = "server";
	jabber.passwordField = "password";
	jabber.domainSeparator = "@";
	jabber.homeContactPattern = JabberContactPattern;
	jabber.availContactPattern = JabberContactPattern;
	FGateDescriptors.append(jabber);

	//      ..   
	IGateServiceDescriptor mail;
	mail.id = GSID_MAIL;
	mail.needGate = true;
	mail.needLogin = false;
	mail.autoLogin = true;
	mail.type = "mail";
	mail.name = tr("Mail");
	mail.iconKey = MNI_GATEWAYS_SERVICE_MAIL;
	mail.loginLabel = tr("Mail");
	mail.loginField = "username";
	mail.passwordField = "password";
	mail.homeContactPattern = "^"MAIL_NODE_PATTERN"@"JID_DOMAIN_PATTERN"$";
	mail.availContactPattern = mail.homeContactPattern;
	FGateDescriptors.append(mail);

	if (FDiscovery)
	{
		registerDiscoFeatures();
	}

	if (FRostersViewPlugin)
	{
		LegacyAccountFilter *filter = new LegacyAccountFilter(this,this);
		FRostersViewPlugin->rostersView()->insertProxyModel(filter,RPO_GATEWAYS_ACCOUNT_FILTER);
	}

	if (FNotifications)
	{
		INotificationType notifyType;
		notifyType.order = OWO_NOTIFICATIONS_GATEWAYS_CONFLICT;
		notifyType.kindMask = INotification::PopupWindow|INotification::SoundPlay;
		notifyType.kindDefs = notifyType.kindMask;
		FNotifications->registerNotificationType(NNT_GATEWAYS_CONFLICT,notifyType);
	}

	return true;
}

bool Gateways::initSettings()
{
	if (FOptionsManager)
	{
		FOptionsManager->insertOptionsHolder(this);
	}
	return true;
}

QMultiMap<int, IOptionsWidget *> Gateways::optionsWidgets(const QString &ANodeId, QWidget *AParent)
{
	QMultiMap<int, IOptionsWidget *> widgets;
	if (ANodeId == OPN_GATEWAYS)
	{
		widgets.insertMulti(OWO_GATEWAYS_ACCOUNTS_MANAGE-1, FOptionsManager->optionsHeaderWidget(QString::null,tr("Accounts"),AParent));
		widgets.insertMulti(OWO_GATEWAYS_ACCOUNTS_MANAGE, new ManageLegacyAccountsOptions(this,FOptionsStreamJid,AParent));
		widgets.insertMulti(OWO_GATEWAYS_ACCOUNTS_APPEND-1, FOptionsManager->optionsHeaderWidget(QString::null,tr("Add account"),AParent));
		widgets.insertMulti(OWO_GATEWAYS_ACCOUNTS_APPEND, new AddLegacyAccountOptions(this,FDiscovery,FOptionsStreamJid,AParent));
	}
	return widgets;
}

void Gateways::stanzaRequestResult(const Jid &AStreamJid, const Stanza &AStanza)
{
	Q_UNUSED(AStreamJid);
	if (FPromptRequests.contains(AStanza.id()))
	{
		if (AStanza.type() == "result")
		{
			LogDetail(QString("[Gateways] Gateway prompt received from '%1', id='%2'").arg(AStanza.from(),AStanza.id()));
			QString desc = AStanza.firstElement("query",NS_JABBER_GATEWAY).firstChildElement("desc").text();
			QString prompt = AStanza.firstElement("query",NS_JABBER_GATEWAY).firstChildElement("prompt").text();
			emit promptReceived(AStanza.id(),desc,prompt);
		}
		else
		{
			ErrorHandler err(AStanza.element());
			LogError(QString("[Gateways] Failed to request gateway prompt from '%1', id='%2': %3").arg(AStanza.from(),AStanza.id(),err.message()));
			emit errorReceived(AStanza.id(),err.message());
		}
		FPromptRequests.removeAt(FPromptRequests.indexOf(AStanza.id()));
	}
	else if (FUserJidRequests.contains(AStanza.id()))
	{
		if (AStanza.type() == "result")
		{
			LogDetail(QString("[Gateways] User JID received from '%1', id='%2'").arg(AStanza.from(),AStanza.id()));
			Jid userJid = AStanza.firstElement("query",NS_JABBER_GATEWAY).firstChildElement("jid").text();
			emit userJidReceived(AStanza.id(),userJid);
		}
		else
		{
			ErrorHandler err(AStanza.element());
			LogError(QString("[Gateways] Failed to request user JID from '%1' id='%2': %3").arg(AStanza.from(),AStanza.id(),err.message()));
			emit errorReceived(AStanza.id(),err.message());
		}
		FUserJidRequests.removeAt(FUserJidRequests.indexOf(AStanza.id()));
	}
}

void Gateways::stanzaRequestTimeout(const Jid &AStreamJid, const QString &AStanzaId)
{
	Q_UNUSED(AStreamJid);
	if (FPromptRequests.contains(AStanzaId))
	{
		ErrorHandler err(ErrorHandler::REQUEST_TIMEOUT);
		LogError(QString("[Gateways] Failed to request gateway prompt id='%1': %2").arg(AStanzaId,err.message()));
		emit errorReceived(AStanzaId,err.message());
		FPromptRequests.removeAt(FPromptRequests.indexOf(AStanzaId));
	}
	else if (FUserJidRequests.contains(AStanzaId))
	{
		ErrorHandler err(ErrorHandler::REQUEST_TIMEOUT);
		LogError(QString("[Gateways] Failed to request user JID id='%1': %2").arg(AStanzaId,err.message()));
		emit errorReceived(AStanzaId,err.message());
		FUserJidRequests.removeAt(FUserJidRequests.indexOf(AStanzaId));
	}
}

void Gateways::resolveNickName(const Jid &AStreamJid, const Jid &AContactJid)
{
	IRoster *roster = FRosterPlugin!=NULL ? FRosterPlugin->findRoster(AStreamJid) : NULL;
	IRosterItem ritem = roster!=NULL ? roster->rosterItem(AContactJid) : IRosterItem();
	if (ritem.isValid)
	{
		if (FVCardPlugin->hasVCard(ritem.itemJid))
		{
			IVCard *vcard = FVCardPlugin->vcard(ritem.itemJid);
			QString nick = vcard->value(VVN_NICKNAME);
			if (!nick.isEmpty())
				roster->renameItem(ritem.itemJid,nick);
			vcard->unlock();
		}
		else
		{
			if (!FResolveNicks.contains(ritem.itemJid))
				FVCardPlugin->requestVCard(AStreamJid,ritem.itemJid);
			FResolveNicks.insertMulti(ritem.itemJid,AStreamJid);
		}
	}
}

void Gateways::sendLogPresence(const Jid &AStreamJid, const Jid &AServiceJid, bool ALogIn)
{
	IPresence *presence = FPresencePlugin!=NULL ? FPresencePlugin->findPresence(AStreamJid) : NULL;
	if (presence && presence->isOpen())
	{
		if (ALogIn)
		{
			LogDetail(QString("[Gateways] Sending Log-In presence to service '%1'").arg(AServiceJid.full()));
			presence->sendPresence(AServiceJid,presence->show(),presence->status(),presence->priority());
		}
		else
		{
			LogDetail(QString("[Gateways] Sending Log-Out presence to service '%1'").arg(AServiceJid.full()));
			presence->sendPresence(AServiceJid,IPresence::Offline,tr("Log Out"),0);
		}
	}
}

QList<Jid> Gateways::keepConnections(const Jid &AStreamJid) const
{
	return FKeepConnections.value(AStreamJid).toList();
}

void Gateways::setKeepConnection(const Jid &AStreamJid, const Jid &AServiceJid, bool AEnabled)
{
	IXmppStream *stream = FXmppStreams!=NULL ? FXmppStreams->xmppStream(AStreamJid) : NULL;
	if (stream && stream->isOpen())
	{
		if (AEnabled)
			FKeepConnections[AStreamJid] += AServiceJid;
		else
			FKeepConnections[AStreamJid] -= AServiceJid;
	}
}

QList<IGateServiceDescriptor> Gateways::gateDescriptors() const
{
	return FGateDescriptors;
}

IGateServiceDescriptor Gateways::gateDescriptorById(const QString &ADescriptorId) const
{
	for (QList<IGateServiceDescriptor>::const_iterator it = FGateDescriptors.constBegin() ; it!=FGateDescriptors.constEnd(); it++)
		if (it->id == ADescriptorId)
			return *it;
	return IGateServiceDescriptor();
}

QList<IGateServiceDescriptor> Gateways::gateHomeDescriptorsByContact(const QString &AContact) const
{
	QList<IGateServiceDescriptor> descriptors;
	if (!AContact.isEmpty())
	{
		QRegExp homeRegExp;
		homeRegExp.setCaseSensitivity(Qt::CaseInsensitive);
		for (QList<IGateServiceDescriptor>::const_iterator it = FGateDescriptors.constBegin() ; it!=FGateDescriptors.constEnd(); it++)
		{
			if (!it->homeContactPattern.isEmpty())
			{
				QString contact = normalizedContactLogin(*it,AContact);
				if (!contact.isEmpty())
				{
					homeRegExp.setPattern(it->homeContactPattern);
					if (homeRegExp.exactMatch(contact))
					{
						descriptors.append(*it);
					}
					else if (!it->domains.isEmpty() && it->domainSeparator=="@" && it->domains.contains(Jid(contact).pDomain()))
					{
						descriptors.clear();
						break;
					}
				}
			}
		}
	}
	return descriptors;
}

QList<IGateServiceDescriptor> Gateways::gateAvailDescriptorsByContact(const QString &AContact) const
{
	QList<IGateServiceDescriptor> descriptors;
	if (!AContact.isEmpty())
	{
		QRegExp availRegExp;
		availRegExp.setCaseSensitivity(Qt::CaseInsensitive);
		for (QList<IGateServiceDescriptor>::const_iterator it = FGateDescriptors.constBegin() ; it!=FGateDescriptors.constEnd(); it++)
		{
			if (!it->availContactPattern.isEmpty())
			{
				QString contact = normalizedContactLogin(*it,AContact);
				if (!contact.isEmpty())
				{
					availRegExp.setPattern(it->availContactPattern);
					if (availRegExp.exactMatch(contact))
						descriptors.append(*it);
				}
			}
		}
	}
	return descriptors;
}

int Gateways::gateDescriptorStatus(const Jid &AStreamJid, const IGateServiceDescriptor &ADescriptor) const
{
	if (!ADescriptor.id.isEmpty())
	{
		if (ADescriptor.needGate)
		{
			if (!gateDescriptorServices(AStreamJid,ADescriptor).isEmpty())
			{
				if (ADescriptor.needLogin)
				{
					QList<Jid> streamGates = gateDescriptorServices(AStreamJid,ADescriptor,true);
					if (!streamGates.isEmpty())
					{
						foreach(Jid gateJid, streamGates)
						{
							if (isServiceEnabled(AStreamJid,gateJid))
								return GDS_ENABLED;
						}
						return GDS_DISABLED;
					}
					if (!gateDescriptorRegistrator(AStreamJid,ADescriptor).isEmpty())
						return GDS_UNREGISTERED;
					return GDS_UNAVAILABLE;
				}
				return GDS_ENABLED;
			}
			return GDS_UNAVAILABLE;
		}
		return GDS_ENABLED;
	}
	return GDS_UNAVAILABLE;
}

Jid Gateways::gateDescriptorRegistrator(const Jid &AStreamJid, const IGateServiceDescriptor &ADescriptor, bool AFree) const
{
	QList<Jid> availGates = gateDescriptorServices(AStreamJid,ADescriptor);
	foreach(Jid serviceJid, availGates)
	{
		IDiscoInfo dinfo = FDiscovery->discoInfo(AStreamJid,serviceJid);
		if (dinfo.features.contains(NS_RAMBLER_GATEWAY_REGISTER))
		{
			if (AFree)
			{
				QSet<Jid> freeGates = availGates.toSet() - streamServices(AStreamJid).toSet();
				freeGates -= serviceJid;
				if (freeGates.isEmpty())
					return Jid::null;
			}
			return serviceJid;
		}
	}
	return Jid::null;
}

QString Gateways::formattedContactLogin(const IGateServiceDescriptor &ADescriptor, const QString &AContact) const
{
	QString contact = normalizedContactLogin(ADescriptor,AContact,true);
	if (ADescriptor.id==GSID_SMS && contact.length()==12)
	{
		// +7 (XXX) XXX-XX-XX
		contact.insert(2," (");
		contact.insert(7,") ");
		contact.insert(12,"-");
		contact.insert(15,"-");
	}
	else if (ADescriptor.id == GSID_ICQ)
	{
		for(int pos=3; contact.length()-pos>=2; pos+=4)
			contact.insert(pos,"-");
	}
	else if (ADescriptor.type == "xmpp")
	{
		contact = Jid(contact).full();
	}
	return contact;
}

QString Gateways::normalizedContactLogin(const IGateServiceDescriptor &ADescriptor, const QString &AContact, bool AComplete) const
{
	QString contact = AContact.trimmed();
	if (!contact.isEmpty())
	{
		//    
		if (ADescriptor.id == GSID_SMS)
		{
			QString number;
			for (int i=0; i<contact.length(); i++)
			{
				QChar ch = contact.at(i);
				if (ch.isDigit() || ch.isLetter())
					number += ch;
				else if (i==0 && ch=='+')
					number += ch;
			}
			contact = number;

			if (!contact.isEmpty())
			{
				if ( contact.length()==11 && (contact.startsWith('8') || contact.startsWith('7')) )
				{
					contact.remove(0,1);
					contact.prepend("+7");
				}
				else if (contact.length()==10 && !contact.startsWith('+'))
				{
					contact.prepend("+7");
				}
			}
		}
		else if (ADescriptor.id == GSID_ICQ)
		{
			QString number;
			for (int i=0; i<contact.length(); i++)
			{
				QChar ch = contact.at(i);
				if (ch.isDigit() || ch.isLetter())
					number += ch;
			}
			contact = number;
		}
		else
		{
			//  ,   
			if (AComplete && !ADescriptor.domainSeparator.isEmpty() && !contact.contains(ADescriptor.domainSeparator) && !ADescriptor.domains.isEmpty())
			{
				contact += ADescriptor.domainSeparator + ADescriptor.domains.value(0);
			}

			//  Jid Escaping
			if (ADescriptor.type == "xmpp")
			{
				contact = Jid(contact).eFull();
			}
		}
	}
	return contact;
}

QString Gateways::checkNormalizedContactLogin(const IGateServiceDescriptor &ADescriptor, const QString &AContact) const
{
	QString errMessage;

	//    
	if (ADescriptor.id == GSID_SMS)
	{
		bool validChars = true;
		for (int i=0; validChars && i<AContact.length(); i++)
			validChars = AContact.at(i).isDigit() || ( i==0 && AContact.at(i)=='+');

		if (!validChars)
		{
			errMessage = tr("Entered phone number contains invalid characters.");
		}
		else if (!AContact.startsWith("+") || AContact.length()<12)
		{
			errMessage = tr("Enter the entire number, including area code or operator code.");
		}
		else if (AContact.length()>12)
		{
			errMessage = tr("Too many digits in the phone number.");
		}
	}

	//     
	QRegExp availRegExp(ADescriptor.availContactPattern);
	availRegExp.setCaseSensitivity(Qt::CaseInsensitive);
	if (errMessage.isEmpty() && !availRegExp.exactMatch(AContact))
	{
		errMessage = tr("Entered address is not suitable for selected service.");
	}

	return errMessage;
}

QList<Jid> Gateways::availRegistrators(const Jid &AStreamJid, bool AFree) const
{
	IDiscoIdentity identity;
	identity.category = "gateway";
	QList<Jid> usedGates = streamServices(AStreamJid,identity);
	QList<Jid> availGates = availServices(AStreamJid,identity);

	QList<Jid> registrators;
	foreach(Jid registratorJid, availGates)
	{
		if (FDiscovery && FDiscovery->discoInfo(AStreamJid,registratorJid).features.contains(NS_RAMBLER_GATEWAY_REGISTER))
		{
			if (AFree)
			{
				IGateServiceDescriptor rdescriptor = serviceDescriptor(AStreamJid,registratorJid);
				foreach(Jid serviceJid, availGates)
				{
					if (serviceJid!=registratorJid && !usedGates.contains(serviceJid) && serviceDescriptor(AStreamJid,serviceJid).id==rdescriptor.id)
					{
						registrators.append(registratorJid);
						break;
					}
				}
			}
			else
			{
				registrators.append(registratorJid);
			}
		}
	}
	return registrators;
}

QList<Jid> Gateways::availServices(const Jid &AStreamJid, const IDiscoIdentity &AIdentity) const
{
	QList<Jid> services;
	foreach (IDiscoItem ditem, FStreamDiscoItems.value(AStreamJid).items)
	{
		if (FDiscovery && (!AIdentity.category.isEmpty() || !AIdentity.type.isEmpty()))
		{
			IDiscoInfo dinfo = FDiscovery->discoInfo(AStreamJid, ditem.itemJid);
			foreach(IDiscoIdentity identity, dinfo.identity)
			{
				if ((AIdentity.category.isEmpty() || AIdentity.category == identity.category) && (AIdentity.type.isEmpty() || AIdentity.type == identity.type))
				{
					services.append(ditem.itemJid);
					break;
				}
			}
		}
		else
		{
			services.append(ditem.itemJid);
		}
	}
	return services;
}

QList<Jid> Gateways::streamServices(const Jid &AStreamJid, const IDiscoIdentity &AIdentity) const
{
	QList<Jid> services;
	IRoster *roster = FRosterPlugin!=NULL ? FRosterPlugin->findRoster(AStreamJid) : NULL;
	QList<IRosterItem> ritems = roster!=NULL ? roster->rosterItems() : QList<IRosterItem>();
	foreach(IRosterItem ritem, ritems)
	{
		if (ritem.itemJid.node().isEmpty())
		{
			if (FDiscovery && (!AIdentity.category.isEmpty() || !AIdentity.type.isEmpty()))
			{
				IDiscoInfo dinfo = FDiscovery->discoInfo(AStreamJid, ritem.itemJid);
				foreach(IDiscoIdentity identity, dinfo.identity)
				{
					if ((AIdentity.category.isEmpty() || AIdentity.category == identity.category) && (AIdentity.type.isEmpty() || AIdentity.type == identity.type))
					{
						services.append(ritem.itemJid);
						break;
					}
				}
			}
			else
			{
				services.append(ritem.itemJid);
			}
		}
	}
	return services;
}

QList<Jid> Gateways::gateDescriptorServices(const Jid &AStreamJid, const IGateServiceDescriptor &ADescriptor, bool AStreamOnly) const
{
	IDiscoIdentity identity;
	identity.category = "gateway";
	identity.type = ADescriptor.type;
	QList<Jid> gates = AStreamOnly ? streamServices(AStreamJid,identity) : availServices(AStreamJid,identity);
	if (ADescriptor.needGate && !ADescriptor.prefix.isEmpty())
	{
		QRegExp regexp(QString(GATE_PREFIX_PATTERN).arg(ADescriptor.prefix));
		for(QList<Jid>::iterator it = gates.begin(); it!=gates.end(); )
		{
			if (regexp.exactMatch(it->pDomain()))
				it = gates.erase(it);
			else
				it++;
		}
	}
	return gates;
}

QList<Jid> Gateways::serviceContacts(const Jid &AStreamJid, const Jid &AServiceJid) const
{
	QList<Jid> contacts;
	IRoster *roster = FRosterPlugin!=NULL ? FRosterPlugin->findRoster(AStreamJid) : NULL;
	QList<IRosterItem> ritems = roster!=NULL ? roster->rosterItems() : QList<IRosterItem>();
	foreach(IRosterItem ritem, ritems)
		if (!ritem.itemJid.node().isEmpty() && ritem.itemJid.pDomain()==AServiceJid.pDomain())
			contacts.append(ritem.itemJid);
	return contacts;
}

IPresenceItem Gateways::servicePresence(const Jid &AStreamJid, const Jid &AServiceJid) const
{
	IPresence *presence = FPresencePlugin!=NULL ? FPresencePlugin->findPresence(AStreamJid) : NULL;
	return presence!=NULL ? presence->presenceItem(AServiceJid) : IPresenceItem();
}

IGateServiceDescriptor Gateways::serviceDescriptor(const Jid &AStreamJid, const Jid &AServiceJid) const
{
	return FDiscovery!=NULL ? findGateDescriptor(FDiscovery->discoInfo(AStreamJid, AServiceJid)) : IGateServiceDescriptor();
}

IGateServiceLogin Gateways::serviceLogin(const Jid &AStreamJid, const Jid &AServiceJid, const IRegisterFields &AFields) const
{
	IGateServiceLogin login;
	IGateServiceDescriptor descriptor = FDiscovery!=NULL ? findGateDescriptor(FDiscovery->discoInfo(AStreamJid, AServiceJid)) : IGateServiceDescriptor();
	if (!descriptor.id.isEmpty())
	{
		login.fields = AFields;
		login.domainSeparator = descriptor.domainSeparator;
		if ((AFields.fieldMask & (IRegisterFields::Username|IRegisterFields::Email)) > 0)
		{
			login.isValid = true;
			login.login = AFields.fieldMask & IRegisterFields::Username ? AFields.username : AFields.email;
			login.password = AFields.password;
		}
		else if (FDataForms && FDataForms->isFormValid(AFields.form) && FDataForms->fieldIndex(descriptor.loginField,AFields.form.fields)>=0)
		{
			login.isValid = true;
			login.fields.fieldMask = 0;
			login.login = FDataForms->fieldValue(descriptor.loginField, AFields.form.fields).toString();
			login.domain = FDataForms->fieldValue(descriptor.domainField, AFields.form.fields).toString();
			login.password = FDataForms->fieldValue(descriptor.passwordField, AFields.form.fields).toString();
		}
		if (login.isValid && !descriptor.domainSeparator.isEmpty() && login.domain.isEmpty())
		{
			QStringList parts = login.login.split(descriptor.domainSeparator);
			login.login = parts.value(0);
			login.domain = parts.value(1);
			login.isValid = login.domain.isEmpty() || descriptor.domains.isEmpty() || descriptor.domains.contains(login.domain);
		}
	}
	return login;
}

IRegisterSubmit Gateways::serviceSubmit(const Jid &AStreamJid, const Jid &AServiceJid, const IGateServiceLogin &ALogin) const
{
	IRegisterSubmit submit;
	IGateServiceDescriptor descriptor = FDiscovery!=NULL ? findGateDescriptor(FDiscovery->discoInfo(AStreamJid, AServiceJid)) : IGateServiceDescriptor();
	if (!descriptor.id.isEmpty())
	{
		submit.fieldMask = ALogin.fields.fieldMask;
		submit.key = ALogin.fields.key;
		if (ALogin.fields.fieldMask > 0)
		{
			QString login = !ALogin.domainSeparator.isEmpty() ? ALogin.login + descriptor.domainSeparator + ALogin.domain : ALogin.login;
			if (ALogin.fields.fieldMask & IRegisterFields::Username)
				submit.username = login;
			else
				submit.email = login;
			submit.password = ALogin.password;
			submit.serviceJid = AServiceJid;
		}
		else if (FDataForms)
		{
			QMap<QString, QVariant> fields = descriptor.extraFields;
			if (ALogin.domainSeparator.isEmpty())
			{
				fields.insert(descriptor.loginField,ALogin.login);
			}
			else if (FDataForms->fieldIndex(descriptor.domainField,ALogin.fields.form.fields)>=0)
			{
				fields.insert(descriptor.loginField,ALogin.login);
				fields.insert(descriptor.domainField,ALogin.domain);
			}
			else
			{
				fields.insert(descriptor.loginField,ALogin.login + ALogin.domainSeparator + ALogin.domain);
			}
			fields.insert(descriptor.passwordField, ALogin.password);

			IDataForm form = ALogin.fields.form;
			QMap<QString, QVariant>::const_iterator it = fields.constBegin();
			while (it != fields.constEnd())
			{
				int index = FDataForms->fieldIndex(it.key(),form.fields);
				if (index >= 0)
					form.fields[index].value = it.value();
				it++;
			}

			submit.form = FDataForms->dataSubmit(form);
			if (FDataForms->isSubmitValid(form,submit.form))
				submit.serviceJid = AServiceJid;
		}
	}
	return submit;
}

bool Gateways::isServiceEnabled(const Jid &AStreamJid, const Jid &AServiceJid) const
{
	IRoster *roster = FRosterPlugin!=NULL ? FRosterPlugin->findRoster(AStreamJid) : NULL;
	if (roster)
	{
		IRosterItem ritem = roster->rosterItem(AServiceJid);
		return ritem.isValid && ritem.subscription!=SUBSCRIPTION_NONE;
	}
	return false;
}

bool Gateways::setServiceEnabled(const Jid &AStreamJid, const Jid &AServiceJid, bool AEnabled)
{
	IRoster *roster = FRosterPlugin!=NULL ? FRosterPlugin->findRoster(AStreamJid) : NULL;
	if (roster && roster->isOpen())
	{
		LogDetail(QString("[Gateways] Changing service '%1' state to enabled='%2'").arg(AServiceJid.full()).arg(AEnabled));
		if (AEnabled)
		{
			if (FRosterChanger)
				FRosterChanger->insertAutoSubscribe(AStreamJid,AServiceJid,true,true,false);
			roster->sendSubscription(AServiceJid,IRoster::Unsubscribe);
			roster->sendSubscription(AServiceJid,IRoster::Subscribe);
			sendLogPresence(AStreamJid,AServiceJid,true);
			setKeepConnection(AStreamJid,AServiceJid,true);
		}
		else
		{
			if (FRosterChanger)
				FRosterChanger->insertAutoSubscribe(AStreamJid,AServiceJid,true,false,true);
			setKeepConnection(AStreamJid,AServiceJid,false);
			roster->sendSubscription(AServiceJid,IRoster::Unsubscribe);
			roster->sendSubscription(AServiceJid,IRoster::Unsubscribed);
		}
		return true;
	}
	return false;
}

bool Gateways::changeService(const Jid &AStreamJid, const Jid &AServiceFrom, const Jid &AServiceTo, bool ARemove, bool ASubscribe)
{
	IRoster *roster = FRosterPlugin!=NULL ? FRosterPlugin->findRoster(AStreamJid) : NULL;
	IPresence *presence = FPresencePlugin!=NULL ? FPresencePlugin->findPresence(AStreamJid) : NULL;
	if (roster && presence && FRosterChanger && presence->isOpen() && AServiceFrom.isValid() && AServiceTo.isValid() && AServiceFrom.pDomain()!=AServiceTo.pDomain())
	{
		LogDetail(QString("[Gateways] Changing service from '%1' to '%2'").arg(AServiceFrom.full()).arg(AServiceTo.full()));
		
		IRosterItem ritemOld = roster->rosterItem(AServiceFrom);
		IRosterItem ritemNew = roster->rosterItem(AServiceTo);

		if (!presence->presenceItems(AServiceFrom).isEmpty())
			sendLogPresence(AStreamJid,AServiceFrom,false);

		if (FRegistration && ARemove)
			FRegistration->sendUnregiterRequest(AStreamJid,AServiceFrom);

		if (ritemOld.isValid && !ARemove)
			FRosterChanger->unsubscribeContact(AStreamJid,AServiceFrom,"",true);

		QList<IRosterItem> newItems, oldItems, curItems;
		foreach(IRosterItem ritem, roster->rosterItems())
		{
			if (ritem.itemJid.pDomain() == AServiceFrom.pDomain())
			{
				IRosterItem newItem = ritem;
				newItem.itemJid.setDomain(AServiceTo.domain());
				if (!roster->rosterItem(newItem.itemJid).isValid)
					newItems.append(newItem);
				else
					curItems += newItem;
				if (ARemove)
				{
					oldItems.append(ritem);
					FRosterChanger->insertAutoSubscribe(AStreamJid, ritem.itemJid, true, false, true);
				}
			}
		}
		roster->removeItems(oldItems);
		roster->setItems(newItems);

		if (ASubscribe)
		{
			curItems+=newItems;
			foreach(IRosterItem ritem, curItems)
				FRosterChanger->insertAutoSubscribe(AStreamJid,ritem.itemJid, true, true, false);
			FRosterChanger->insertAutoSubscribe(AStreamJid,AServiceTo,true,true,false);
			roster->sendSubscription(AServiceTo,IRoster::Subscribe);
		}

		return true;
	}
	return false;
}

QString Gateways::removeService(const Jid &AStreamJid, const Jid &AServiceJid, bool AWithContacts)
{
	IRoster *roster = FRosterPlugin!=NULL ? FRosterPlugin->findRoster(AStreamJid) : NULL;
	if (FRegistration && roster && roster->isOpen())
	{
		LogDetail(QString("[Gateways] Removing service '%1', with_contacts='%2'").arg(AServiceJid.full()).arg(AWithContacts));

		if (FRosterChanger)
			FRosterChanger->insertAutoSubscribe(AStreamJid,AServiceJid,true,false,true);

		QString requestId = FRegistration->sendUnregiterRequest(AStreamJid,AServiceJid.full());
		if (!requestId.isEmpty())
		{
			RemoveRequestParams params;
			params.streamJid = AStreamJid;
			params.serviceJid = AServiceJid;
			params.withContacts = AWithContacts;
			FRemoveRequests.insert(requestId, params);
			return requestId;
		}
	}
	return QString::null;
}

QString Gateways::legacyIdFromUserJid(const Jid &AUserJid) const
{
	QString legacyId = AUserJid.node();
	for (int i=1; i<legacyId.length(); i++)
	{
		if (legacyId.at(i)=='%' && legacyId.at(i-1)!='\\')
			legacyId[i] = '@';
	}
	return legacyId;
}

QString Gateways::legacyIdFromUserJid(const Jid &AStreamJid, const Jid &AUserJid) const
{
	if (streamServices(AStreamJid).contains(AUserJid.domain()))
		return legacyIdFromUserJid(AUserJid);
	return AUserJid.bare();
}

QString Gateways::sendLoginRequest(const Jid &AStreamJid, const Jid &AServiceJid)
{
	if (FRegistration)
	{
		LogDetail(QString("[Gateways] Sending login request to '%1'").arg(AServiceJid.full()));
		QString requestId = FRegistration->sendRegiterRequest(AStreamJid,AServiceJid);
		if (!requestId.isEmpty())
		{
			FLoginRequests.insert(requestId, AStreamJid);
			return requestId;
		}
	}
	return QString::null;
}

QString Gateways::sendPromptRequest(const Jid &AStreamJid, const Jid &AServiceJid)
{
	Stanza request("iq");
	request.setType("get").setTo(AServiceJid.eFull()).setId(FStanzaProcessor->newId());
	request.addElement("query",NS_JABBER_GATEWAY);
	if (FStanzaProcessor->sendStanzaRequest(this,AStreamJid,request,GATEWAY_TIMEOUT))
	{
		LogDetail(QString("[Gateways] Gateway prompt request sent to '%1' id='%2'").arg(AServiceJid.full(),request.id()));
		FPromptRequests.append(request.id());
		return request.id();
	}
	return QString::null;
}

QString Gateways::sendUserJidRequest(const Jid &AStreamJid, const Jid &AServiceJid, const QString &AContactID)
{
	Stanza request("iq");
	request.setType("set").setTo(AServiceJid.eFull()).setId(FStanzaProcessor->newId());
	QDomElement elem = request.addElement("query",NS_JABBER_GATEWAY);
	elem.appendChild(request.createElement("prompt")).appendChild(request.createTextNode(AContactID));
	if (FStanzaProcessor->sendStanzaRequest(this,AStreamJid,request,GATEWAY_TIMEOUT))
	{
		LogDetail(QString("[Gateways] User JID request sent to '%1' id='%2'").arg(AServiceJid.full(),request.id()));
		FUserJidRequests.append(request.id());
		return request.id();
	}
	return QString::null;
}

QDialog *Gateways::showAddLegacyAccountDialog(const Jid &AStreamJid, const Jid &AServiceJid, QWidget *AParent)
{
	IPresence *presence = FPresencePlugin!=NULL ? FPresencePlugin->findPresence(AStreamJid) : NULL;
	if (FRegistration && presence && presence->isOpen())
	{
		QDialog *dialog;
		if (serviceDescriptor(AStreamJid,AServiceJid).id == GSID_FACEBOOK)
			dialog = new AddFacebookAccountDialog(this,FRegistration,presence,AServiceJid,AParent);
		else
			dialog = new AddLegacyAccountDialog(this,FRegistration,presence,AServiceJid,AParent);
		connect(presence->instance(),SIGNAL(closed()),dialog,SLOT(reject()));

		CustomBorderContainer *border = CustomBorderStorage::staticStorage(RSR_STORAGE_CUSTOMBORDER)->addBorder(dialog, CBS_DIALOG);
		if (border)
		{
			border->setAttribute(Qt::WA_DeleteOnClose, true);
			border->setMaximizeButtonVisible(false);
			border->setMinimizeButtonVisible(false);
			connect(border, SIGNAL(closeClicked()), dialog, SLOT(reject()));
			connect(dialog, SIGNAL(rejected()), border, SLOT(close()));
			connect(dialog, SIGNAL(accepted()), border, SLOT(close()));
			border->setResizable(false);
			border->show();
			border->adjustSize();
		}
		else
		{
			dialog->show();
		}
		return dialog;
	}
	return NULL;
}

void Gateways::registerDiscoFeatures()
{
	IDiscoFeature dfeature;
	dfeature.active = false;
	dfeature.var = NS_JABBER_GATEWAY;
	dfeature.name = tr("Gateway Interaction");
	dfeature.description = tr("Supports the adding of the contact by the username of the legacy system");
	FDiscovery->insertDiscoFeature(dfeature);
}

void Gateways::startAutoLogin(const Jid &AStreamJid)
{
	if (FDiscovery && FRegistration)
	{
		IDiscoItems ditems = FStreamDiscoItems.value(AStreamJid);
		foreach(IDiscoItem ditem, ditems.items)
		{
			if (!FStreamAutoRegServices.contains(AStreamJid,ditem.itemJid))
			{
				IGateServiceDescriptor descriptor = findGateDescriptor(FDiscovery->discoInfo(AStreamJid,ditem.itemJid));
				if (!descriptor.id.isEmpty() && descriptor.autoLogin)
				{
					LogDetail(QString("[Gateways] Sending auto login register request to '%1'").arg(ditem.itemJid.full()));
					QString requestId = FRegistration->sendRegiterRequest(AStreamJid,ditem.itemJid);
					if (!requestId.isEmpty())
					{
						FAutoLoginRequests.insert(requestId,qMakePair<Jid,Jid>(AStreamJid,ditem.itemJid));
						FStreamAutoRegServices.insertMulti(AStreamJid,ditem.itemJid);
					}
				}
			}
		}
	}
}

IGateServiceDescriptor Gateways::findGateDescriptor(const IDiscoInfo &AInfo) const
{
	int index = FDiscovery ? FDiscovery->findIdentity(AInfo.identity,"gateway",QString::null) : -1;
	if (index >= 0)
	{
		QString domain = AInfo.contactJid.pDomain();
		QString identType = AInfo.identity.at(index).type.toLower();
		for (QList<IGateServiceDescriptor>::const_iterator it = FGateDescriptors.constBegin() ; it!=FGateDescriptors.constEnd(); it++)
		{
			QRegExp regexp(QString(GATE_PREFIX_PATTERN).arg(it->prefix));
			if (it->type==identType && (it->prefix.isEmpty() || regexp.exactMatch(domain)))
				return *it;
		}
	}
	return IGateServiceDescriptor();
}

void Gateways::insertConflictNotice(const Jid &AStreamJid, const Jid &AServiceJid, const QString &ALogin)
{
	IInternalNoticeWidget *noticeWidget = FMainWindowPlugin!=NULL ? FMainWindowPlugin->mainWindow()->noticeWidget() : NULL;
	if (noticeWidget && !FConflictNotices.value(AStreamJid).contains(AServiceJid))
	{
		IGateServiceDescriptor descriptor = serviceDescriptor(AStreamJid,AServiceJid);

		IInternalNotice notice;
		notice.priority = INP_GATEWAYS_CONFLICT;
		notice.iconKey = descriptor.iconKey;
		notice.iconStorage = RSR_STORAGE_MENUICONS;
		notice.caption = tr("Account disconnected");
		notice.message = QString("%1<br><i>%2</i>").arg(ALogin).arg(tr("disconnected"));

		Action *action = new Action(this);
		action->setText(tr("Enable"));
		action->setData(ADR_STREAM_JID,AStreamJid.full());
		action->setData(ADR_SERVICE_JID,AServiceJid.bare());
		connect(action,SIGNAL(triggered()),SLOT(onInternalConflictNoticeActionTriggered()));
		notice.actions.append(action);

		FConflictNotices[AStreamJid].insert(AServiceJid, noticeWidget->insertNotice(notice));
	}
}

void Gateways::removeConflictNotice(const Jid &AStreamJid, const Jid &AServiceJid)
{
	IInternalNoticeWidget *noticeWidget = FMainWindowPlugin!=NULL ? FMainWindowPlugin->mainWindow()->noticeWidget() : NULL;
	if (noticeWidget)
		noticeWidget->removeNotice(FConflictNotices.value(AStreamJid).value(AServiceJid));
}

void Gateways::onXmppStreamOpened(IXmppStream *AXmppStream)
{
	if (FOptionsManager)
	{
		FOptionsStreamJid = AXmppStream->streamJid();
		IOptionsDialogNode dnode = { ONO_GATEWAYS, OPN_GATEWAYS, tr("Accounts"), MNI_GATEWAYS_ACCOUNTS_OPTIONS };
		FOptionsManager->insertOptionsDialogNode(dnode);
	}
	if (FDiscovery)
	{
		FDiscovery->requestDiscoItems(AXmppStream->streamJid(),AXmppStream->streamJid().domain());
	}
}

void Gateways::onXmppStreamClosed(IXmppStream *AXmppStream)
{
	if (FOptionsManager)
		FOptionsManager->removeOptionsDialogNode(OPN_GATEWAYS);

	foreach(int notifyId, FConflictNotifies.keys(AXmppStream->streamJid()))
		FNotifications->removeNotification(notifyId);

	foreach(int noticeId, FConflictNotices.value(AXmppStream->streamJid()).values())
		FMainWindowPlugin->mainWindow()->noticeWidget()->removeNotice(noticeId);
	FConflictNotices.remove(AXmppStream->streamJid());

	FResolveNicks.remove(AXmppStream->streamJid());
	FStreamDiscoItems.remove(AXmppStream->streamJid());
	FStreamAutoRegServices.remove(AXmppStream->streamJid());
}

void Gateways::onRosterOpened(IRoster *ARoster)
{
	if (FDiscovery)
	{
		foreach(IRosterItem ritem, ARoster->rosterItems())
			if (ritem.itemJid.node().isEmpty() && !FDiscovery->hasDiscoInfo(ARoster->streamJid(),ritem.itemJid))
				FDiscovery->requestDiscoInfo(ARoster->streamJid(),ritem.itemJid);
	}
	if (FPrivateStorage)
	{
		FPrivateStorage->loadData(ARoster->streamJid(),PST_GATEWAYS_SERVICES,PSN_GATEWAYS_KEEP);
		FKeepTimer.start(KEEP_INTERVAL);
	}
	startAutoLogin(ARoster->streamJid());
}

void Gateways::onRosterItemReceived(IRoster *ARoster, const IRosterItem &AItem, const IRosterItem &ABefore)
{
	Q_UNUSED(ABefore);
	if (AItem.itemJid.node().isEmpty())
	{
		if (AItem.subscription!=SUBSCRIPTION_NONE && AItem.subscription!=SUBSCRIPTION_REMOVE)
		{
			removeConflictNotice(ARoster->streamJid(),AItem.itemJid);
			emit serviceEnableChanged(ARoster->streamJid(),AItem.itemJid,true);
		}
		else if (AItem.ask != SUBSCRIPTION_SUBSCRIBE)
		{
			emit serviceEnableChanged(ARoster->streamJid(),AItem.itemJid,false);
		}
		emit streamServicesChanged(ARoster->streamJid());
	}
}

void Gateways::onPresenceItemReceived(IPresence *APresence, const IPresenceItem &AItem, const IPresenceItem &ABefore)
{
	Q_UNUSED(ABefore);
	if (AItem.itemJid.node().isEmpty())
	{
		QString conflictCond = ErrorHandler::conditionByCode(ErrorHandler::CONFLICT);
		if (AItem.errCondition==conflictCond && AItem.errCondition!=ABefore.errCondition && isServiceEnabled(APresence->streamJid(),AItem.itemJid))
		{
			setServiceEnabled(APresence->streamJid(),AItem.itemJid,false);
			if (FNotifications)
			{
				INotification notify;
				notify.kinds = FNotifications->enabledTypeNotificationKinds(NNT_BIRTHDAY_REMIND);
				if ((notify.kinds & (INotification::PopupWindow|INotification::SoundPlay))>0)
				{
					IGateServiceDescriptor descriptor = serviceDescriptor(APresence->streamJid(),AItem.itemJid);
					notify.typeId = NNT_GATEWAYS_CONFLICT;
					notify.data.insert(NDR_STREAM_JID,APresence->streamJid().full());
					notify.data.insert(NDR_POPUP_IMAGE,IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getImage(descriptor.iconKey));
					notify.data.insert(NDR_POPUP_STYLEKEY,STS_NOTIFICATION_NOTIFYWIDGET);
					notify.data.insert(NDR_SOUND_FILE,SDF_GATEWAYS_CONFLICT);
					if (descriptor.id == GSID_ICQ)
					{
						notify.data.insert(NDR_POPUP_TITLE,tr("ICQ disconnected (offline)"));
						notify.data.insert(NDR_POPUP_TEXT,tr("Your ICQ was connected from another computer. You can enable ICQ again."));
					}
					else if (descriptor.id == GSID_MAGENT)
					{
						notify.data.insert(NDR_POPUP_TITLE,tr("Agent@Mail disconnected (offline)"));
						notify.data.insert(NDR_POPUP_TEXT,tr("Your Agent@Mail was connected from another computer. You can enable it again."));
					}
					else
					{
						notify.data.insert(NDR_POPUP_TITLE,tr("Account %1 disconnected").arg(descriptor.name));
						notify.data.insert(NDR_POPUP_TEXT,tr("Your account %1 was connected from another computer. You can enable it again.").arg(descriptor.name));
					}
					FConflictNotifies.insert(FNotifications->appendNotification(notify), APresence->streamJid());
				}
			}
			if (FMainWindowPlugin && !FConflictNotices.value(APresence->streamJid()).contains(AItem.itemJid))
			{
				LogDetail(QString("[Gateways] Sending conflict login request to '%1'").arg(AItem.itemJid.full()));
				QString requestId = FRegistration->sendRegiterRequest(APresence->streamJid(),AItem.itemJid);
				if (!requestId.isEmpty())
					FConflictLoginRequests.insert(requestId,APresence->streamJid());
			}
		}
		emit servicePresenceChanged(APresence->streamJid(),AItem.itemJid,AItem);
	}
}

void Gateways::onPrivateStorateOpened(const Jid &AStreamJid)
{
	Q_UNUSED(AStreamJid);
}

void Gateways::onPrivateStorageLoaded(const QString &AId, const Jid &AStreamJid, const QDomElement &AElement)
{
	Q_UNUSED(AId);
	if (AElement.tagName() == PST_GATEWAYS_SERVICES && AElement.namespaceURI() == PSN_GATEWAYS_KEEP)
	{
		IRoster *roster = FRosterPlugin!=NULL ? FRosterPlugin->findRoster(AStreamJid) : NULL;
		if (roster)
		{
			QDomElement elem = AElement.firstChildElement("service");
			while (!elem.isNull())
			{
				IRosterItem ritem = roster->rosterItem(elem.text());
				if (ritem.isValid && isServiceEnabled(AStreamJid,ritem.itemJid))
					setKeepConnection(AStreamJid,ritem.itemJid,true);
				elem = elem.nextSiblingElement("service");
			}
		}
	}
}

void Gateways::onPrivateStorateAboutToClose(const Jid &AStreamJid)
{
	QDomDocument doc;
	doc.appendChild(doc.createElement("services"));
	QDomElement elem = doc.documentElement().appendChild(doc.createElementNS(PSN_GATEWAYS_KEEP,PST_GATEWAYS_SERVICES)).toElement();
	foreach(Jid service, FKeepConnections.value(AStreamJid))
		elem.appendChild(doc.createElement("service")).appendChild(doc.createTextNode(service.eBare()));
	FPrivateStorage->saveData(AStreamJid,elem);
}

void Gateways::onPrivateStorateClosed(const Jid &AStreamJid)
{
	FKeepConnections.remove(AStreamJid);
}

void Gateways::onKeepTimerTimeout()
{
	foreach(Jid streamJid, FKeepConnections.keys())
	{
		IRoster *roster = FRosterPlugin!=NULL ? FRosterPlugin->findRoster(streamJid) : NULL;
		IPresence *presence = FPresencePlugin!=NULL ? FPresencePlugin->findPresence(streamJid) : NULL;
		if (roster && presence && presence->isOpen())
		{
			QSet<Jid> services = FKeepConnections.value(streamJid);
			foreach(Jid service, services)
			{
				if (roster->rosterItem(service).isValid)
				{
					const QList<IPresenceItem> pitems = presence->presenceItems(service);
					if (pitems.isEmpty() || pitems.at(0).show==IPresence::Error)
					{
						presence->sendPresence(service,IPresence::Offline,QString::null,0);
						presence->sendPresence(service,presence->show(),presence->status(),presence->priority());
					}
				}
			}
		}
	}
}

void Gateways::onVCardReceived(const Jid &AContactJid)
{
	if (FResolveNicks.contains(AContactJid))
	{
		QList<Jid> streamJids = FResolveNicks.values(AContactJid);
		foreach(Jid streamJid, streamJids)
			resolveNickName(streamJid,AContactJid);
		FResolveNicks.remove(AContactJid);
	}
}

void Gateways::onVCardError(const Jid &AContactJid, const QString &AError)
{
	Q_UNUSED(AError);
	FResolveNicks.remove(AContactJid);
}

void Gateways::onDiscoInfoChanged(const IDiscoInfo &AInfo)
{
	if (AInfo.contactJid.node().isEmpty() && AInfo.node.isEmpty())
	{
		IRoster *roster = FRosterPlugin!=NULL ? FRosterPlugin->findRoster(AInfo.streamJid) : NULL;
		if (roster && roster->isOpen())
			startAutoLogin(roster->streamJid());
		if (roster && roster->rosterItem(AInfo.contactJid).isValid)
			emit streamServicesChanged(AInfo.streamJid);
		emit availServicesChanged(AInfo.streamJid);
	}
}

void Gateways::onDiscoItemsReceived(const IDiscoItems &AItems)
{
	if (AItems.contactJid==AItems.streamJid.domain() && AItems.node.isEmpty())
	{
		foreach(IDiscoItem ditem, AItems.items)
			if (!FDiscovery->hasDiscoInfo(AItems.streamJid,ditem.itemJid))
				FDiscovery->requestDiscoInfo(AItems.streamJid,ditem.itemJid);
		FStreamDiscoItems.insert(AItems.streamJid,AItems);
		emit availServicesChanged(AItems.streamJid);
	}
}

void Gateways::onRegisterFields(const QString &AId, const IRegisterFields &AFields)
{
	if (FLoginRequests.contains(AId))
	{
		Jid streamJid = FLoginRequests.take(AId);
		IGateServiceLogin gslogin = serviceLogin(streamJid,AFields.serviceJid,AFields);
		if (gslogin.isValid)
		{
			QString login = gslogin.login;
			if (!gslogin.domain.isEmpty())
				login += "@" + gslogin.domain;
			LogDetail(QString("[Gateways] Gateway login received from '%1' id='%2': %3").arg(AFields.serviceJid.full(),AId,login));
			emit loginReceived(AId, login);
		}
		else
		{
			LogError(QString("[Gateways] Failed to receive gateway login from '%1' id='%2': Unsupported registration fields").arg(AFields.serviceJid.full(),AId));
			emit errorReceived(AId, tr("Unsupported gateway type"));
		}
	}
	else if (FAutoLoginRequests.contains(AId))
	{
		LogDetail(QString("[Gateways] Auto login register fields received from '%1' id='%2' registered='%3'").arg(AFields.serviceJid.full(),AId).arg(AFields.registered));
		Jid streamJid = FAutoLoginRequests.take(AId).first;
		if (!AFields.registered)
		{
			IRegisterSubmit submit;
			submit.key = AFields.key;
			submit.fieldMask = AFields.fieldMask;
			submit.serviceJid = AFields.serviceJid;
			submit.username = streamJid.pBare();

			LogDetail(QString("[Gateways] Sending auto login register submit to '%1'").arg(submit.serviceJid.full()));
			QString submitId = FRegistration->sendSubmit(streamJid,submit);
			if (!submitId.isEmpty())
				FAutoLoginRequests.insert(submitId,qMakePair<Jid,Jid>(streamJid,AFields.serviceJid));
		}
		else if (FRosterPlugin)
		{
			IRoster *roster = FRosterPlugin->findRoster(streamJid);
			IRosterItem ritem = roster!=NULL ? roster->rosterItem(AFields.serviceJid) : IRosterItem();
			if (ritem.subscription != SUBSCRIPTION_BOTH)
			{
				if (ritem.subscription == SUBSCRIPTION_TO)
					roster->sendSubscription(AFields.serviceJid,IRoster::Unsubscribe);
				roster->sendSubscription(AFields.serviceJid,IRoster::Subscribe);
			}
		}
	}
	else if (FConflictLoginRequests.contains(AId))
	{
		Jid streamJid = FConflictLoginRequests.take(AId);
		IGateServiceLogin gslogin = serviceLogin(streamJid,AFields.serviceJid,AFields);
		if (gslogin.isValid)
		{
			QString login = gslogin.login;
			if (!gslogin.domain.isEmpty())
				login += "@" + gslogin.domain;
			LogDetail(QString("[Gateways] Conflict notice login received from '%1' id='%2': %3").arg(AFields.serviceJid.full(),AId,login));
			insertConflictNotice(streamJid,AFields.serviceJid,login);
		}
		else
		{
			LogError(QString("[Gateways] Failed to receive conflict notice login from '%1' id='%2': Unsupported registration fields").arg(AFields.serviceJid.full(),AId));
		}
	}
}

void Gateways::onRegisterSuccess(const QString &AId)
{
	if (FAutoLoginRequests.contains(AId))
	{
		QPair<Jid,Jid> service = FAutoLoginRequests.take(AId);
		LogDetail(QString("[Gateways] Auto login registration finished on '%1' id='%2'").arg(service.second.full(),AId));
		setServiceEnabled(service.first,service.second,true);
	}
	else if (FRemoveRequests.contains(AId))
	{
		RemoveRequestParams params = FRemoveRequests.take(AId);
		LogDetail(QString("[Gateways] Registration removed on '%1' id='%2'").arg(params.serviceJid.full(),AId));
		IRoster *roster = FRosterPlugin!=NULL ? FRosterPlugin->findRoster(params.streamJid) : NULL;
		if (roster && roster->isOpen())
		{
			roster->removeItem(params.serviceJid);
			if (params.withContacts)
			{
				foreach(IRosterItem ritem, roster->rosterItems())
					if (ritem.itemJid!=params.serviceJid && ritem.itemJid.pDomain()==params.serviceJid.pDomain())
						roster->removeItem(ritem.itemJid);
			}
			LogDetail(QString("[Gateways] Service '%1' removed from roster id='%2'").arg(params.serviceJid.full(),AId));
			emit serviceRemoved(AId);
		}
		else
		{
			LogError(QString("[Gateways] Failed to remove service '%1' from roster id='%2'").arg(params.serviceJid.full(),AId));
			emit errorReceived(AId, tr("Failed to remove service from roster"));
		}
	}
}

void Gateways::onRegisterError(const QString &AId, const QString &ACondition, const QString &AMessage)
{
	Q_UNUSED(ACondition);
	if (FLoginRequests.contains(AId))
	{
		LogError(QString("[Gateway] Failed to receive gateway login id='%1': %2").arg(AId,AMessage));
		FLoginRequests.remove(AId);
		emit errorReceived(AId,AMessage);
	}
	else if (FAutoLoginRequests.contains(AId))
	{
		LogError(QString("[Gateway] Failed to receive auto login id='%1': %2").arg(AId,AMessage));
		FAutoLoginRequests.remove(AId);
	}
	else if (FConflictLoginRequests.contains(AId))
	{
		LogError(QString("[Gateway] Failed to receive conflict notice login id='%1': %2").arg(AId,AMessage));
		FConflictLoginRequests.remove(AId);
	}
	else if (FRemoveRequests.contains(AId))
	{
		RemoveRequestParams params = FRemoveRequests.take(AId);
		LogError(QString("[Gateway] Failed to remove service '%1' registration id='%2': %3").arg(params.serviceJid.full(),AId,AMessage));
		emit errorReceived(AId, AMessage);
	}
}

void Gateways::onInternalNoticeReady()
{
	IInternalNoticeWidget *widget = FMainWindowPlugin->mainWindow()->noticeWidget();
	if (widget->isEmpty())
	{
		IDiscoIdentity identity;
		identity.category = "gateway";
		if (streamServices(FOptionsStreamJid,identity).isEmpty() && !availServices(FOptionsStreamJid,identity).isEmpty())
		{
			int showCount = Options::node(OPV_GATEWAYS_NOTICE_SHOWCOUNT).value().toInt();
			int removeCount = Options::node(OPV_GATEWAYS_NOTICE_REMOVECOUNT).value().toInt();
			QDateTime showLast = Options::node(OPV_GATEWAYS_NOTICE_SHOWLAST).value().toDateTime();
			if (showCount <= 3 && (!showLast.isValid() || showLast.daysTo(QDateTime::currentDateTime())>=7*removeCount))
			{
				IInternalNotice notice;
				notice.priority = INP_DEFAULT;
				notice.iconStorage = RSR_STORAGE_MENUICONS;
				notice.caption = tr("Add your accounts");
				notice.message = Qt::escape(tr("Add your accounts and send messages to your friends on these services"));

				Action *action = new Action(this);
				action->setText(tr("Add my accounts..."));
				connect(action,SIGNAL(triggered()),SLOT(onInternalAccountNoticeActionTriggered()));
				notice.actions.append(action);

				FInternalNoticeId = widget->insertNotice(notice);
				Options::node(OPV_GATEWAYS_NOTICE_SHOWCOUNT).setValue(showCount+1);
				Options::node(OPV_GATEWAYS_NOTICE_SHOWLAST).setValue(QDateTime::currentDateTime());
			}
		}
	}
}

void Gateways::onInternalAccountNoticeActionTriggered()
{
	if (FOptionsManager)
	{
		FOptionsManager->showOptionsDialog(OPN_GATEWAYS);
	}
}

void Gateways::onInternalConflictNoticeActionTriggered()
{
	Action *action = qobject_cast<Action *>(sender());
	if (action)
	{
		setServiceEnabled(action->data(ADR_STREAM_JID).toString(),action->data(ADR_SERVICE_JID).toString(),true);
	}
}

void Gateways::onInternalNoticeRemoved(int ANoticeId)
{
	if (ANoticeId>0 && ANoticeId==FInternalNoticeId)
	{
		int removeCount = Options::node(OPV_GATEWAYS_NOTICE_REMOVECOUNT).value().toInt();
		Options::node(OPV_GATEWAYS_NOTICE_REMOVECOUNT).setValue(removeCount+1);
		FInternalNoticeId = -1;
	}
	else foreach(Jid streamJid, FConflictNotices.keys())
	{
		Jid serviceJid = FConflictNotices.value(streamJid).key(ANoticeId);
		if (serviceJid.isValid())
		{
			FConflictNotices[streamJid].remove(serviceJid);
			break;
		}
	}
}

void Gateways::onNotificationActivated(int ANotifyId)
{
	if (FConflictNotifies.contains(ANotifyId))
	{
		if (FMainWindowPlugin)
			FMainWindowPlugin->showMainWindow();
		FNotifications->removeNotification(ANotifyId);
	}
}

void Gateways::onNotificationRemoved(int ANotifyId)
{
	if (FConflictNotifies.contains(ANotifyId))
	{
		FConflictNotifies.remove(ANotifyId);
	}
}


Q_EXPORT_PLUGIN2(plg_gateways, Gateways)
