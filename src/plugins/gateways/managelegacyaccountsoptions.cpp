#include "managelegacyaccountsoptions.h"

ManageLegacyAccountsOptions::ManageLegacyAccountsOptions(IGateways *AGateways, IRosterPlugin *ARosterPlugin, const Jid &AStreamJid, QWidget *AParent) : QWidget(AParent)
{
	ui.setupUi(this);
	FGateways = AGateways;
	FStreamJid = AStreamJid;

	FLayout = new QVBoxLayout(ui.wdtAccounts);
	FLayout->setMargin(0);

	IRoster *roster = ARosterPlugin!=NULL ? ARosterPlugin->getRoster(FStreamJid) : NULL;
	if (roster)
	{
		connect(roster->instance(),SIGNAL(received(const IRosterItem &)),SLOT(onRosterItemChanged(const IRosterItem &)));
		connect(roster->instance(),SIGNAL(removed(const IRosterItem &)),SLOT(onRosterItemChanged(const IRosterItem &)));
	}

	IDiscoIdentity identity;
	identity.category = "gateway";
	foreach(Jid serviceJid, FGateways->streamServices(FStreamJid,identity))
		appendServiceOptions(serviceJid);

	ui.lblNoAccount->setVisible(FOptions.isEmpty());
}

ManageLegacyAccountsOptions::~ManageLegacyAccountsOptions()
{

}

void ManageLegacyAccountsOptions::apply()
{

}

void ManageLegacyAccountsOptions::reset()
{

}

void ManageLegacyAccountsOptions::appendServiceOptions(const Jid &AServiceJid)
{
	if (!FOptions.contains(AServiceJid))
	{
		LegacyAccountOptions *options = new LegacyAccountOptions(FGateways,FStreamJid,AServiceJid,ui.wdtAccounts);
		FLayout->addWidget(options);
		FOptions.insert(AServiceJid,options);
	}
}

void ManageLegacyAccountsOptions::removeServiceOptions(const Jid &AServiceJid)
{
	if (FOptions.contains(AServiceJid))
	{
		LegacyAccountOptions *options = FOptions.take(AServiceJid);
		FLayout->removeWidget(options);
		options->deleteLater();
	}
}

void ManageLegacyAccountsOptions::onRosteritemChanged(const IRosterItem &AItem)
{
	if (AItem.subscription == SUBSCRIPTION_REMOVE)
	{
		removeServiceOptions(AItem.itemJid);
	}
}
