#include "addfacebookaccountdialog.h"

#include <QWebFrame>
#include <QTextDocument>
#include <QNetworkRequest>
#include <QDesktopServices>

#define AUTH_HOST "fb.tx.contacts.rambler.ru"

AddFacebookAccountDialog::AddFacebookAccountDialog(IGateways *AGateways, IRegistration *ARegistration, IPresence *APresence, const Jid &AServiceJid, QWidget *AParent) : QDialog(AParent)
{
	ui.setupUi(this);
	setWindowModality(AParent ? Qt::WindowModal : Qt::NonModal);
	StyleStorage::staticStorage(RSR_STORAGE_STYLESHEETS)->insertAutoStyle(this,STS_GATEWAYS_ADDFACEBOOKACCOUNTDIALOG);

	CustomBorderContainer *border = CustomBorderStorage::staticStorage(RSR_STORAGE_CUSTOMBORDER)->addBorder(this, CBS_DIALOG);
	if (border)
	{
		border->setAttribute(Qt::WA_DeleteOnClose, true);
		border->setMaximizeButtonVisible(false);
		border->setMinimizeButtonVisible(false);
		connect(border, SIGNAL(closeClicked()), SLOT(reject()));
		connect(this, SIGNAL(rejected()), border, SLOT(close()));
		connect(this, SIGNAL(accepted()), border, SLOT(close()));
		border->setResizable(false);
	}
	else
	{
		setAttribute(Qt::WA_DeleteOnClose,true);
		ui.lblCaption->setVisible(false);
		layout()->setContentsMargins(0, 0, 0, 0);
	}

	FPresence = APresence;
	FGateways = AGateways;
	FRegistration = ARegistration;

	FServiceJid = AServiceJid;
	FAbortMessage = tr("The service is temporarily unavailable, please try to connect later.");

	setMaximumSize(500,500);

	if (FPresence->xmppStream() && FPresence->xmppStream()->connection())
	{
		IDefaultConnection *defConnection = qobject_cast<IDefaultConnection *>(FPresence->xmppStream()->connection()->instance());
		if (defConnection)
			ui.wbvView->page()->networkAccessManager()->setProxy(defConnection->proxy());
	}

	ui.wbvView->page()->setLinkDelegationPolicy(QWebPage::DelegateAllLinks);
	ui.wbvView->page()->mainFrame()->setScrollBarPolicy(Qt::Vertical,Qt::ScrollBarAlwaysOff);
	connect(ui.wbvView,SIGNAL(loadStarted()),SLOT(onWebViewLoadStarted()));
	connect(ui.wbvView,SIGNAL(loadFinished(bool)),SLOT(onWebViewLoadFinished(bool)));
	connect(ui.wbvView->page(),SIGNAL(linkClicked(const QUrl &)),SLOT(onWebPageLinkClicked(const QUrl &)));

	connect(FRegistration->instance(),SIGNAL(registerFields(const QString &, const IRegisterFields &)),
		SLOT(onRegisterFields(const QString &, const IRegisterFields &)));
	connect(FRegistration->instance(),SIGNAL(registerSuccess(const QString &)),
		SLOT(onRegisterSuccess(const QString &)));
	connect(FRegistration->instance(),SIGNAL(registerError(const QString &, const QString &, const QString &)),
		SLOT(onRegisterError(const QString &, const QString &, const QString &)));

	LogDetail(QString("[AddLegacyAccountDialog][%1] Sending registration fields request").arg(FServiceJid.full()));
	FRegisterId = FRegistration->sendRegiterRequest(FPresence->streamJid(),FServiceJid);
	if (FRegisterId.isEmpty())
		abort(FAbortMessage);
	else
		setWaitMode(true, tr("Waiting for host response..."));
}

AddFacebookAccountDialog::~AddFacebookAccountDialog()
{

}

