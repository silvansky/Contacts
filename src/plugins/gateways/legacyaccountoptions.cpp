#include "legacyaccountoptions.h"

#include <QMessageBox>

LegacyAccountOptions::LegacyAccountOptions(IGateways *AGateways, const Jid &AStreamJid, const Jid &AServiceJid, QWidget *AParent) : QWidget(AParent)
{
	ui.setupUi(this);
	FGateways = AGateways;
	FStreamJid = AStreamJid;
	FServiceJid = AServiceJid;

	FGateLabel = FGateways->serviceDescriptor(FStreamJid,FServiceJid);
	ui.lblLogin->setText(FGateLabel.isValid ? FGateLabel.name : FServiceJid.full());
	FLoginRequest = FGateways->sendLoginRequest(FStreamJid,FServiceJid);
	IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->insertAutoIcon(ui.lblIcon,FGateLabel.iconKey,0,0,"pixmap");

	connect(ui.pbtEnable,SIGNAL(clicked(bool)),SLOT(onEnableButtonClicked(bool)));
	connect(ui.pbtDisable,SIGNAL(clicked(bool)),SLOT(onDisableButtonClicked(bool)));
	connect(ui.lblChange,SIGNAL(linkActivated(const QString &)),SLOT(onChangeLinkActivated(const QString &)));
	connect(ui.cbtDelete,SIGNAL(clicked(bool)),SLOT(onDeleteButtonClicked(bool)));

	connect(FGateways->instance(),SIGNAL(loginReceived(const QString &, const QString &)),
		SLOT(onServiceLoginReceived(const QString &, const QString &)));
	connect(FGateways->instance(),SIGNAL(serviceEnableChanged(const Jid &, const Jid &, bool)),
		SLOT(onServiceEnableChanged(const Jid &, const Jid &, bool)));
	connect(FGateways->instance(),SIGNAL(servicePresenceChanged(const Jid &, const Jid &, const IPresenceItem &)),
		SLOT(onServicePresenceChanged(const Jid &, const Jid &, const IPresenceItem &)));

	onServiceEnableChanged(FStreamJid,FServiceJid,FGateways->isServiceEnabled(FStreamJid,FServiceJid));
	onServicePresenceChanged(FStreamJid,FServiceJid,FGateways->servicePresence(FStreamJid,FServiceJid));
}

LegacyAccountOptions::~LegacyAccountOptions()
{

}

void LegacyAccountOptions::onEnableButtonClicked(bool)
{
	if (FGateways->setServiceEnabled(FStreamJid,FServiceJid,true))
	{
		ui.pbtEnable->setEnabled(false);
		ui.pbtEnable->setText(tr("Enabling"));
	}
}

void LegacyAccountOptions::onDisableButtonClicked(bool)
{

	if (FGateways->setServiceEnabled(FStreamJid,FServiceJid,false))
	{
		ui.pbtDisable->setEnabled(false);
		ui.pbtDisable->setText(tr("Disabling"));
	}
}

void LegacyAccountOptions::onChangeLinkActivated(const QString &ALink)
{
	Q_UNUSED(ALink);
	QDialog *dialog = FGateways->showAddLegacyAccountDialog(FStreamJid,FServiceJid);
	if (dialog)
	{
		connect(dialog,SIGNAL(accepted()),SLOT(onChangeDialogAccepted()));
	}
}

void LegacyAccountOptions::onChangeDialogAccepted()
{
	FLoginRequest = FGateways->sendLoginRequest(FStreamJid,FServiceJid);
}

void LegacyAccountOptions::onDeleteButtonClicked(bool)
{
	if (QMessageBox::question(this,tr("Account Deletion"),tr("Are you sure you want to delete <b>%1</b> account?").arg(ui.lblLogin->text()),
		QMessageBox::Yes|QMessageBox::No,QMessageBox::No) == QMessageBox::Yes)
	{
		setEnabled(false);
		FGateways->removeService(FStreamJid,FServiceJid);
	}
}

void LegacyAccountOptions::onServiceLoginReceived(const QString &AId, const QString &ALogin)
{
	if (AId == FLoginRequest)
	{
		if (!ALogin.isEmpty())
			ui.lblLogin->setText(ALogin);
		else
			ui.lblLogin->setText(FGateLabel.isValid ? FGateLabel.name : FServiceJid.full());
	}
}

void LegacyAccountOptions::onServiceEnableChanged(const Jid &AStreamJid, const Jid &AServiceJid, bool AEnabled)
{
	if (AStreamJid==FStreamJid && AServiceJid==FServiceJid)
	{
		if (AEnabled)
		{
			ui.pbtEnable->setEnabled(false);
			ui.pbtEnable->setText(tr("Enabled"));
			ui.pbtDisable->setEnabled(true);
			ui.pbtDisable->setText(tr("Disable"));
		}
		else
		{
			ui.pbtEnable->setEnabled(true);
			ui.pbtEnable->setText(tr("Enable"));
			ui.pbtDisable->setEnabled(false);
			ui.pbtDisable->setText(tr("Disabled"));
		}
	}
}

void LegacyAccountOptions::onServicePresenceChanged(const Jid &AStreamJid, const Jid &AServiceJid, const IPresenceItem &AItem)
{
	if (AStreamJid==FStreamJid && AServiceJid==FServiceJid)
	{
		if (AItem.show == IPresence::Error)
		{
			ui.lblError->setText(AItem.status);
			ui.lblError->setVisible(true);
		}
		else
		{
			ui.lblError->setVisible(false);
		}
	}
}
