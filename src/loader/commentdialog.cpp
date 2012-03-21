#include "commentdialog.h"
#include <utils/log.h>
#include <utils/widgetmanager.h>
#include <utils/customborderstorage.h>
#include <utils/stylestorage.h>
#include <utils/systemmanager.h>
#include <definitions/resources.h>
#include <definitions/customborder.h>
#include <definitions/stylesheets.h>
#include <QSysInfo>
#include <QDesktopWidget>
#include <QScrollBar>
#ifdef Q_WS_MAC
# include <utils/macwidgets.h>
#endif

CommentDialog::CommentDialog(IPluginManager *APluginManager, QWidget *AParent) : QDialog(AParent)
{
	ui.setupUi(this);
	setAttribute(Qt::WA_DeleteOnClose, true);

	CustomBorderContainer *border = CustomBorderStorage::staticStorage(RSR_STORAGE_CUSTOMBORDER)->addBorder(this, CBS_DIALOG);
	if (border)
	{
		border->setMinimizeButtonVisible(false);
		border->setMaximizeButtonVisible(false);
		border->setWindowTitle(windowTitle());
		border->setResizable(false);
		connect(this, SIGNAL(accepted()), border, SLOT(closeWidget()));
		connect(this, SIGNAL(rejected()), border, SLOT(closeWidget()));
		connect(border, SIGNAL(closeClicked()), SLOT(reject()));
		border->setAttribute(Qt::WA_DeleteOnClose, true);
	}

	ui.lneYourName->setAttribute(Qt::WA_MacShowFocusRect, false);
	ui.lneEMail->setAttribute(Qt::WA_MacShowFocusRect, false);
	ui.tedComment->setAttribute(Qt::WA_MacShowFocusRect, false);

#ifdef Q_WS_MAC
	ui.buttonsLayout->setSpacing(16);
	ui.buttonsLayout->addWidget(ui.pbtSendComment);
	setWindowGrowButtonEnabled(this->window(), false);
#endif

	ui.lblSendCommentStatus->setVisible(false);

	QString techInfo("<br><br><br>");
	techInfo += "-----------------------------<br>";
	techInfo += tr("TECHNICAL DATA (may be useful for developers)") + "<br>";
	techInfo += QString(tr("Rambler Contacts version: %1 (r%2)")).arg(APluginManager->version(), APluginManager->revision())+"<br>";
	QString os = SystemManager::systemOSVersion();

	techInfo += tr("Operating system: %1").arg(os)+"<br>";
	QDesktopWidget * dw = QApplication::desktop();
	QStringList displays;
	for (int i = 0; i < dw->screenCount(); i++)
	{
		QRect dr = dw->screenGeometry(i);
		displays << QString("%1*%2").arg(dr.width()).arg(dw->height());
	}
	techInfo += tr("Screen: %1").arg(displays.join(" + "));

	ui.tedComment->setText(techInfo);
	//ui.lblTechData->setText(techInfo);

	//StyleStorage::staticStorage(RSR_STORAGE_STYLESHEETS)->insertAutoStyle(this, STS_PLUGINMANAGER_FEEDBACK);
	QScrollBar * sb = new QScrollBar(Qt::Vertical);
	//StyleStorage::staticStorage(RSR_STORAGE_STYLESHEETS)->insertAutoStyle(sb, STS_PLUGINMANAGER_APPLICATION);
	ui.tedComment->setVerticalScrollBar(sb);
	ui.tedComment->verticalScrollBar()->installEventFilter(this);

	IPlugin* plugin = APluginManager->pluginInterface("IAccountManager").value(0);
	IAccountManager *accountManager = plugin != NULL ? qobject_cast<IAccountManager *>(plugin->instance()) : NULL;
	if (accountManager->accounts().count())
	{
		IAccount *account = accountManager->accounts().value(0);
		connect(account->xmppStream()->instance(), SIGNAL(jidChanged(Jid)), SLOT(onJidChanded(Jid)));
		streamJid = account->xmppStream()->streamJid();
	}

	plugin = APluginManager->pluginInterface("IVCardPlugin").value(0);
	IVCardPlugin *vCardPlugin = plugin != NULL ? qobject_cast<IVCardPlugin *>(plugin->instance()) : NULL;
	IVCard* vCard = vCardPlugin->vcard(streamJid);
	fullName = vCard->value(VVN_FULL_NAME);
	if (fullName.isEmpty())
		fullName = streamJid.node();
	QString email = vCard->value(VVN_EMAIL);
	if ((emailIsJid = email.isEmpty()))
		email = streamJid.bare();

	ui.lneEMail->setText(email);

	plugin = APluginManager->pluginInterface("IStanzaProcessor").value(0);
	FStanzaProcessor = plugin != NULL ? qobject_cast<IStanzaProcessor *>(plugin->instance()) : NULL;

	plugin = APluginManager->pluginInterface("IMessageProcessor").value(0);
	FMessageProcessor= plugin != NULL ? qobject_cast<IMessageProcessor*>(plugin->instance()) : NULL;

	ui.lneYourName->setText(fullName);
	//connect(FStanzaProcessor->instance(), SIGNAL(stanzaSent(const Jid&, const Stanza&)), this, SLOT(stanzaSent(const Jid&, const Stanza&)));

	ui.tedComment->setFocus();
	QTextCursor c = ui.tedComment->textCursor();
	c.setPosition(0);
	ui.tedComment->setTextCursor(c);

	ui.chbAddTechData->setVisible(false);
	ui.lblTechData->setVisible(false);

	connect(ui.pbtSendComment, SIGNAL(clicked()), this, SLOT(SendComment()));
}

