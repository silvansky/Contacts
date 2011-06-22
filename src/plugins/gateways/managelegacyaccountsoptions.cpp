#include "managelegacyaccountsoptions.h"

ManageLegacyAccountsOptions::ManageLegacyAccountsOptions(IGateways *AGateways, const Jid &AStreamJid, QWidget *AParent) : QWidget(AParent)
{
	ui.setupUi(this);
	FGateways = AGateways;
	FStreamJid = AStreamJid;

	connect(FGateways->instance(),SIGNAL(streamServicesChanged(const Jid &)),SLOT(onStreamServicesChanged(const Jid &)));

	FLayout = new QVBoxLayout();
	ui.wdtAccounts->setLayout(FLayout);
	FLayout->setMargin(0);
	FLayout->setSpacing(0);

	onStreamServicesChanged(FStreamJid);
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
		IGateServiceDescriptor descriptor = FGateways->serviceDescriptor(FStreamJid,AServiceJid);
		if (!descriptor.id.isEmpty() && descriptor.needLogin)
		{
			LegacyAccountOptions *options = new LegacyAccountOptions(FGateways,FStreamJid,AServiceJid,ui.wdtAccounts);
			if (FLayout->count() && !qobject_cast<QFrame*>(FLayout->itemAt(FLayout->count() - 1)->widget()))
			{
				QFrame * frame = new QFrame;
				frame->setObjectName("serviceSeparator");
				FLayout->addWidget(frame);
			}
			FLayout->addWidget(options);
			FOptions.insert(AServiceJid,options);
		}
	}
}

void ManageLegacyAccountsOptions::removeServiceOptions(const Jid &AServiceJid)
{
	if (FOptions.contains(AServiceJid))
	{
		LegacyAccountOptions *options = FOptions.take(AServiceJid);
		int i = FLayout->indexOf(options);
		if (i && qobject_cast<QFrame*>(FLayout->itemAt(i - 1)->widget()))
			FLayout->takeAt(i - 1)->widget()->deleteLater();
		FLayout->removeWidget(options);
		options->deleteLater();
	}
}

void ManageLegacyAccountsOptions::onStreamServicesChanged(const Jid &AStreamJid)
{
	if (FStreamJid == AStreamJid)
	{
		IDiscoIdentity identity;
		identity.category = "gateway";

		QList<Jid> curGates = FGateways->streamServices(FStreamJid,identity);

		foreach(Jid serviceJid, curGates)
			appendServiceOptions(serviceJid);

		foreach(Jid serviceJid, FOptions.keys().toSet() - curGates.toSet())
			removeServiceOptions(serviceJid);

		ui.lblNoAccount->setVisible(FOptions.isEmpty());
		emit modified();
	}
}
