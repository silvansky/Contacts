#include "addlegacyaccountdialog.h"

#include <QTimer>
#include <QMessageBox>
#include <QPushButton>
#include <utils/log.h>
#include <QListView>

AddLegacyAccountDialog::AddLegacyAccountDialog(IGateways *AGateways, IRegistration *ARegistration, const Jid &AStreamJid, const Jid &AServiceJid, QWidget *AParent)	: QDialog(AParent)
{
	ui.setupUi(this);
	ui.cmbDomains->setView(new QListView);
	setAttribute(Qt::WA_DeleteOnClose,true);
	setWindowModality(AParent ? Qt::WindowModal : Qt::NonModal);

	FGateways = AGateways;
	FRegistration = ARegistration;

	FStreamJid = AStreamJid;
	FServiceJid = AServiceJid;

	initialize();

	ui.lneLogin->setAttribute(Qt::WA_MacShowFocusRect, false);
	ui.lnePassword->setAttribute(Qt::WA_MacShowFocusRect, false);

	ui.lblError->setVisible(false);
	ui.chbShowPassword->setVisible(false);
	IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->insertAutoIcon(ui.lblErrorIcon,MNI_GATEWAYS_ADD_ACCOUNT_ERROR,0,0,"pixmap");
	connect(ui.btbButtons,SIGNAL(clicked(QAbstractButton *)),SLOT(onDialogButtonClicked(QAbstractButton *)));

	connect(ui.lneLogin,SIGNAL(textChanged(const QString &)),SLOT(onLineEditTextChanged(const QString &)));
	connect(ui.lnePassword,SIGNAL(textChanged(const QString &)),SLOT(onLineEditTextChanged(const QString &)));
	connect(ui.chbShowPassword,SIGNAL(stateChanged(int)),SLOT(onShowPasswordStateChanged(int)));
	onLineEditTextChanged(QString::null);

	FGateLabel = FGateways->serviceDescriptor(FStreamJid, FServiceJid);
	if (!FGateLabel.id.isEmpty())
	{
		setWindowTitle(tr("Add %1 account").arg(FGateLabel.name));

		ui.lblCaption->setText(FGateLabel.name);
		ui.lneLogin->setPlaceholderText(!FGateLabel.loginLabel.isEmpty() ? FGateLabel.loginLabel : tr("Login"));
		ui.lnePassword->setPlaceholderText(tr("Password"));

		foreach(QString domain, FGateLabel.domains)
			ui.cmbDomains->addItem("@"+domain,domain);
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
	QDialog::showEvent(AEvent);
	QTimer::singleShot(0,this,SLOT(onAdjustDialogSize()));
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
	QMessageBox::critical(this,tr("Error connecting account"),tr("Failed to connect account due to error:\n%1").arg(AMessage));
	QTimer::singleShot(0,this,SLOT(reject()));
	hide();
}

void AddLegacyAccountDialog::setError(const QString &AMessage)
{
	if (!AMessage.isEmpty())
		Log(QString("[Add legacy account error] %1").arg(AMessage));
	if (ui.lblError->text() != AMessage)
	{
		ui.lblError->setText(AMessage);
		ui.lblError->setVisible(!AMessage.isEmpty());
		ui.lblErrorIcon->setVisible(!AMessage.isEmpty());
		ui.chbShowPassword->setVisible(!AMessage.isEmpty());
		ui.lnePassword->setFocus();

		ui.lneLogin->setProperty("error", !AMessage.isEmpty());
		ui.lnePassword->setProperty("error", !AMessage.isEmpty());
		ui.cmbDomains->setProperty("error", !AMessage.isEmpty());
		setStyleSheet(styleSheet());

		QTimer::singleShot(0,this,SLOT(onAdjustDialogSize()));
	}
}

void AddLegacyAccountDialog::setWaitMode(bool AWait, const QString &AMessage)
{
	if (AWait)
	{
		ui.lblInfo->setText(AMessage);
		ui.btbButtons->button(QDialogButtonBox::Ok)->setEnabled(false);
		ui.chbShowPassword->setVisible(false);
		setError(QString::null);
	}
	else
	{
		QString info = tr("Please, enter your login and password.");
		if (!FGateways->streamServices(FStreamJid).contains(FServiceJid))
			info = tr("Your account is not connected.") + " " + info;
		ui.lblInfo->setText(info);
		onLineEditTextChanged(QString::null);
	}
	ui.lneLogin->setEnabled(!AWait);
	ui.cmbDomains->setEnabled(!AWait);
	ui.lnePassword->setEnabled(!AWait);
	ui.chbShowPassword->setEnabled(!AWait);

	QTimer::singleShot(0,this,SLOT(onAdjustDialogSize()));
}

void AddLegacyAccountDialog::onAdjustDialogSize()
{
	if (parentWidget())
		parentWidget()->adjustSize();
	else
		adjustSize();
	StyleStorage::staticStorage(RSR_STORAGE_STYLESHEETS)->insertAutoStyle(this,STS_GATEWAYS_ADDLEGACYACCOUNTDIALOG);
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
		FGateLogin.password = ui.lnePassword->text();
		if (!FGateLabel.domains.isEmpty())
		{
			FGateLogin.domain = ui.cmbDomains->itemData(ui.cmbDomains->currentIndex()).toString();
		}
		else if (!FGateLogin.domainSeparator.isEmpty())
		{
			QStringList parts = FGateLogin.login.split(FGateLogin.domainSeparator);
			FGateLogin.login = parts.value(0);
			FGateLogin.domain = parts.value(1);
		}

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
		if (FGateLogin.isValid)
		{
			if (FGateLabel.domains.isEmpty())
			{
				if (FGateLogin.domain.isEmpty())
					ui.lneLogin->setText(FGateLogin.login);
				else
					ui.lneLogin->setText(FGateLogin.login + FGateLogin.domainSeparator + FGateLogin.domain);
			}
			else
			{
				if (!FGateLogin.domain.isEmpty())
					ui.cmbDomains->setCurrentIndex(ui.cmbDomains->findData(FGateLogin.domain,Qt::UserRole,Qt::MatchExactly));
				ui.lneLogin->setText(FGateLogin.login);
			}
			ui.lnePassword->setText(FGateLogin.password);

			if (FGateLogin.login.isEmpty())
				ui.btbButtons->button(QDialogButtonBox::Ok)->setText(tr("Append"));
			else
				ui.btbButtons->button(QDialogButtonBox::Ok)->setText(tr("Change"));
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
	Log(QString("[Add legacy account register error] %1").arg(AError));
	if (AId == FRegisterId)
	{
		if (FGateLogin.isValid)
		{
			setError(tr("Failed to add account, check your login and password"));
			setWaitMode(false);
		}
		else
		{
			abort(tr("Gateway registration request failed"));
		}
	}
}
