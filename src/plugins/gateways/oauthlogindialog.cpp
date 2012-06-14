#include "oauthlogindialog.h"

#include <QPair>
#include <QWebFrame>
#include <QNetworkRequest>
#include <QDesktopServices>
#include <definitions/resources.h>
#include <definitions/stylesheets.h>
#include <definitions/customborder.h>
#include <utils/log.h>
#include <utils/stylestorage.h>
#include <utils/customborderstorage.h>

OAuthLoginDialog::OAuthLoginDialog(IPresence *APresence, const QUrl &AAuthUrl, const IGateServiceLabel &AGateLabel, const Jid &AServiceJid, QWidget *AParent) : QDialog(AParent)
{
	ui.setupUi(this);
	setMinimumSize(600,600);
	StyleStorage::staticStorage(RSR_STORAGE_STYLESHEETS)->insertAutoStyle(this,STS_GATEWAYS_OAUTHLOGINDIALOG);

	FGateLabel = AGateLabel;
	FServiceJid = AServiceJid;

	CustomBorderContainer *border = CustomBorderStorage::staticStorage(RSR_STORAGE_CUSTOMBORDER)->addBorder(this, CBS_DIALOG);
	if (border)
	{
		//border->setResizable(false);
		border->setAttribute(Qt::WA_DeleteOnClose, true);
		border->setMaximizeButtonVisible(false);
		border->setMinimizeButtonVisible(false);
		connect(border, SIGNAL(closeClicked()), SLOT(reject()));
		connect(this, SIGNAL(rejected()), border, SLOT(close()));
		connect(this, SIGNAL(accepted()), border, SLOT(close()));
	}
	else
	{
		ui.lblCaption->setVisible(false);
		setAttribute(Qt::WA_DeleteOnClose,true);
		layout()->setContentsMargins(0, 0, 0, 0);
	}
	window()->setWindowModality(AParent ? Qt::WindowModal : Qt::NonModal);

	if (APresence->xmppStream() && APresence->xmppStream()->connection())
	{
		IDefaultConnection *defConnection = qobject_cast<IDefaultConnection *>(APresence->xmppStream()->connection()->instance());
		if (defConnection)
			ui.wbvView->page()->networkAccessManager()->setProxy(defConnection->proxy());
	}

	ui.wbvView->page()->setLinkDelegationPolicy(QWebPage::DelegateAllLinks);
	ui.wbvView->page()->mainFrame()->setScrollBarPolicy(Qt::Vertical,Qt::ScrollBarAlwaysOff);
	connect(ui.wbvView,SIGNAL(loadStarted()),SLOT(onWebViewLoadStarted()));
	connect(ui.wbvView,SIGNAL(loadFinished(bool)),SLOT(onWebViewLoadFinished(bool)));
	connect(ui.wbvView->page(),SIGNAL(linkClicked(const QUrl &)),SLOT(onWebPageLinkClicked(const QUrl &)));

	LogDetail(QString("[OAuthLoginDialog][%1] Loading authorization web page").arg(FServiceJid.full()));
	QNetworkRequest request(AAuthUrl);
	request.setRawHeader("Accept-Encoding","identity");
	ui.wbvView->load(request);
}

OAuthLoginDialog::~OAuthLoginDialog()
{

}

QString OAuthLoginDialog::errorString() const
{
	return FErrorString;
}

QMap<QString, QString> OAuthLoginDialog::urlItems() const
{
	return FUrlItems;
}

void OAuthLoginDialog::checkResult()
{
	setWaitMode(false);
	QUrl result = ui.wbvView->url();
	if (result.hasQueryItem("oauth_success"))
	{
		LogDetail(QString("[OAuthLoginDialog][%1] Authorization finished successfully").arg(FServiceJid.full()));
		QList< QPair<QString,QString> > items = result.queryItems();
		for (QList< QPair<QString,QString> >::const_iterator it=items.constBegin(); it!=items.constEnd(); it++)
			FUrlItems.insert(it->first,it->second);
		accept();
	}
	else if (result.hasQueryItem("oauth_error"))
	{
		abort(result.queryItemValue("oauth_error"));
	}
}

void OAuthLoginDialog::abort(const QString &AMessage)
{
	LogError(QString("[OAuthLoginDialog][%1] Authorization failed: %2").arg(FServiceJid.full(),AMessage));
	FErrorString = AMessage;
	reject();
}

void OAuthLoginDialog::setWaitMode(bool AWait, const QString &AMessage)
{
	ui.lblCaption->setText(tr("Authorization in %1").arg(FGateLabel.name));
	if (AWait && !AMessage.isEmpty())
		ui.lblCaption->setText(ui.lblCaption->text()+" - "+AMessage);
	window()->setWindowTitle(ui.lblCaption->text());
	ui.wbvView->setEnabled(!AWait);
}

void OAuthLoginDialog::onWebViewLoadStarted()
{
	setWaitMode(true,tr("Loading..."));
}

void OAuthLoginDialog::onWebViewLoadFinished(bool AOk)
{
	if (AOk)
		checkResult();
	else
		abort(tr("Failed to load authorization web page"));
}

void OAuthLoginDialog::onWebPageLinkClicked(const QUrl &AUrl)
{
	QDesktopServices::openUrl(AUrl);
}
