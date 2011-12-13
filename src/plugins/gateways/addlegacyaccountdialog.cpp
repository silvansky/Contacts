#include "addlegacyaccountdialog.h"

#include <QTimer>
#include <QListView>
#include <QPushButton>
#include <utils/graphicseffectsstorage.h>
#include <definitions/graphicseffects.h>
#include <definitions/resources.h>

AddLegacyAccountDialog::AddLegacyAccountDialog(IGateways *AGateways, IRegistration *ARegistration, IPresence *APresence, const Jid &AServiceJid, QWidget *AParent)	: QDialog(AParent)
{
	ui.setupUi(this);
	ui.cmbDomains->setView(new QListView);
	setAttribute(Qt::WA_DeleteOnClose,true);
	setWindowModality(AParent ? Qt::WindowModal : Qt::NonModal);

#ifdef Q_WS_MAC
	ui.buttonsLayout->addWidget(ui.pbtOk);
	ui.buttonsLayout->setSpacing(16);
	ui.loginLayout->setSpacing(6);
#endif

	FPresence = APresence;
	FGateways = AGateways;
	FRegistration = ARegistration;

	FServiceJid = AServiceJid;
	FAbortMessage = tr("The service is temporarily unavailable, please try to connect later.");

	ui.lneLogin->setAttribute(Qt::WA_MacShowFocusRect, false);
	ui.lnePassword->setAttribute(Qt::WA_MacShowFocusRect, false);

	ui.lblError->setVisible(false);
	ui.chbShowPassword->setVisible(false);
	IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->insertAutoIcon(ui.lblErrorIcon,MNI_GATEWAYS_ADD_ACCOUNT_ERROR,0,0,"pixmap");
	connect(ui.btbButtons,SIGNAL(clicked(QAbstractButton *)),SLOT(onDialogButtonClicked(QAbstractButton *)));
	connect(ui.pbtOk, SIGNAL(clicked()), SLOT(onOkClicked()));
	connect(ui.pbtCancel, SIGNAL(clicked()), SLOT(onCancelClicked()));
	ui.btbButtons->setVisible(false);

	connect(ui.lneLogin,SIGNAL(textChanged(const QString &)),SLOT(onLineEditTextChanged(const QString &)));
	connect(ui.lnePassword,SIGNAL(textChanged(const QString &)),SLOT(onLineEditTextChanged(const QString &)));
	connect(ui.chbShowPassword,SIGNAL(stateChanged(int)),SLOT(onShowPasswordStateChanged(int)));
	onLineEditTextChanged(QString::null);

	connect(FRegistration->instance(),SIGNAL(registerFields(const QString &, const IRegisterFields &)),
		SLOT(onRegisterFields(const QString &, const IRegisterFields &)));
	connect(FRegistration->instance(),SIGNAL(registerSuccess(const QString &)),
		SLOT(onRegisterSuccess(const QString &)));
	connect(FRegistration->instance(),SIGNAL(registerError(const QString &, const QString &, const QString &)),
		SLOT(onRegisterError(const QString &, const QString &, const QString &)));

	domainsMenu = new Menu(this);
	domainsMenu->setObjectName("domainsMenu");
	ui.tlbDomains->setMenu(domainsMenu);

	FGateLabel = FGateways->serviceDescriptor(FPresence->streamJid(), FServiceJid);
	if (!FGateLabel.id.isEmpty())
	{
		setWindowTitle(tr("Add %1 account").arg(FGateLabel.name));

		ui.lblCaption->setText(FGateLabel.name);
		ui.lneLogin->setPlaceholderText(!FGateLabel.loginLabel.isEmpty() ? FGateLabel.loginLabel : tr("Login"));
		ui.lnePassword->setPlaceholderText(tr("Password"));

		int i = 0;
		foreach(QString domain, FGateLabel.domains)
		{
			ui.cmbDomains->addItem("@"+domain,domain);
			Action * action = new Action(domainsMenu);
			action->setText("@"+domain);
			action->setProperty("domain", domain);
			domainsMenu->addAction(action);
			connect(action, SIGNAL(triggered()), SLOT(onDomainsMenuActionTriggered()));
			if (!i++)
				action->trigger();
		}
		ui.cmbDomains->setVisible(!FGateLabel.domains.isEmpty());
		ui.tlbDomains->setVisible(!FGateLabel.domains.isEmpty());

		LogDetaile(QString("[AddLegacyAccountDialog][%1] Sending registration fields request").arg(FServiceJid.full()));
		FRegisterId = FRegistration->sendRegiterRequest(FPresence->streamJid(),FServiceJid);
		if (FRegisterId.isEmpty())
			abort(FAbortMessage);
		else
			setWaitMode(true, tr("Waiting for host response..."));
	}
	else
	{
		LogError(QString("[AddLegacyAccountDialog][%1] Failed to find service descriptor").arg(FServiceJid.full()));
		abort(FAbortMessage);
	}

	//GraphicsEffectsStorage::staticStorage(RSR_STORAGE_GRAPHICSEFFECTS)->installGraphicsEffect(this, GFX_LABELS);

	//ui.tlbDomains->setVisible(false);
	ui.cmbDomains->setVisible(false);
}

