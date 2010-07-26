#include "addlegacyaccountdialog.h"

#include <QTimer>
#include <QPushButton>
#include <QMessageBox>

AddLegacyAccountDialog::AddLegacyAccountDialog(IGateways *AGateways, IRegistration *ARegistration, const Jid &AStreamJid, const Jid &AServiceJid, QWidget *AParent)	: QDialog(AParent)
{
	ui.setupUi(this);
	setAttribute(Qt::WA_DeleteOnClose,true);
	setWindowModality(AParent!=NULL ? Qt::WindowModal : Qt::NonModal);

	FGateways = AGateways;
	FRegistration = ARegistration;

	FStreamJid = AStreamJid;
	FServiceJid = AServiceJid;

	initialize();

	ui.lblError->setVisible(false);
	ui.chbShowPassword->setVisible(false);
	ui.btbButtons->button(QDialogButtonBox::Ok)->setText(tr("Append"));
	connect(ui.btbButtons,SIGNAL(clicked(QAbstractButton *)),SLOT(onDialogButtonClicked(QAbstractButton *)));

	connect(ui.lneLogin,SIGNAL(textChanged(const QString &)),SLOT(onLineEditTextChanged(const QString &)));
	connect(ui.lnePassword,SIGNAL(textChanged(const QString &)),SLOT(onLineEditTextChanged(const QString &)));
	connect(ui.chbShowPassword,SIGNAL(stateChanged(int)),SLOT(onShowPasswordStateChanged(int)));
	onLineEditTextChanged(QString::null);

	FGateLabel = FGateways->serviceLabel(FStreamJid, FServiceJid);
	if (FGateLabel.valid)
	{
		setWindowTitle(tr("Account: %1").arg(FGateLabel.name));
		IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->insertAutoIcon(ui.lblLogin,FGateLabel.iconKey,0,0,"pixmap");
		ui.lblLogin->setText(!FGateLabel.loginLabel.isEmpty() ? FGateLabel.loginLabel : ui.lblLogin->text());
		ui.cmbDomains->addItems(FGateLabel.domains);
		ui.cmbDomains->setVisible(!FGateLabel.domains.isEmpty());

		FRegisterId = FRegistration->sendRegiterRequest(FStreamJid,FServiceJid);
		if (FRegisterId.isEmpty())
			abort(tr("Gateway registration request failed"));
		else
			setWaitMode(true, tr("Waiting for host response..."));
	}
	else
	{
		abort(tr("Unsupported gateway type"));
	}
}

AddLegacyAccountDialog::~AddLegacyAccountDialog()
{

}

void AddLegacyAccountDialog::showEvent(QShowEvent *AEvent)
{
	onAdjustDialogSize();
	QDialog::showEvent(AEvent);
}

void AddLegacyAccountDialog::initialize()
{
	connect(FRegistration->instance(),SIGNAL(registerFields(const QString &, const IRegisterFields &)),
		SLOT(onRegisterFields(const QString &, const IRegisterFields &)));
	connect(FRegistration->instance(),SIGNAL(registerSuccess(const QString &)),
		SLOT(onRegisterSuccess(const QString &)));
	connect(FRegistration->instance(),SIGNAL(registerError(const QString &, const QString &)),
		SLOT(onRegisterError(const QString &, const QString &)));
}

void AddLegacyAccountDialog::abort(const QString &AMessage)
{
	hide();
	QMessageBox::critical(this,tr("Error connecting account"),tr("Failed to connect account due to error:\n%1").arg(AMessage));
	QTimer::singleShot(0,this,SLOT(reject()));
}

void AddLegacyAccountDialog::setError(const QString &AMessage)
{
	ui.lblError->setText(AMessage);
	ui.lblError->setVisible(!AMessage.isEmpty());
	ui.chbShowPassword->setVisible(!AMessage.isEmpty());
	QTimer::singleShot(0,this,SLOT(onAdjustDialogSize()));
}

