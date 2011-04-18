#include "commentdialog.h"
#include <utils/log.h>
#include <utils/customborderstorage.h>
#include <utils/stylestorage.h>
#include <definitions/resources.h>
#include <definitions/customborder.h>
#include <definitions/stylesheets.h>
#include <QSysInfo>
#include <QDesktopWidget>

#ifdef Q_WS_WIN
#include <Windows.h>
#include <comutil.h>
typedef BOOL (WINAPI *IW64PFP)(HANDLE, BOOL *);

static QString windowsSP()
{
	OSVERSIONINFO ovi;
	ovi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);

	if (GetVersionEx(&ovi))
	{
		if (ovi.szCSDVersion == L"")
			return "no SP";
		else
			return QString::fromWCharArray(ovi.szCSDVersion);
	}
	else
		return QString::null;
}

static QString windowsBitness()
{
	IW64PFP IW64P = (IW64PFP)GetProcAddress(GetModuleHandle(L"kernel32"), "IsWow64Process");
	BOOL res = FALSE;
	if (IW64P != NULL)
	{
		IW64P(GetCurrentProcess(), &res);
	}
	return res ? "64" : "32";
}

static QString resolveWidowsVersion(QSysInfo::WinVersion ver)
{
	QString win("Windows %1 %2 %3, ");
	QString version;
	switch (ver)
	{
	case QSysInfo::WV_32s:
		version = "32s";
		break;
	case QSysInfo::WV_95:
		version = "95";
		break;
	case QSysInfo::WV_98:
		version = "98";
		break;
	case QSysInfo::WV_Me:
		version = "Me";
		break;
	case QSysInfo::WV_DOS_based:
		version = "DOS based";
		break;
	case QSysInfo::WV_NT:
		version = "NT";
		break;
	case QSysInfo::WV_2000:
		version = "2000";
		break;
	case QSysInfo::WV_XP:
		version = "XP";
		break;
	case QSysInfo::WV_2003:
		version = "2003";
		break;
	case QSysInfo::WV_VISTA:
		version = "Vista";
		break;
	case QSysInfo::WV_WINDOWS7:
		version = "Seven";
		break;
	case QSysInfo::WV_NT_based:
		version = "NT Based";
		break;
	default:
		version = "Unknown";
		break;
	}
	return win.arg(version, windowsBitness(), windowsSP());
}
#endif

#ifdef Q_WS_MAC
static QString resolveMacVersion(QSysInfo::MacVersion ver)
{
	QString mac("Mac OS X %1, ");
	QString version;
	switch(ver)
	{
	case QSysInfo::MV_10_3:
		version = "10.3 (Panther)";
		break;
	case QSysInfo::MV_10_4:
		version = "10.4 (Tiger)";
		break;
	case QSysInfo::MV_10_5:
		version = "10.5 (Leopard)";
		break;
	case QSysInfo::MV_10_6:
		version = "10.6 (Snow Leopard)";
		break;
	default:
		version = "Unknown";
		break;
	}
	return mac.arg(version);
}
#endif

CommentDialog::CommentDialog(IPluginManager *APluginManager, QWidget *AParent) : QDialog(AParent)
{
	ui.setupUi(this);
	ui.lneYourName->setAttribute(Qt::WA_MacShowFocusRect, false);

	QString techInfo;
#ifdef Q_WS_WIN
	techInfo += resolveWidowsVersion(QSysInfo::windowsVersion());
#elif defined (Q_WS_MAC)
	techInfo += resolveMacVersion(QSysInfo::MacintoshVersion);
#endif
	QDesktopWidget * dw = QApplication::desktop();
	for (int i = 0; i < dw->screenCount(); i++)
	{
		QRect dr = dw->screenGeometry(i);
		techInfo += QString("%1x%2 px, ").arg(dr.width()).arg(dw->height());
	}
	techInfo += QString(tr("version %1 (r%2)")).arg(APluginManager->version(), APluginManager->revision());

	ui.lblTechData->setText(techInfo);

	StyleStorage::staticStorage(RSR_STORAGE_STYLESHEETS)->insertAutoStyle(this, STS_PLUGINMANAGER_FEEDBACK);

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

	QString comment = ui.tedComment->toPlainText();

	Message message;
	message.setType(Message::Chat);
	QString commentHtml = QString("<b>%1</b><br><i>%2</i><br><b>%3</b><br><br>%4<br><br>Technical data: %5").arg(Qt::escape(ui.lneYourName->text()), Qt::escape(ui.lneEMail->text()), Qt::escape(ui.lblTechData->text()), Qt::escape(comment), (ui.chbAddTechData->isChecked() ? ui.lblTechData->text() : "[not added]"));
	QTextDocument * doc = new QTextDocument;
	doc->setHtml(commentHtml);
	FMessageProcessor->textToMessage(message, doc);
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
	doc->deleteLater();
}

void CommentDialog::onJidChanded(Jid)
{
	IXmppStream * stream = qobject_cast<IXmppStream*>(sender());
	if (stream)
	{
		streamJid = stream->streamJid();
	}
}
