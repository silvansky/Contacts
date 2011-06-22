#include "addfacebookaccountdialog.h"

#include <QDebug>
#include <QWebFrame>
#include <QTextDocument>
#include <QDesktopServices>

#define AUTH_HOST "fb.tx.contacts.rambler.ru"

AddFacebookAccountDialog::AddFacebookAccountDialog(IGateways *AGateways, IRegistration *ARegistration, IPresence *APresence, const Jid &AServiceJid, QWidget *AParent) : QDialog(AParent)
{
	ui.setupUi(this);
	setAttribute(Qt::WA_DeleteOnClose,true);
	setWindowModality(AParent ? Qt::WindowModal : Qt::NonModal);
	StyleStorage::staticStorage(RSR_STORAGE_STYLESHEETS)->insertAutoStyle(this,STS_GATEWAYS_ADDFACEBOOKACCOUNTDIALOG);

	FPresence = APresence;
	FGateways = AGateways;
	FRegistration = ARegistration;

	FServiceJid = AServiceJid;

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
	connect(FRegistration->instance(),SIGNAL(registerError(const QString &, const QString &)),
		SLOT(onRegisterError(const QString &, const QString &)));

   FAbortMessage = tr("The service is temporarily unavailable, please try to connect later.");

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
				FRegisterId = FRegistration->sendSubmit(FPresence->streamJid(),submit);
				if (!FRegisterId.isEmpty())
					setWaitMode(true, tr("Waiting for host response..."));
				else
					abort(FAbortMessage);
			}
			else
			{
				abort(FAbortMessage);
			}
		}
		else if (result.hasQueryItem("error"))
		{
			if (result.queryItemValue("error_reason") != "user_denied")
				abort(FAbortMessage/*result.queryItemValue("error_description").replace('+',' ')*/);
			else
				reject();
		}
	}
}

void AddFacebookAccountDialog::abort(const QString &AMessage)
{
	CustomInputDialog * dialog = new CustomInputDialog(CustomInputDialog::Info);
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
			QUrl request;
			request.setScheme("http");
			request.setHost(AUTH_HOST);
			request.setPath("auth");
			ui.wbvView->load(request);
		}
		else
		{
			abort(FAbortMessage);
		}
	}
}

void AddFacebookAccountDialog::onRegisterSuccess(const QString &AId)
{
	if (AId == FRegisterId)
	{
		if (FGateways->setServiceEnabled(FPresence->streamJid(),FServiceJid,true))
			accept();
		else
			abort(FAbortMessage);
	}
}

void AddFacebookAccountDialog::onRegisterError(const QString &AId, const QString &AError)
{
	if (AId == FRegisterId)
	{
      Log(QString("[Add legacy account register error] %1").arg(AError));
		if (FGateLogin.isValid)
			abort(FAbortMessage);
      else if (AError == "resource-limit-exceeded") // јцкий хак, нужно везде передавать код ошибки!
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
	if (AOk)
		checkResult();
	else
		abort(FAbortMessage);
}

void AddFacebookAccountDialog::onWebPageLinkClicked(const QUrl &ALink)
{
	QDesktopServices::openUrl(ALink);
}