void AddFacebookAccountDialog::checkResult()
{
	setWaitMode(false);
	QUrl result = ui.wbvView->url();
	if (result.host() == AUTH_HOST)
	{
		if (result.hasQueryItem("access_token") && result.hasQueryItem("username"))
		{
			ui.wbvView->setHtml(Qt::escape(tr("Facebook has confirmed your authorization")));

			FGateLogin.login = result.queryItemValue("username");
			FGateLogin.domain = "chat.facebook.com";
			FGateLogin.password = result.queryItemValue("access_token").split("|").value(1);
			IRegisterSubmit submit = FGateways->serviceSubmit(FPresence->streamJid(),FServiceJid,FGateLogin);
			if (submit.serviceJid.isValid())
			{
				FGateways->sendLogPresence(FPresence->streamJid(),FServiceJid,false);
				LogDetail(QString("[AddFacebookAccountDialog][%1] Sending registration submit").arg(FServiceJid.full()));
				FRegisterId = FRegistration->sendSubmit(FPresence->streamJid(),submit);
				if (FRegisterId.isEmpty())
					abort(FAbortMessage);
				else
					setWaitMode(true, tr("Waiting for host response..."));
			}
			else
			{
				LogError(QString("[AddFacebookAccountDialog][%1] Failed to generate registration submit").arg(FServiceJid.full()));
				abort(FAbortMessage);
			}
		}
		else if (result.hasQueryItem("error"))
		{
			if (result.queryItemValue("error_reason") != "user_denied")
			{
				LogError(QString("[AddFacebookAccountDialog][%1] Registration failed: %2").arg(FServiceJid.full(),result.queryItemValue("error_description")));
				abort(FAbortMessage/*result.queryItemValue("error_description").replace('+',' ')*/);
			}
			else
			{
				LogDetail(QString("[AddFacebookAccountDialog][%1] Registration canceled by user").arg(FServiceJid.full()));
				reject();
			}
		}
	}
}

void AddFacebookAccountDialog::abort(const QString &AMessage)
{
	CustomInputDialog *dialog = new CustomInputDialog(CustomInputDialog::Info);
	dialog->setCaptionText(tr("Error"));
	dialog->setInfoText(AMessage);
	dialog->setAcceptButtonText(tr("Ok"));
	dialog->setDeleteOnClose(true);
	dialog->show();
	QTimer::singleShot(0,this,SLOT(reject()));
	hide();
}

void AddFacebookAccountDialog::setWaitMode(bool AWait, const QString &AMessage)
{
	ui.lblCaption->setText(tr("Facebook authorization"));
	if (AWait && !AMessage.isEmpty())
		ui.lblCaption->setText(ui.lblCaption->text()+" - "+AMessage);

	if (parentWidget())
		parentWidget()->setWindowTitle(ui.lblCaption->text());
	else
		setWindowTitle(ui.lblCaption->text());

	ui.wbvView->setEnabled(!AWait);
}

void AddFacebookAccountDialog::onRegisterFields(const QString &AId, const IRegisterFields &AFields)
{
	if (AId == FRegisterId)
	{
		FGateLogin = FGateways->serviceLogin(FPresence->streamJid(),FServiceJid,AFields);
		if (FGateLogin.isValid)
		{
			LogDetail(QString("[AddFacebookAccountDialog][%1] Loading registration web page").arg(FServiceJid.full()));
			QNetworkRequest request(QUrl("http://"AUTH_HOST"/auth"));
			request.setRawHeader("Accept-Encoding","identity");
			ui.wbvView->load(request);
		}
		else
		{
			LogError(QString("[AddFacebookAccountDialog][%1] Unsupported registration fields received, id='%2'").arg(FServiceJid.full(),AId));
			abort(FAbortMessage);
		}
	}
}

void AddFacebookAccountDialog::onRegisterSuccess(const QString &AId)
{
	if (AId == FRegisterId)
	{
		LogDetail(QString("[AddFacebookAccountDialog][%1] Registration finished successfully, id='%2'").arg(FServiceJid.full(),AId));
		accept();
	}
}

void AddFacebookAccountDialog::onRegisterError(const QString &AId, const QString &ACondition, const QString &AMessage)
{
	if (AId == FRegisterId)
	{
		LogError(QString("[AddFacebookAccountDialog][%1] Registration error, id='%2': %3").arg(FServiceJid.full(),AId,AMessage));
		if (ACondition == "resource-limit-exceeded")
			abort(tr("You have connected the maximum number of %1 accounts.").arg(tr("Facebook")));
		else
			abort(FAbortMessage);
	}
}

void AddFacebookAccountDialog::onWebViewLoadStarted()
{
	setWaitMode(true, tr("Loading..."));
}

void AddFacebookAccountDialog::onWebViewLoadFinished(bool AOk)
{
	if (!AOk)
	{
		LogError(QString("[AddFacebookAccountDialog][%1] Failed to load web page").arg(FServiceJid.full()));
		abort(FAbortMessage);
	}
	else
	{
		checkResult();
	}
}

void AddFacebookAccountDialog::onWebPageLinkClicked(const QUrl &ALink)
{
	QDesktopServices::openUrl(ALink);
}
