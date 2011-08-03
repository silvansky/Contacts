#include "accountmanager.h"

#define ADR_ACCOUNT_ID              Action::DR_Parametr1

AccountManager::AccountManager()
{
	FOptionsManager = NULL;
}

AccountManager::~AccountManager()
{

}

//IPlugin
void AccountManager::pluginInfo(IPluginInfo *APluginInfo)
{
	APluginInfo->name = tr("Account Manager");
	APluginInfo->description = tr("Allows to create and manage Jabber accounts");
	APluginInfo->version = "1.0";
	APluginInfo->author = "Potapov S.A. aka Lion";
	APluginInfo->homePage = "http://contacts.rambler.ru";
	APluginInfo->dependences.append(XMPPSTREAMS_UUID);
}

bool AccountManager::initConnections(IPluginManager *APluginManager, int &/*AInitOrder*/)
{
	IPlugin *plugin = APluginManager->pluginInterface("IXmppStreams").value(0,NULL);
	if (plugin)
		FXmppStreams = qobject_cast<IXmppStreams *>(plugin->instance());

	plugin = APluginManager->pluginInterface("IOptionsManager").value(0,NULL);
	if (plugin)
	{
		FOptionsManager = qobject_cast<IOptionsManager *>(plugin->instance());
		if (FOptionsManager)
		{
			connect(FOptionsManager->instance(),SIGNAL(profileOpened(const QString &)),SLOT(onProfileOpened(const QString &)));
			connect(FOptionsManager->instance(),SIGNAL(profileClosed(const QString &)),SLOT(onProfileClosed(const QString &)));
		}
	}

	connect(Options::instance(),SIGNAL(optionsOpened()),SLOT(onOptionsOpened()));
	connect(Options::instance(),SIGNAL(optionsClosed()),SLOT(onOptionsClosed()));

	return FXmppStreams!=NULL;
}

bool AccountManager::initSettings()
{
	return true;
}

QList<IAccount *> AccountManager::accounts() const
{
	return FAccounts.values();
}

IAccount *AccountManager::accountById(const QUuid &AAcoountId) const
{
	return FAccounts.value(AAcoountId);
}

IAccount *AccountManager::accountByStream(const Jid &AStreamJid) const
{
	foreach(IAccount *account, FAccounts)
	{
		if (account->xmppStream() && account->xmppStream()->streamJid()==AStreamJid)
			return account;
		else if (account->streamJid() == AStreamJid)
			return account;
	}
	return NULL;
}

IAccount *AccountManager::appendAccount(const QUuid &AAccountId)
{
	if (!AAccountId.isNull() && !FAccounts.contains(AAccountId))
	{
		Account *account = new Account(FXmppStreams,Options::node(OPV_ACCOUNT_ITEM,AAccountId.toString()),this);
		connect(account,SIGNAL(activeChanged(bool)),SLOT(onAccountActiveChanged(bool)));
		connect(account,SIGNAL(optionsChanged(const OptionsNode &)),SLOT(onAccountOptionsChanged(const OptionsNode &)));
		FAccounts.insert(AAccountId,account);
		emit appended(account);
		return account;
	}
	return FAccounts.value(AAccountId);
}

void AccountManager::showAccount(const QUuid &AAccountId)
{
	IAccount *account = FAccounts.value(AAccountId);
	if (account)
		account->setActive(true);
}

void AccountManager::hideAccount(const QUuid &AAccountId)
{
	IAccount *account = FAccounts.value(AAccountId);
	if (account)
		account->setActive(false);
}

void AccountManager::removeAccount(const QUuid &AAccountId)
{
	IAccount *account = FAccounts.value(AAccountId);
	if (account)
	{
		hideAccount(AAccountId);
		emit removed(account);
		FAccounts.remove(AAccountId);
		delete account->instance();
	}
}

void AccountManager::destroyAccount(const QUuid &AAccountId)
{
	IAccount *account = FAccounts.value(AAccountId);
	if (account)
	{
		hideAccount(AAccountId);
		removeAccount(AAccountId);
		Options::node(OPV_ACCOUNT_ROOT).removeChilds("account",AAccountId.toString());
		emit destroyed(AAccountId);
	}
}

void AccountManager::onProfileOpened(const QString &AProfile)
{
	Q_UNUSED(AProfile);
	foreach(IAccount *account, FAccounts)
		account->setActive(Options::node(OPV_ACCOUNT_ITEM,account->accountId()).value("active").toBool());
}

void AccountManager::onProfileClosed(const QString &AProfile)
{
	Q_UNUSED(AProfile);
	foreach(IAccount *account, FAccounts)
	{
		Options::node(OPV_ACCOUNT_ITEM,account->accountId()).setValue(account->isActive(),"active");
		account->setActive(false);
	}
}

void AccountManager::onOptionsOpened()
{
	foreach(QString id, Options::node(OPV_ACCOUNT_ROOT).childNSpaces("account"))
		appendAccount(id);
}

void AccountManager::onOptionsClosed()
{
	foreach(QUuid id, FAccounts.keys())
		removeAccount(id);
}

void AccountManager::onAccountActiveChanged(bool AActive)
{
	IAccount *account = qobject_cast<IAccount *>(sender());
	if (account)
	{
		if (AActive)
			emit shown(account);
		else
			emit hidden(account);
	}
}

void AccountManager::onAccountOptionsChanged(const OptionsNode &ANode)
{
	Account *account = qobject_cast<Account *>(sender());
	if (account)
		emit changed(account, ANode);
}

Q_EXPORT_PLUGIN2(plg_accountmanager, AccountManager)
