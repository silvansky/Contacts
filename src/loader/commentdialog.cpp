#include "commentdialog.h"


CommentDialog::CommentDialog(IPluginManager *APluginManager, QWidget *AParent) : QDialog(AParent)
{
	ui.setupUi(this);

	ui.chbAddTechData->setVisible(false);

	setAttribute(Qt::WA_DeleteOnClose,true);


	IPlugin* plugin = APluginManager->pluginInterface("IAccountManager").value(0);
	IAccountManager *accountManager = plugin != NULL ? qobject_cast<IAccountManager *>(plugin->instance()) : NULL;
	IAccount *account = accountManager->accounts().value(0);
	streamJid = account->streamJid();

	plugin = APluginManager->pluginInterface("IVCardPlugin").value(0);
	IVCardPlugin *vCardPlugin = plugin != NULL ? qobject_cast<IVCardPlugin *>(plugin->instance()) : NULL;
	IVCard* vCard = vCardPlugin->vcard(streamJid);
	fullName = vCard->value(VVN_FULL_NAME);

	plugin = APluginManager->pluginInterface("IStanzaProcessor").value(0);
	FStanzaProcessor = plugin != NULL ? qobject_cast<IStanzaProcessor *>(plugin->instance()) : NULL;

	ui.lneYourName->setText(fullName);
	//connect(FStanzaProcessor->instance(), SIGNAL(stanzaSent(const Jid&, const Stanza&)), this, SLOT(stanzaSent(const Jid&, const Stanza&)));

	connect(ui.pbtSendComment, SIGNAL(clicked()), this, SLOT(SendComment()));
}

CommentDialog::~CommentDialog()
{

}

//void CommentDialog::stanzaSent(const Jid &AStreamJid, const Stanza &AStanza)
void CommentDialog::SendComment()
{

	ui.pbtSendComment->setEnabled(false);
	ui.tedComment->setEnabled(false);
	ui.pbtSendComment->setText(tr("Message sending..."));

	QString comment = ui.tedComment->toPlainText();

	Message message;
	message.setTo("support@rambler.ru");
	message.setBody(comment);
	Stanza stanza = message.stanza();
	bool ret = FStanzaProcessor->sendStanzaOut(streamJid, stanza);

	ret = true;
	if(ret)
	{
		ui.pbtSendComment->setText(tr("Message delivered"));
		ui.lblSendCommentStatus->setText(tr("Thank you for your comment."));
	}
	else
	{
		ui.lblSendCommentStatus->setText(tr("Message was not delivered. May be internet connection was lost."));
		ui.pbtSendComment->setText(tr("Send comment"));
		ui.pbtSendComment->setEnabled(true);
		ui.tedComment->setEnabled(true);
		
	}
}