void AddLegacyAccountDialog::setWaitMode(bool AWait, const QString &AMessage)
{
	if (AWait)
	{
		ui.lblCaption->setText(AMessage);
		ui.btbButtons->button(QDialogButtonBox::Ok)->setEnabled(false);
		ui.chbShowPassword->setVisible(false);
		setError(QString::null);
	}
	else
	{
		ui.lblCaption->setText(tr("Enter the username and password to your %1 account").arg(FGateLabel.name));
		onLineEditTextChanged(QString::null);
	}
	ui.lneLogin->setEnabled(!AWait);
	ui.cmbDomains->setEnabled(!AWait);
	ui.lnePassword->setEnabled(!AWait);
	ui.chbShowPassword->setEnabled(!AWait);
}

void AddLegacyAccountDialog::onAdjustDialogSize()
{
	resize(width(),minimumSizeHint().height());
}

void AddLegacyAccountDialog::onLineEditTextChanged(const QString &AText)
{
	Q_UNUSED(AText);
	ui.btbButtons->button(QDialogButtonBox::Ok)->setEnabled(!ui.lneLogin->text().isEmpty() && !ui.lnePassword->text().isEmpty());
}

void AddLegacyAccountDialog::onShowPasswordStateChanged(int AState)
{
	if (AState == Qt::Checked)
	{
		ui.lnePassword->clear();
		ui.lnePassword->setEchoMode(QLineEdit::Normal);
	}
	else
		ui.lnePassword->setEchoMode(QLineEdit::Password);
}

void AddLegacyAccountDialog::onDialogButtonClicked(QAbstractButton *AButton)
{
	if (ui.btbButtons->standardButton(AButton) == QDialogButtonBox::Ok)
	{
		FGateLogin.login = ui.lneLogin->text();
		if (ui.cmbDomains->isVisible())
			FGateLogin.domain = ui.cmbDomains->currentText();
		FGateLogin.password = ui.lnePassword->text();

		IRegisterSubmit submit = FGateways->serviceSubmit(FStreamJid,FServiceJid,FGateLogin);
		if (submit.serviceJid.isValid())
		{
			FGateways->sendLogPresence(FStreamJid,FServiceJid,false);
			FRegisterId = FRegistration->sendSubmit(FStreamJid,submit);
			if (FRegisterId.isEmpty())
				setError(tr("Gateway registration request failed"));
			else
				setWaitMode(true, tr("Waiting for host response..."));
		}
		else
		{
			setError(tr("Invalid registration params"));
		}
	}
	else
	{
		reject();
	}
}

void AddLegacyAccountDialog::onRegisterFields(const QString &AId, const IRegisterFields &AFields)
{
	if (AId == FRegisterId)
	{
		FGateLogin = FGateways->serviceLogin(FStreamJid,FServiceJid,AFields);
		if (FGateLogin.valid)
		{
			ui.lneLogin->setText(FGateLogin.login);
			ui.lnePassword->setText(FGateLogin.password);
			if (!FGateLogin.domain.isEmpty())
				ui.cmbDomains->setCurrentIndex(ui.cmbDomains->findText(FGateLogin.domain));
		}
		else
		{
			abort(tr("Unsupported gateway registration form"));
		}
		setWaitMode(false);
	}
}

void AddLegacyAccountDialog::onRegisterSuccess(const QString &AId)
{
	if (AId == FRegisterId)
	{
		if (FGateways->setServiceEnabled(FStreamJid,FServiceJid,true))
			accept();
		else
			setError(tr("Connection to gateway is lost"));
	}
}

void AddLegacyAccountDialog::onRegisterError(const QString &AId, const QString &AError)
{
	if (AId == FRegisterId)
	{
		if (FGateLogin.valid)
		{
			setError(AError);
			setWaitMode(false);
		}
		else
		{
			abort(tr("Gateway registration request failed"));
		}
	}
}
