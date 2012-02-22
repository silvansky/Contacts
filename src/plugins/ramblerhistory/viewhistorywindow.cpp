#include "viewhistorywindow.h"

#include <QUrl>
#include <QWebFrame>
#include <QNetworkRequest>
#include <QDesktopServices>

ViewHistoryWindow::ViewHistoryWindow(IRoster *ARoster, const Jid &AContactJid, QWidget *AParent) : QMainWindow(AParent)
{
	ui.setupUi(this);
	StyleStorage::staticStorage(RSR_STORAGE_STYLESHEETS)->insertAutoStyle(this,STS_RAMBLERHISTORY_VIEWHISTORYWINDOW);

	FProgress = 0;
	FRoster = ARoster;
	FContactJid = AContactJid;

	FBorder = CustomBorderStorage::staticStorage(RSR_STORAGE_CUSTOMBORDER)->addBorder(this, CBS_WINDOW);
	if (FBorder)
	{
		FBorder->setResizable(true);
		FBorder->setAttribute(Qt::WA_DeleteOnClose,true);
	}
	else
	{
		ui.lblCaption->setVisible(false);
		ui.centralWidget->layout()->setContentsMargins(0, 0, 0, 0);
		setAttribute(Qt::WA_DeleteOnClose,true);
	}
	resize(650,500);

	if (FRoster->xmppStream() && FRoster->xmppStream()->connection())
	{
		IDefaultConnection *defConnection = qobject_cast<IDefaultConnection *>(ARoster->xmppStream()->connection()->instance());
		if (defConnection)
			ui.wbvHistoryView->page()->networkAccessManager()->setProxy(defConnection->proxy());
	}

	connect(ui.wbvHistoryView,SIGNAL(loadProgress(int)),SLOT(onWebLoadProgress(int)));
	connect(ui.wbvHistoryView,SIGNAL(loadFinished(bool)),SLOT(onWebLoadFinished(bool)));
	connect(ui.wbvHistoryView->page(),SIGNAL(linkClicked(const QUrl &)),SLOT(onWebPageLinkClicked(const QUrl &)));

	if (FRoster)
	{
		connect(FRoster->instance(),SIGNAL(itemReceived(const IRosterItem &, const IRosterItem &)),
			SLOT(onRosterItemReceived(const IRosterItem &, const IRosterItem &)));
		connect(FRoster->instance(),SIGNAL(destroyed(QObject *)),SLOT(deleteLater()));
	}

	initViewHtml();
	updateTitle();
}

ViewHistoryWindow::~ViewHistoryWindow()
{
	if (FBorder)
		FBorder->deleteLater();
	emit windowDestroyed();
}

Jid ViewHistoryWindow::streamJid() const
{
	return FRoster->streamJid();
}

Jid ViewHistoryWindow::contactJid() const
{
	return FContactJid;
}

void ViewHistoryWindow::initViewHtml()
{
	static const QString HtmlTemplate = 
		"<html><body> \
			<div style=\"position:absolute; left:50%; top:50%; width:18px; height:18px; margin:-9px 0 0 -9px; background: transparent url('%6') no-repeat;\"></div> \
			<div style=\"display:none\"> \
				<form method=\"post\" action=\"http://id.rambler.ru/script/auth.cgi?mode=login\" name=\"auth_form\"> \
					<input type=\"hidden\" name=\"back\" value=\"http://history.xmpp.rambler.ru/m/history/talks/%1\"> \
					<input type=\"text\" name=\"login\" value=\"%2\"> \
					<input type=\"text\" name=\"domain\" value=\"%3\"> \
					<input type=\"password\" name=\"passw\" value=\"%4\"> \
					<input type=\"text\" name=\"long_session\" value=\"0\"> \
					<input type=\"submit\" name=\"user.password\" value=\"%5\"> \
				</form> \
			</div> \
			<script>document.forms.auth_form.submit()</script> \
		</body></html>";

	QString html = HtmlTemplate.arg(
		QUrl::toPercentEncoding(contactJid().eBare()).constData(),
		streamJid().eBare(),
		streamJid().pDomain(),
		FRoster->xmppStream()->password(),
		QString("Enter"),
		QUrl::fromLocalFile(IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->fileFullName(MNI_RAMBLERHISTORY_KRYTILKA)).toString());

	ui.wbvHistoryView->setHtml(html);

/*
	QByteArray body;
	body.append("back="+QString("http://mail.rambler.ru/m/history/talks/%1").arg(contactJid().eBare()).toUtf8().toPercentEncoding());
	body.append("&login="+streamJid().eBare().toUtf8().toPercentEncoding());
	body.append("&domain="+streamJid().pDomain().toUtf8().toPercentEncoding());
	body.append("&passw="+FRoster->xmppStream()->password().toUtf8().toPercentEncoding());
	body.append("&long_session=0");

	QNetworkRequest request;
	request.setHeader(QNetworkRequest::ContentTypeHeader,"application/x-www-form-urlencoded");
	request.setUrl(QUrl("http://id.rambler.ru/script/auth.cgi?mode=login"));
	ui.wbvHistoryView->load(request,QNetworkAccessManager::PostOperation,body);
*/
}

void ViewHistoryWindow::updateTitle()
{
	IRosterItem ritem = FRoster!=NULL ? FRoster->rosterItem(FContactJid) : IRosterItem();
	QString title = tr("Chat history - %1").arg(!ritem.name.isEmpty() ? ritem.name : contactJid().bare());
	if (FProgress>0 && FProgress<100)
		title += " - " + tr("Loading... %1%").arg(FProgress);

	ui.lblCaption->setText(title);
	if (FBorder)
		FBorder->setWindowTitle(ui.lblCaption->text());
	else
		setWindowTitle(ui.lblCaption->text());
}

void ViewHistoryWindow::onWebLoadProgress(int AProgress)
{
	FProgress = AProgress;
	updateTitle();
}

void ViewHistoryWindow::onWebLoadFinished(bool AOk)
{
	Q_UNUSED(AOk);
	FProgress = 100;
	updateTitle();
	ui.wbvHistoryView->page()->setLinkDelegationPolicy(QWebPage::DelegateAllLinks);
}

void ViewHistoryWindow::onWebPageLinkClicked(const QUrl &AUrl)
{
	QDesktopServices::openUrl(AUrl);
}

void ViewHistoryWindow::onRosterItemReceived(const IRosterItem &AItem, const IRosterItem &ABefore)
{
	Q_UNUSED(ABefore);
	if (AItem.itemJid && FContactJid)
		updateTitle();
}
