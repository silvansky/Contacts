#include "addfacebookaccountdialog.h"

AddFacebookAccountDialog::AddFacebookAccountDialog(IGateways *AGateways, IRegistration *ARegistration, const Jid &AStreamJid, const Jid &AServiceJid, QWidget *AParent) : QDialog(AParent)
{
	ui.setupUi(this);
	setAttribute(Qt::WA_DeleteOnClose,true);
	setWindowModality(AParent ? Qt::WindowModal : Qt::NonModal);
	StyleStorage::staticStorage(RSR_STORAGE_STYLESHEETS)->insertAutoStyle(this,STS_GATEWAYS_ADDFACEBOOKACCOUNTDIALOG);

	FGateways = AGateways;
	FRegistration = ARegistration;

	FStreamJid = AStreamJid;
	FServiceJid = AServiceJid;

	setMaximumSize(500,500);

	ui.wbvView->page()->setLinkDelegationPolicy(QWebPage::DelegateAllLinks);
	connect(ui.wbvView->page(),SIGNAL(linkClicked(const QUrl &)),SLOT(onWebPageLinkClicked(const QUrl &)));

	ui.wbvView->load(QUrl("http://fb.tx.friends.rambler.ru/auth"));
}

AddFacebookAccountDialog::~AddFacebookAccountDialog()
{

}

void AddFacebookAccountDialog::onWebPageLinkClicked(const QUrl &AUrl)
{

}
