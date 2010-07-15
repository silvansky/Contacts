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

	connect(ui.lneLogin,SIGNAL(textChanged(const QString &)),SLOT(onLineEditTextChanged(const QString &)));
	connect(ui.lnePassword,SIGNAL(textChanged(const QString &)),SLOT(onLineEditTextChanged(const QString &)));
	onLineEditTextChanged(QString::null);

	IGateRegisterLabel grlabel = FGateways->registerLabel(FStreamJid, FServiceJid);
	if (grlabel.valid)
	{
		setWindowTitle(tr("Account: %1").arg(grlabel.name));
		ui.lblIcon->setPixmap(grlabel.icon.pixmap(32,32));
		ui.lblCaption->setText(tr("Enter the username and password to your %1 account").arg(grlabel.name));
		ui.lblLogin->setText(!grlabel.loginLabel.isEmpty() ? grlabel.loginLabel : ui.lblLogin->text());
		ui.cmbDomains->addItems(grlabel.domains);
		ui.cmbDomains->setVisible(!grlabel.domains.isEmpty());

		FRegisterId = FRegistration->sendRegiterRequest(FStreamJid,FServiceJid);
		if (FRegisterId.isEmpty())
			abort(tr("Gateway registration request failed"));
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

void AddLegacyAccountDialog::onAdjustDialogSize()
{
	resize(width(),minimumSizeHint().height());
}

void AddLegacyAccountDialog::onLineEditTextChanged( const QString &AText )
{
	ui.btbButtons->button(QDialogButtonBox::Ok)->setEnabled(!ui.lneLogin->text().isEmpty() && !ui.lnePassword->text().isEmpty());
}

void AddLegacyAccountDialog::onRegisterFields(const QString &AId, const IRegisterFields &AFields)
{
	if (AId == FRegisterId)
	{
		FGateLogin = FGateways->registerLogin(FStreamJid,FStreamJid,AFields);
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
	}
}

void AddLegacyAccountDialog::onRegisterSuccess(const QString &AId)
{
	if (AId == FRegisterId)
	{

	}
}

void AddLegacyAccountDialog::onRegisterError(const QString &AId, const QString &AError)
{
	if (AId == FRegisterId)
	{
		
	}
}
