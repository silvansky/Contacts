#include "viewhistorywindow.h"

#include <QWebFrame>
#include <QNetworkRequest>
#include <QDesktopServices>

ViewHistoryWindow::ViewHistoryWindow(IRoster *ARoster, const Jid &AContactJid, QWidget *AParent) : QMainWindow(AParent)
{
	ui.setupUi(this);
	StyleStorage::staticStorage(RSR_STORAGE_STYLESHEETS)->insertAutoStyle(this,STS_RAMBLERHISTORY_VIEWHISTORYWINDOW);

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
		setAttribute(Qt::WA_DeleteOnClose,true);
	}
	resize(650,500);

	connect(FRoster->instance(),SIGNAL(received(const IRosterItem &, const IRosterItem &)),
		SLOT(onRosterItemReceived(const IRosterItem &, const IRosterItem &)));
	connect(FRoster->instance(),SIGNAL(destroyed(QObject *)),SLOT(deleteLater()));

	IRosterItem ritem = FRoster->rosterItem(AContactJid);
	ritem.itemJid = FContactJid;
	onRosterItemReceived(ritem,ritem);
	
	if (FRoster->xmppStream() && FRoster->xmppStream()->connection())
	{
		IDefaultConnection *defConnection = qobject_cast<IDefaultConnection *>(ARoster->xmppStream()->connection()->instance());
		if (defConnection)
			ui.wbvHistoryView->page()->networkAccessManager()->setProxy(defConnection->proxy());
	}

	ui.wbvHistoryView->page()->setLinkDelegationPolicy(QWebPage::DelegateAllLinks);
	connect(ui.wbvHistoryView->page(),SIGNAL(linkClicked(const QUrl &)),SLOT(onWebPageLinkClicked(const QUrl &)));

	initViewHtml();
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
					<input type=\"hidden\" name=\"back\" value=\"http://mail.rambler.ru/m/history/talks/%1\"> \
					<input type=\"text\" name=\"login\" value=\"%2\"> \
					<input type=\"text\" name=\"domain\" value=\"%3\"> \
					<input type=\"password\" name=\"passw\" value=\"%4\"> \
					<input type=\"text\" name=\"long_session\" value=\"0\"> \
					<input type=\"submit\" name=\"user.password\" value=\"%5\"> \
				</form> \
			</div> \
			<script>document.forms.auth_form.submit()</script> \
		</body></html>";

	QString html = HtmlTemplate
		.arg(contactJid().bare())
		.arg(streamJid().bare())
		.arg(streamJid().domain())
		.arg(FRoster->xmppStream()->password())
		.arg("Enter")
		.arg(QUrl::fromLocalFile(IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->fileFullName(MNI_RAMBLERHISTORY_KRYTILKA)).toString());
	ui.wbvHistoryView->setHtml(html);
}

void ViewHistoryWindow::onWebPageLinkClicked(const QUrl &AUrl)
{
	QDesktopServices::openUrl(AUrl);
}

void ViewHistoryWindow::onRosterItemReceived(const IRosterItem &AItem, const IRosterItem &ABefore)
{
	Q_UNUSED(ABefore);
	if (AItem.itemJid && FContactJid)
	{
		ui.lblCaption->setText(tr("Chat history - %1").arg(!AItem.name.isEmpty() ? AItem.name : contactJid().bare()));
		if (FBorder)
			FBorder->setWindowTitle(ui.lblCaption->text());
		else
			setWindowTitle(ui.lblCaption->text());
	}
}