AddLegacyAccountDialog::~AddLegacyAccountDialog()
{
	domainsMenu->deleteLater();
}

void AddLegacyAccountDialog::showEvent(QShowEvent *AEvent)
{
	QDialog::showEvent(AEvent);
	QTimer::singleShot(0,this,SLOT(onAdjustDialogSize()));
}

void AddLegacyAccountDialog::abort(const QString &AMessage)
{
	Q_UNUSED(AMessage);
	CustomInputDialog *dialog = new CustomInputDialog(CustomInputDialog::Info);
	dialog->setCaptionText(tr("Error"));
	dialog->setInfoText(AMessage);
	dialog->setAcceptButtonText(tr("Ok"));
	dialog->setDeleteOnClose(true);
	dialog->show();
	QTimer::singleShot(0,this,SLOT(reject()));
	hide();
}

void AddLegacyAccountDialog::setError(const QString &AMessage)
{
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
		ui.tlbDomains->setProperty("error", !AMessage.isEmpty());
		StyleStorage::updateStyle(this);

		QTimer::singleShot(0,this,SLOT(onAdjustDialogSize()));
	}
}

void AddLegacyAccountDialog::setWaitMode(bool AWait, const QString &AMessage)
{
	if (AWait)
	{
		ui.lblInfo->setText(AMessage);
		ui.btbButtons->button(QDialogButtonBox::Ok)->setEnabled(false);
		ui.pbtOk->setEnabled(false);
		ui.chbShowPassword->setVisible(false);
		setError(QString::null);
	}
	else
	{
		QString info = tr("Please, enter your login and password.");
		if (!FGateways->streamServices(FPresence->streamJid()).contains(FServiceJid))
			info = tr("Your account is not connected.") + " " + info;
		ui.lblInfo->setText(info);
		onLineEditTextChanged(QString::null);
	}
	ui.lneLogin->setEnabled(!AWait);
	ui.cmbDomains->setEnabled(!AWait);
	ui.tlbDomains->setEnabled(!AWait);
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
	//ui.btbButtons->button(QDialogButtonBox::Ok)->setEnabled(!ui.lneLogin->text().isEmpty() && !ui.lnePassword->text().isEmpty());
	ui.pbtOk->setEnabled(!ui.lneLogin->text().isEmpty() && !ui.lnePassword->text().isEmpty());
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
		onOkClicked();
	else
		onCancelClicked();
}

