#include "legacyaccountoptions.h"

#include <utils/custominputdialog.h>
#include <definitions/menuicons.h>

LegacyAccountOptions::LegacyAccountOptions(IGateways *AGateways, const Jid &AStreamJid, const Jid &AServiceJid, QWidget *AParent) : QWidget(AParent)
{
	ui.setupUi(this);
	StyleStorage::staticStorage(RSR_STORAGE_STYLESHEETS)->insertAutoStyle(this,STS_GATEWAYS_LEGACYACCOUNTOPTIONSWIDGET);

	FGateways = AGateways;
	FStreamJid = AStreamJid;
	FServiceJid = AServiceJid;

	FGateLabel = FGateways->serviceDescriptor(FStreamJid,FServiceJid);
	ui.lblLogin->setText(!FGateLabel.id.isEmpty() ? FGateLabel.name : FServiceJid.full());
	FLoginRequest = FGateways->sendLoginRequest(FStreamJid,FServiceJid);
	IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->insertAutoIcon(ui.lblIcon,FGateLabel.iconKey,0,0,"pixmap");

	if (FGateLabel.id == GSID_VKONTAKTE)
		ui.pbtChange->setEnabled(false);

	connect(ui.chbState, SIGNAL(toggled(bool)), SLOT(onStateCheckboxToggled(bool)));
	connect(ui.pbtChange, SIGNAL(clicked(bool)), SLOT(onChangeButtonClicked(bool)));
	connect(ui.cbtDelete,SIGNAL(clicked(bool)),SLOT(onDeleteButtonClicked(bool)));

	connect(FGateways->instance(),SIGNAL(loginReceived(const QString &, const QString &)),
		SLOT(onServiceLoginReceived(const QString &, const QString &)));
	connect(FGateways->instance(),SIGNAL(serviceEnableChanged(const Jid &, const Jid &, bool)),
		SLOT(onServiceEnableChanged(const Jid &, const Jid &, bool)));
	connect(FGateways->instance(),SIGNAL(servicePresenceChanged(const Jid &, const Jid &, const IPresenceItem &)),
		SLOT(onServicePresenceChanged(const Jid &, const Jid &, const IPresenceItem &)));
	connect(FGateways->instance(),SIGNAL(errorReceived(const QString &, const QString &)),
		SLOT(onGatewaysErrorReceived(const QString &, const QString &)));

	updateState(FGateways->servicePresence(FStreamJid,FServiceJid),FGateways->isServiceEnabled(FStreamJid,FServiceJid));
}

LegacyAccountOptions::~LegacyAccountOptions()
{

}

void LegacyAccountOptions::updateState(const IPresenceItem &APresenceItem, bool AEnabled)
{
	if (!AEnabled)
	{
		IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->removeAutoIcon(ui.lblInfo);
		ui.lblInfo->setText(QString::null);
		ui.lblInfo->setProperty("state",QString("disconnected"));
	}
	else if (APresenceItem.show == IPresence::Error)
	{
		IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->removeAutoIcon(ui.lblInfo);
		ui.lblInfo->setText(tr("Failed to connect"));
		ui.lblInfo->setProperty("state",QString("error"));
	}
	else if (APresenceItem.show == IPresence::Offline)
	{
		IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->insertAutoIcon(ui.lblInfo, MNI_GATEWAYS_CONNECTING_ANIMATION, 0, 100, "pixmap");
		//ui.lblInfo->setText(tr("Connecting..."));
		ui.lblInfo->setText(QString::null);
		ui.lblInfo->setProperty("state",QString("connected"));
	}
	else
	{
		IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->removeAutoIcon(ui.lblInfo);
		ui.lblInfo->setText(tr("Connected"));
		ui.lblInfo->setProperty("state",QString("connected"));
	}

	ui.chbState->blockSignals(true);
	if (AEnabled)
	{
		ui.chbState->setChecked(true);
		ui.chbState->setEnabled(true);
	}
	else
	{
		ui.chbState->setChecked(false);
		ui.chbState->setEnabled(true);
	}
	ui.chbState->blockSignals(false);

	StyleStorage::updateStyle(this);
	emit updated();
}

void LegacyAccountOptions::onStateCheckboxToggled(bool AChecked)
{
	if (FGateways->setServiceEnabled(FStreamJid,FServiceJid,AChecked))
	{
		if (AChecked)
		{
			ui.chbState->setEnabled(false);
			IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->insertAutoIcon(ui.lblInfo, MNI_GATEWAYS_CONNECTING_ANIMATION, 0, 100, "pixmap");
			//ui.lblInfo->setText(tr("Connecting..."));
			ui.lblInfo->setText(QString::null);
			ui.lblInfo->setProperty("state",QString("connected"));
		}
		else
		{
			ui.chbState->setEnabled(false);
			IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->insertAutoIcon(ui.lblInfo, MNI_GATEWAYS_CONNECTING_ANIMATION, 0, 100, "pixmap");
			//ui.lblInfo->setText(tr("Disconnecting..."));
			ui.lblInfo->setText(QString::null);
			ui.lblInfo->setProperty("state",QString("disconnected"));
		}
		StyleStorage::updateStyle(this);
		emit updated();
	}
}

void LegacyAccountOptions::onChangeButtonClicked(bool)
{
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
	CustomInputDialog *dialog = new CustomInputDialog(CustomInputDialog::None);
	dialog->setCaptionText(tr("Account Deletion"));
	dialog->setInfoText(tr("Are you sure you want to delete <b>%1</b> account?").arg(ui.lblLogin->text()));
	dialog->setAcceptButtonText(tr("Delete"));
	dialog->setRejectButtonText(tr("Cancel"));
	dialog->setAcceptIsDefault(false);
	dialog->setDeleteOnClose(true);
	connect(dialog, SIGNAL(accepted()), SLOT(onDeleteDialogAccepted()));
	dialog->show();
}

void LegacyAccountOptions::onDeleteDialogAccepted()
{
	setEnabled(false);
	FRemoveRequest = FGateways->removeService(FStreamJid,FServiceJid,false);
}

void LegacyAccountOptions::onServiceLoginReceived(const QString &AId, const QString &ALogin)
{
	if (AId == FLoginRequest)
	{
		if (!ALogin.isEmpty())
			ui.lblLogin->setText(ALogin);
		else
			ui.lblLogin->setText(!FGateLabel.id.isEmpty() ? FGateLabel.name : FServiceJid.full());
	}
}

void LegacyAccountOptions::onServiceEnableChanged(const Jid &AStreamJid, const Jid &AServiceJid, bool AEnabled)
{
	if (AStreamJid==FStreamJid && AServiceJid==FServiceJid)
		updateState(FGateways->servicePresence(AStreamJid,AServiceJid),AEnabled);
}

void LegacyAccountOptions::onServicePresenceChanged(const Jid &AStreamJid, const Jid &AServiceJid, const IPresenceItem &AItem)
{
	if (AStreamJid==FStreamJid && AServiceJid==FServiceJid)
		updateState(AItem,FGateways->isServiceEnabled(FStreamJid,FServiceJid));
}

void LegacyAccountOptions::onGatewaysErrorReceived(const QString &AId, const QString &AError)
{
	Q_UNUSED(AError);
	if (FRemoveRequest == AId)
	{
		CustomInputDialog *dialog = new CustomInputDialog(CustomInputDialog::Info);
		dialog->setCaptionText(tr("Error"));
		dialog->setInfoText(tr("The service is temporarily unavailable, please try again later."));
		dialog->setAcceptButtonText(tr("Ok"));
		dialog->setDeleteOnClose(true);
		dialog->show();
		setEnabled(true);
	}
}