CommentDialog::~CommentDialog()
{

}

void CommentDialog::show()
{
	setFixedSize(sizeHint());
	WidgetManager::showActivateRaiseWindow(window());
	window()->adjustSize();
	QTimer::singleShot(1, this, SLOT(updateStyle()));
}

//void CommentDialog::stanzaSent(const Jid &AStreamJid, const Stanza &AStanza)
void CommentDialog::SendComment()
{
	ui.pbtSendComment->setEnabled(false);
	ui.tedComment->setEnabled(false);
	ui.lneEMail->setEnabled(false);
	ui.lneYourName->setEnabled(false);
	ui.pbtSendComment->setText(tr("Sending message..."));

	QString comment = ui.tedComment->toPlainText();

	Message message;
	message.setType(Message::Chat);
	QString commentHtml = QString("<b>%1</b><br><i>%2</i><br><b>%3</b><br><br>%4").arg(Qt::escape(ui.lneYourName->text()), Qt::escape(ui.lneEMail->text()), Qt::escape(ui.lblTechData->text()), Qt::escape(comment));
	QTextDocument *doc = new QTextDocument;
	doc->setHtml(commentHtml);
	FMessageProcessor->textToMessage(message, doc);
	message.setTo("support@rambler.ru");
	message.setFrom(streamJid.full());

	bool ret = FStanzaProcessor->sendStanzaOut(streamJid, message.stanza());
	if (ret)
	{
		ui.pbtSendComment->setText(tr("Message delivered"));
		ui.lblSendCommentStatus->setVisible(true);
		ui.lblSendCommentStatus->setText(tr("Thank you for your comment."));
		ui.pbtClose->setDefault(true);
		ui.pbtClose->setText(tr("Close"));
		ui.pbtSendComment->setDefault(false);
	}
	else
	{
		ui.lblSendCommentStatus->setVisible(true);
		ui.lblSendCommentStatus->setText(tr("Message was not delivered. May be internet connection was lost."));
		ui.pbtSendComment->setText(tr("Send comment"));
		ui.pbtSendComment->setEnabled(true);
		ui.lneEMail->setEnabled(true);
		ui.lneYourName->setEnabled(true);
		ui.tedComment->setEnabled(true);
		ui.pbtClose->setDefault(false);
		ui.pbtClose->setText(tr("Cancel"));
		ui.pbtSendComment->setDefault(true);

		LogError(QString("[CommentDialog] Can't send comment message: %1").arg(message.body()));
		ReportError("FAILED-SEND-COMMENT",QString("[CommentDialog] Can't send comment message: %1").arg(message.body()));
	}
	doc->deleteLater();
}

void CommentDialog::onJidChanded(Jid)
{
	IXmppStream * stream = qobject_cast<IXmppStream*>(sender());
	if (stream)
	{
		streamJid = stream->streamJid();
		if (emailIsJid)
			ui.lneEMail->setText(streamJid.bare());
	}
}

void CommentDialog::updateStyle()
{
	StyleStorage::staticStorage(RSR_STORAGE_STYLESHEETS)->insertAutoStyle(this, STS_PLUGINMANAGER_FEEDBACK);
}

bool CommentDialog::eventFilter(QObject * obj, QEvent * event)
{
	if (obj == ui.tedComment->verticalScrollBar())
	{
		if (event->type() == QEvent::EnabledChange || event->type() == QEvent::ShowToParent || event->type() == QEvent::Show)
		{
			//ui.tedComment->verticalScrollBar()->setStyleSheet(styleSheet());
			updateStyle();
		}
	}
	return QDialog::eventFilter(obj, event);
}