void AddLegacyAccountDialog::onOkClicked()
{
	FGateLogin.login = ui.lneLogin->text();
	FGateLogin.password = ui.lnePassword->text();
	if (!FGateLabel.domains.isEmpty())
	{
		//FGateLogin.domain = ui.cmbDomains->itemData(ui.cmbDomains->currentIndex()).toString();
		FGateLogin.domain = ui.tlbDomains->property("domain").toString();
	}
	else if (!FGateLogin.domainSeparator.isEmpty())
	{
		QStringList parts = FGateLogin.login.split(FGateLogin.domainSeparator);
		FGateLogin.login = parts.value(0);
		FGateLogin.domain = parts.value(1);
	}

	IRegisterSubmit submit = FGateways->serviceSubmit(FPresence->streamJid(),FServiceJid,FGateLogin);
	if (submit.serviceJid.isValid())
	{
		FGateways->sendLogPresence(FPresence->streamJid(),FServiceJid,false);
		LogDetaile(QString("[AddLegacyAccountDialog][%1] Sending registration submit").arg(FServiceJid.full()));
		FRegisterId = FRegistration->sendSubmit(FPresence->streamJid(),submit);
		if (FRegisterId.isEmpty())
			abort(FAbortMessage);
		else
			setWaitMode(true, tr("Waiting for host response..."));
	}
	else
	{
		LogError(QString("[AddLegacyAccountDialog][%1] Failed to generate registration submit").arg(FServiceJid.full()));
		setError(tr("Invalid registration params"));
	}
}

void AddLegacyAccountDialog::onCancelClicked()
{
	LogDetaile(QString("[AddLegacyAccountDialog][%1] Registration canceled by user").arg(FServiceJid.full()));
	reject();
}

void AddLegacyAccountDialog::onDomainsMenuActionTriggered()
{
	Action * action = qobject_cast<Action*>(sender());
	if (action)
	{
		ui.tlbDomains->setText(action->text());
		ui.tlbDomains->setProperty("domain", action->property("domain"));
	}
}

void AddLegacyAccountDialog::onRegisterFields(const QString &AId, const IRegisterFields &AFields)
{
	if (AId == FRegisterId)
	{
		FGateLogin = FGateways->serviceLogin(FPresence->streamJid(),FServiceJid,AFields);
		if (FGateLogin.isValid)
		{
			LogDetaile(QString("[AddLegacyAccountDialog][%1] Received registration fields, id='%2'").arg(AFields.serviceJid.full(),AId));
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
				{
					//ui.cmbDomains->setCurrentIndex(ui.cmbDomains->findData(FGateLogin.domain,Qt::UserRole,Qt::MatchExactly));
					ui.tlbDomains->setText("@"+FGateLogin.domain);
					ui.tlbDomains->setProperty("domain", FGateLogin.domain);
				}
				ui.lneLogin->setText(FGateLogin.login);
			}
			ui.lnePassword->setText(FGateLogin.password);

			if (FGateLogin.login.isEmpty())
			{
				ui.btbButtons->button(QDialogButtonBox::Ok)->setText(tr("Append"));
				ui.pbtOk->setText(tr("Append"));
			}
			else
			{
				ui.btbButtons->button(QDialogButtonBox::Ok)->setText(tr("Change"));
				ui.pbtOk->setText(tr("Change"));
			}
		}
		else
		{
			LogError(QString("[AddLegacyAccountDialog][%1] Unsupported registration fields received, id='%2'").arg(FServiceJid.full(),AId));
			abort(FAbortMessage);
		}
		setWaitMode(false);
	}
}

void AddLegacyAccountDialog::onRegisterSuccess(const QString &AId)
{
	if (AId == FRegisterId)
	{
		LogDetaile(QString("[AddLegacyAccountDialog][%1] Registration finished successfully, id='%2'").arg(FServiceJid.full(),AId));
		accept();
	}
}

void AddLegacyAccountDialog::onRegisterError(const QString &AId, const QString &ACondition, const QString &AMessage)
{
	if (AId == FRegisterId)
	{
		LogError(QString("[AddLegacyAccountDialog][%1] Registration error, id='%2': %3").arg(FServiceJid.full(),AId,AMessage));
		if (ACondition=="not-authorized" || ACondition=="not-acceptable")
		{
			setError(tr("Failed to add account, check your login and password"));
			setWaitMode(false);
		}
		else if (ACondition == "resource-limit-exceeded")
		{
			abort(tr("You have connected the maximum number of %1 accounts.").arg(FGateLabel.name));
		}
		else
		{
			abort(FAbortMessage);
		}
	}
}
