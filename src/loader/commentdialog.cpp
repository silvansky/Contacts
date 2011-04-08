#include "commentdialog.h"
#include <utils/log.h>
#include <utils/customborderstorage.h>
#include <definitions/resources.h>
#include <definitions/customborder.h>

CommentDialog::CommentDialog(IPluginManager *APluginManager, QWidget *AParent) : QDialog(AParent)
{
	ui.setupUi(this);
	ui.lneYourName->setAttribute(Qt::WA_MacShowFocusRect, false);

	ui.chbAddTechData->setVisible(false);

	IPlugin* plugin = APluginManager->pluginInterface("IAccountManager").value(0);
	IAccountManager *accountManager = plugin != NULL ? qobject_cast<IAccountManager *>(plugin->instance()) : NULL;
	IAccount *account = accountManager->accounts().value(0);
	connect(account->xmppStream()->instance(), SIGNAL(jidChanged(Jid)), SLOT(onJidChanded(Jid)));
	streamJid = account->xmppStream()->streamJid();

	plugin = APluginManager->pluginInterface("IVCardPlugin").value(0);
	IVCardPlugin *vCardPlugin = plugin != NULL ? qobject_cast<IVCardPlugin *>(plugin->instance()) : NULL;
	IVCard* vCard = vCardPlugin->vcard(streamJid);
	fullName = vCard->value(VVN_FULL_NAME);
	if (fullName.isEmpty())
		fullName = streamJid.node();

	plugin = APluginManager->pluginInterface("IStanzaProcessor").value(0);
	FStanzaProcessor = plugin != NULL ? qobject_cast<IStanzaProcessor *>(plugin->instance()) : NULL;

	plugin = APluginManager->pluginInterface("IMessageProcessor").value(0);
	FMessageProcessor= plugin != NULL ? qobject_cast<IMessageProcessor*>(plugin->instance()) : NULL;

	ui.lneYourName->setText(fullName);
	//connect(FStanzaProcessor->instance(), SIGNAL(stanzaSent(const Jid&, const Stanza&)), this, SLOT(stanzaSent(const Jid&, const Stanza&)));

	// border
	border = CustomBorderStorage::staticStorage(RSR_STORAGE_CUSTOMBORDER)->addBorder(this, CBS_DIALOG);
	if (border)
	{
		// init...
		border->setMinimizeButtonVisible(false);
		border->setMaximizeButtonVisible(false);
		border->setWindowTitle(windowTitle());
		connect(this, SIGNAL(accepted()), border, SLOT(closeWidget()));
		connect(this, SIGNAL(rejected()), border, SLOT(closeWidget()));
		connect(border, SIGNAL(closeClicked()), SLOT(reject()));
		border->setAttribute(Qt::WA_DeleteOnClose, true);
	}
	else
		setAttribute(Qt::WA_DeleteOnClose, true);

	connect(ui.pbtSendComment, SIGNAL(clicked()), this, SLOT(SendComment()));
}

CommentDialog::~CommentDialog()
{

}

CustomBorderContainer * CommentDialog::windowBorder() const
{
	return border;
}

//void CommentDialog::stanzaSent(const Jid &AStreamJid, const Stanza &AStanza)
void CommentDialog::SendComment()
{

	ui.pbtSendComment->setEnabled(false);
	ui.tedComment->setEnabled(false);
	ui.pbtSendComment->setText(tr("Message sending..."));

	//QString comment = ui.tedComment->toPlainText();

	Message message;
	message.setType(Message::Chat);
	FMessageProcessor->textToMessage(message, ui.tedComment->document());
	message.setTo("support@rambler.ru");
	message.setFrom(streamJid.full());
	//message.setBody(comment);
	//Stanza stanza = message.stanza();
	bool ret = FMessageProcessor->sendMessage(streamJid, message);
	//bool ret = FStanzaProcessor->sendStanzaOut(streamJid, stanza);
	if (!ret)
		Log(QString("[Comment Dialog error] Can't send comment message!"));

	//ret = true;
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

void CommentDialog::onJidChanded(Jid)
{
	IXmppStream * stream = qobject_cast<IXmppStream*>(sender());
	if (stream)
	{
		streamJid = stream->streamJid();
	}
}
