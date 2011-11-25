#include "commentdialog.h"
#include <utils/log.h>
#include <utils/widgetmanager.h>
#include <utils/customborderstorage.h>
#include <utils/stylestorage.h>
#include <definitions/resources.h>
#include <definitions/customborder.h>
#include <definitions/stylesheets.h>
#include <QSysInfo>
#include <QDesktopWidget>
#include <QScrollBar>
#ifdef Q_WS_MAC
# include <utils/macwidgets.h>
#endif

#ifdef Q_WS_WIN
#include <Windows.h>
#ifndef __MINGW32__
# include <comutil.h>
#endif
typedef BOOL (WINAPI *IW64PFP)(HANDLE, BOOL *);

static QString windowsLanguage()
{
#ifndef __MINGW32__
	LANGID lid = GetUserDefaultUILanguage();
#else
	LANGID lid = 0x0409; // debug only! TODO: fix this function to fit mingw
#endif
	LCID lcid = MAKELCID(lid, SORT_DEFAULT);
	wchar_t * buff = new wchar_t[10];
	int size = GetLocaleInfo(lcid, LOCALE_SLANGUAGE, 0, 0);
	if (size)
	{
		buff = new wchar_t[size];
		int ret = GetLocaleInfo(lcid, LOCALE_SLANGUAGE, buff, size);
		QString res = QString::fromWCharArray(buff);
		delete buff;
		return ret ? res : QString::null;
	}
	return QString::null;
}

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
	QString win("Windows %1 %2 %3 %4-bit");
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
	return win.arg(version, windowsSP(), windowsLanguage(), windowsBitness());
}
#endif

#ifdef Q_WS_MAC
static QString resolveMacVersion(QSysInfo::MacVersion ver)
{
	QString mac("Mac OS X %1.%2.%3 (%4)");
	QString version;
	SInt32 majVer = 0, minVer = 0, fixVer = 0;
	Gestalt(gestaltSystemVersionMajor, &majVer);
	Gestalt(gestaltSystemVersionMinor, &minVer);
	Gestalt(gestaltSystemVersionBugFix, &fixVer);
	switch(minVer)
	{
	case 3:
		version = "Panther";
		break;
	case 4:
		version = "Tiger";
		break;
	case 5:
		version = "Leopard";
		break;
	case 6:
		version = "Snow Leopard";
		break;
	case 7:
		version = "Lion";
		break;
	default:
		version = "Unknown";
		break;
	}
	return mac.arg(majVer).arg(minVer).arg(fixVer).arg(version);
}
#endif

CommentDialog::CommentDialog(IPluginManager *APluginManager, QWidget *AParent) : QDialog(AParent)
{
	ui.setupUi(this);
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
	QString os;
#ifdef Q_WS_WIN
	os = resolveWidowsVersion(QSysInfo::windowsVersion());
#elif defined (Q_WS_MAC)
	os = resolveMacVersion(QSysInfo::MacintoshVersion);
#endif

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
	IAccount *account = accountManager->accounts().value(0);
	connect(account->xmppStream()->instance(), SIGNAL(jidChanged(Jid)), SLOT(onJidChanded(Jid)));
	streamJid = account->xmppStream()->streamJid();

	plugin = APluginManager->pluginInterface("IVCardPlugin").value(0);
	IVCardPlugin *vCardPlugin = plugin != NULL ? qobject_cast<IVCardPlugin *>(plugin->instance()) : NULL;
	IVCard* vCard = vCardPlugin->vcard(streamJid);
	fullName = vCard->value(VVN_FULL_NAME);
	if (fullName.isEmpty())
		fullName = streamJid.node();
	QString email = vCard->value(VVN_EMAIL);
	if (emailIsJid = email.isEmpty())
		email = streamJid.bare();

	ui.lneEMail->setText(email);

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
		border->setResizable(false);
		connect(this, SIGNAL(accepted()), border, SLOT(closeWidget()));
		connect(this, SIGNAL(rejected()), border, SLOT(closeWidget()));
		connect(border, SIGNAL(closeClicked()), SLOT(reject()));
		border->setAttribute(Qt::WA_DeleteOnClose, true);
	}
	else
		setAttribute(Qt::WA_DeleteOnClose, true);

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

CustomBorderContainer * CommentDialog::windowBorder() const
{
	return border;
}

void CommentDialog::show()
{
	setFixedSize(sizeHint());
	if (border)
	{
		WidgetManager::showActivateRaiseWindow(border);
		border->adjustSize();
	}
	else
		WidgetManager::showActivateRaiseWindow(this);
	//setStyleSheet(styleSheet());
	//QTimer::singleShot(1, StyleStorage::staticStorage(RSR_STORAGE_STYLESHEETS), SLOT(previewReset()));
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
