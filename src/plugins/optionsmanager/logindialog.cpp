#include "logindialog.h"

#include <QDir>
#include <QFile>
#include <QCompleter>
#include <QInputDialog>
#include <QDomDocument>
#include <QDesktopServices>

#ifdef Q_WS_WIN32
#	include <windows.h>
#elif Q_WS_X11
#	include <X11/XKBlib.h>
#	undef KeyPress
#	undef FocusIn
#	undef FocusOut
#endif

#define FILE_LOGIN		"login.xml"

LoginDialog::LoginDialog(IPluginManager *APluginManager, QWidget *AParent) : QDialog(AParent)
{
	ui.setupUi(this);
	setWindowModality(Qt::WindowModal);
	setAttribute(Qt::WA_DeleteOnClose, true);

	FNewProfile = true;
	initialize(APluginManager);
	FOptionsManager->setCurrentProfile(QString::null,QString::null);

	IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->insertAutoIcon(ui.lblLogo,MNI_OPTIONS_LOGIN_LOGO,0,0,"pixmap");

	connect(ui.lblRegister,SIGNAL(linkActivated(const QString &)),SLOT(onLabelLinkActivated(const QString &)));
	connect(ui.lblHelp,SIGNAL(linkActivated(const QString &)),SLOT(onLabelLinkActivated(const QString &)));
	connect(ui.lblForgotPassword,SIGNAL(linkActivated(const QString &)),SLOT(onLabelLinkActivated(const QString &)));

	ui.cmbDomain->lineEdit()->setReadOnly(true);
	ui.cmbDomain->addItem("rambler.ru");
	ui.cmbDomain->addItem("lenta.ru");
	ui.cmbDomain->addItem("myrambler.ru");
	ui.cmbDomain->addItem("autorambler.ru");
	ui.cmbDomain->addItem("ro.ru");
	ui.cmbDomain->addItem("r0.ru");

	QStringList profiles;
	foreach(QString profile, FOptionsManager->profiles())
	{
		Jid streamJid = Jid::decode(profile);
		if (streamJid.isValid() && !streamJid.node().isEmpty())
		{
			if (ui.cmbDomain->findText(streamJid.pDomain())<0)
				ui.cmbDomain->insertItem(0,streamJid.pDomain());
			profiles.append(streamJid.full());
		}
	}
	ui.cmbDomain->addItem(tr("Custom..."),QString("custom"));
	connect(ui.cmbDomain,SIGNAL(currentIndexChanged(int)),SLOT(onDomainCurrentIntexChanged(int)));

	QCompleter *completer = new QCompleter(profiles,ui.lneNode);
	completer->setCaseSensitivity(Qt::CaseInsensitive);
	completer->setCompletionMode(QCompleter::PopupCompletion);
	completer->popup()->setAlternatingRowColors(true);
	connect(completer,SIGNAL(activated(const QString &)),SLOT(onCompleterActivated(const QString &)));
	connect(completer,SIGNAL(highlighted(const QString &)),SLOT(onCompleterHighLighted(const QString &)));
	ui.lneNode->setCompleter(completer);

	Jid lastStreamJid = Jid::decode(FOptionsManager->lastActiveProfile());
	if (lastStreamJid.isValid())
	{
		ui.lneNode->setText(lastStreamJid.pNode());
		ui.cmbDomain->setCurrentIndex(ui.cmbDomain->findText(lastStreamJid.pDomain()));
	}
	loadCurrentProfileSettings();

	ui.lneNode->installEventFilter(this);
	ui.lneNode->completer()->popup()->installEventFilter(this);
	ui.cmbDomain->installEventFilter(this);
	ui.cmbDomain->lineEdit()->installEventFilter(this);
	ui.lnePassword->installEventFilter(this);
	ui.chbSavePassword->installEventFilter(this);
	ui.chbAutoRun->installEventFilter(this);

	if (FMainWindowPlugin)
	{
		FMainWindowPlugin->mainWindow()->instance()->installEventFilter(this);
		FMainWindowPlugin->mainWindow()->instance()->hide();
	}

	FReconnectTimer.setSingleShot(true);
	connect(&FReconnectTimer,SIGNAL(timeout()),SLOT(onReconnectTimerTimeout()));

	ui.pbtConnect->setFocus();
	connect(ui.pbtConnect,SIGNAL(clicked()),SLOT(onConnectClicked()));
}

LoginDialog::~LoginDialog()
{

}

void LoginDialog::connectIfReady()
{
	if (ui.chbSavePassword->isChecked() && !ui.lnePassword->text().isEmpty())
		onConnectClicked();
}

void LoginDialog::reject()
{
	if (!FAccountId.isNull())
	{
		IAccount *account = FAccountManager!=NULL ? FAccountManager->accountById(FAccountId) : NULL;
		if (!account || !account->isActive() || !account->xmppStream()->isOpen())
			closeCurrentProfile();
	}
	QDialog::reject();
}

bool LoginDialog::eventFilter(QObject *AWatched, QEvent *AEvent)
{
	if (AEvent->type() == QEvent::MouseButtonPress)
	{
		if (AWatched == ui.cmbDomain->lineEdit())
		{
			ui.cmbDomain->showPopup();
		}
		if (AWatched == ui.lneNode || AWatched == ui.cmbDomain || AWatched == ui.cmbDomain->lineEdit()
			|| AWatched == ui.lnePassword || AWatched == ui.chbSavePassword || AWatched == ui.chbAutoRun)
		{
			stopReconnection();
		}
	}
	else if (AEvent->type() == QEvent::FocusIn)
	{
		if (AWatched == ui.lneNode)
		{
			ui.lneNode->event(AEvent);
			disconnect(ui.lneNode->completer(),0,ui.lneNode,0);
			ui.lneNode->completer()->complete();
			return true;
		}
	}
	else if (AEvent->type() == QEvent::Show)
	{
		if (AWatched == ui.lneNode->completer()->popup())
		{
			ui.lneNode->completer()->popup()->setFixedWidth(ui.frmLogin->width());
		}
		else if (FMainWindowPlugin && AWatched == FMainWindowPlugin->mainWindow()->instance())
		{
			QTimer::singleShot(0,FMainWindowPlugin->mainWindow()->instance(),SLOT(close()));
		}
	}

	return QDialog::eventFilter(AWatched, AEvent);
}

void LoginDialog::initialize(IPluginManager *APluginManager)
{
	FOptionsManager = NULL;
	IPlugin *plugin = APluginManager->pluginInterface("IOptionsManager").value(0,NULL);
	if (plugin)
	{
		FOptionsManager = qobject_cast<IOptionsManager *>(plugin->instance());
	}

	FAccountManager = NULL;
	plugin = APluginManager->pluginInterface("IAccountManager").value(0,NULL);
	if (plugin)
	{
		FAccountManager = qobject_cast<IAccountManager *>(plugin->instance());
	}

	FStatusChanger = NULL;
	plugin = APluginManager->pluginInterface("IStatusChanger").value(0,NULL);
	if (plugin)
	{
		FStatusChanger = qobject_cast<IStatusChanger *>(plugin->instance());
	}

	FMainWindowPlugin = NULL;
	plugin = APluginManager->pluginInterface("IMainWindowPlugin").value(0,NULL);
	if (plugin)
	{
		FMainWindowPlugin = qobject_cast<IMainWindowPlugin *>(plugin->instance());
	}
}

bool LoginDialog::isCapsLockOn() const
{
#ifdef Q_WS_WIN
	return GetKeyState(VK_CAPITAL) == 1;
#elif Q_WS_X11
	Display * d = XOpenDisplay((char*)0);
	bool caps_state = false;
	if (d)
	{
		unsigned n;
		XkbGetIndicatorState(d, XkbUseCoreKbd, &n);
		caps_state = (n & 0x01) == 1;
	}
	return caps_state;
#endif
	return false;
}

void LoginDialog::closeCurrentProfile()
{
	if (!FNewProfile)
		FOptionsManager->setCurrentProfile(QString::null,QString::null);
	else if (FOptionsManager->isOpened())
		FOptionsManager->removeProfile(FOptionsManager->currentProfile());
}

void LoginDialog::setConnectEnabled(bool AEnabled)
{
	if (!AEnabled)
	{
		FReconnectTimer.stop();
		if (!ui.lblReconnect->text().isEmpty())
			ui.lblReconnect->setText(tr("Reconnecting..."));
		QTimer::singleShot(3000,this,SLOT(onShowConnectingAnimation()));
	}
	else
	{
		IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->removeAutoIcon(ui.lblConnecting);
		ui.lblConnecting->clear();
	}

	ui.lneNode->setEnabled(AEnabled);
	ui.cmbDomain->setEnabled(AEnabled);
	ui.lnePassword->setEnabled(AEnabled);
	ui.chbSavePassword->setEnabled(AEnabled);
	ui.chbAutoRun->setEnabled(AEnabled);

	ui.pbtConnect->setEnabled(AEnabled);
	ui.pbtConnect->setText(AEnabled ? tr("Connect") : tr("Connecting..."));
}

void LoginDialog::stopReconnection()
{
	if (FReconnectTimer.isActive())
	{
		IAccount *account = FAccountManager!=NULL ? FAccountManager->accountById(FAccountId) : NULL;
		if (FStatusChanger && account && account->isActive())
			FStatusChanger->setStreamStatus(account->xmppStream()->streamJid(),STATUS_OFFLINE);

		FReconnectTimer.stop();
		ui.lblReconnect->setText(QString::null);
	}
}

void LoginDialog::hideConnectionError()
{
	ui.lblConnectError->setText(QString::null);
	ui.lblReconnect->setText(QString::null);
}

void LoginDialog::showConnectionError(const QString &ACaption, const QString &AError)
{
	hideXmppStreamError();

	QString message = ACaption;
	message += message.isEmpty() || AError.isEmpty() ? AError : "<br>" + AError;
	ui.lblConnectError->setText(message);

	FReconnectTimer.start(0);
	FReconnectTimer.setProperty("ticks",10);
}

void LoginDialog::hideXmppStreamError()
{
	ui.lblXmppError->setText(QString::null);
	ui.frmLogin->setStyleSheet(QString::null);
	ui.frmPassword->setStyleSheet(QString::null);
}

void LoginDialog::showXmppStreamError(const QString &ACaption, const QString &AError, const QString &AHint)
{
	hideConnectionError();

	QString message = ACaption;
	message += message.isEmpty() || AError.isEmpty() ? AError : "<br>" + AError;
	message += message.isEmpty() || AHint.isEmpty() ? AHint : "<br>" + AHint;
	ui.lblXmppError->setText(message);

	if (FNewProfile)
		ui.frmLogin->setStyleSheet("QFrame#frmLogin { border: 1px solid red; }");
	ui.frmPassword->setStyleSheet("QFrame#frmPassword { border: 1px solid red; }");
}

void LoginDialog::onConnectClicked()
{
	if (ui.pbtConnect->isEnabled())
	{
		Jid streamJid(ui.lneNode->text().trimmed(),ui.cmbDomain->currentText(),CLIENT_NAME);
		QString profile = Jid::encode(streamJid.pBare());
		if (FOptionsManager->currentProfile() != profile)
			closeCurrentProfile();

		if (streamJid.isValid() && !streamJid.node().isEmpty())
		{
			if (!FOptionsManager->isOpened())
				FNewProfile = !FOptionsManager->profiles().contains(profile);

			if (!FNewProfile || FOptionsManager->isOpened() || FOptionsManager->addProfile(profile,QString::null))
			{
				if (FOptionsManager->setCurrentProfile(profile,QString::null))
				{
					IAccount *account = FAccountManager!=NULL ? FAccountManager->accounts().value(0, NULL) : NULL;
					if (FAccountManager && !account)
						account = FAccountManager->appendAccount(QUuid::createUuid());

					if (account)
					{
						account->setName(streamJid.domain());
						account->setStreamJid(streamJid);
						account->setPassword(ui.lnePassword->text());
						account->setActive(true);
						if (FStatusChanger && account->isActive())
						{
							setConnectEnabled(false);
							FAccountId = account->accountId();
							connect(account->xmppStream()->instance(),SIGNAL(opened()),SLOT(onXmppStreamOpened()));
							connect(account->xmppStream()->instance(),SIGNAL(closed()),SLOT(onXmppStreamClosed()));
							FStatusChanger->setStreamStatus(account->xmppStream()->streamJid(), STATUS_ONLINE);
						}
						else
							showXmppStreamError(tr("Unable to activate account"), QString::null, tr("Internal error, contact support"));
					}
					else
						showXmppStreamError(tr("Unable to create account"), QString::null, tr("Internal error, contact support"));
				}
				else
					showXmppStreamError(tr("Unable to open profile"), QString::null, tr("This profile is already opened by another Virtus instance"));
			}
			else
				showXmppStreamError(tr("Unable to create profile"), QString::null, tr("Check your system permissions to create folders"));
		}
		else
			showXmppStreamError(tr("Invalid login"), QString::null ,tr("Check your user name and domain"));
	}
}

void LoginDialog::onXmppStreamOpened()
{
	IAccount *account = FAccountManager!=NULL ? FAccountManager->accountById(FAccountId) : NULL;
	if (account && !ui.chbSavePassword->isChecked())
	{
		account->setPassword(QString::null);
	}
	Options::node(OPV_MISC_AUTOSTART).setValue(ui.chbAutoRun->isChecked());

	if (FMainWindowPlugin)
	{
		FMainWindowPlugin->mainWindow()->instance()->removeEventFilter(this);
		FMainWindowPlugin->mainWindow()->instance()->show();
	}

	saveCurrentProfileSettings();
	accept();
}

void LoginDialog::onXmppStreamClosed()
{
	IAccount *account = FAccountManager!=NULL ? FAccountManager->accountById(FAccountId) : NULL;
	if (account && !ui.chbSavePassword->isChecked())
		account->setPassword(QString::null);

	if (account && !account->xmppStream()->connection()->errorString().isEmpty())
	{
		showConnectionError(tr("Unable to connect to server"),account->xmppStream()->connection()->errorString());
	}
	else if (account)
	{
		showXmppStreamError(tr("Unable to login on server"),account->xmppStream()->errorString(),
			FNewProfile ? tr("Entered login or password is not correct") : tr("Maybe entered password is not correct"));
	}

	setConnectEnabled(true);
}

void LoginDialog::onReconnectTimerTimeout()
{
	int ticks = FReconnectTimer.property("ticks").toInt();
	if (ticks > 0)
	{
		ui.lblReconnect->setText(tr("Reconnect after %1 secs").arg(ticks));
		FReconnectTimer.setProperty("ticks",ticks-1);
		FReconnectTimer.start(1000);
	}
	else if (ticks == 0)
	{
		onConnectClicked();
	}
}

void LoginDialog::onCompleterHighLighted(const QString &AText)
{
	Jid streamJid = AText;
	ui.lneNode->setText(streamJid.node());
	ui.cmbDomain->setCurrentIndex(ui.cmbDomain->findText(streamJid.pDomain()));
}

void LoginDialog::onCompleterActivated(const QString &AText)
{
	onCompleterHighLighted(AText);
	loadCurrentProfileSettings();
}

void LoginDialog::onDomainCurrentIntexChanged(int AIndex)
{
	static int prevIndex = 0;
	if (ui.cmbDomain->itemData(AIndex).toString() == "custom")
	{
		QInputDialog *dialog = new QInputDialog(this);
		dialog->setInputMode(QInputDialog::TextInput);
		dialog->setWindowTitle("Add custom domain");
		dialog->setLabelText(tr("Enter custom domain address"));
		dialog->setOkButtonText(tr("Add"));

		if (dialog->exec() && !dialog->textValue().trimmed().isEmpty())
		{
			Jid domain = dialog->textValue().trimmed();
			int index = ui.cmbDomain->findText(domain.pDomain());
			if (index < 0)
			{
				index = 0;
				ui.cmbDomain->blockSignals(true);
				ui.cmbDomain->insertItem(0,domain.pDomain());
				ui.cmbDomain->blockSignals(false);
			}
			ui.cmbDomain->setCurrentIndex(index);
		}
		else
		{
			ui.cmbDomain->setCurrentIndex(prevIndex);
		}
		dialog->deleteLater();
	}
	else
		prevIndex = AIndex;
}

void LoginDialog::onLabelLinkActivated(const QString &ALink)
{
	QDesktopServices::openUrl(ALink);
}

void LoginDialog::saveCurrentProfileSettings()
{
	Jid streamJid(ui.lneNode->text().trimmed(),ui.cmbDomain->currentText(),CLIENT_NAME);
	QString profile = Jid::encode(streamJid.pBare());
	if (FOptionsManager->profiles().contains(profile))
	{
		QFile login(QDir(FOptionsManager->profilePath(profile)).absoluteFilePath(FILE_LOGIN));
		if (login.open(QFile::WriteOnly|QFile::Truncate))
		{
			QDomDocument doc;
			doc.appendChild(doc.createElement("login-settings"));

			QDomElement passElem = doc.documentElement().appendChild(doc.createElement("password")).toElement();
			if (ui.chbSavePassword->isChecked())
			{
				passElem.setAttribute("save","true");
				passElem.appendChild(doc.createTextNode(QString::fromLatin1(Options::encrypt(ui.lnePassword->text(),FOptionsManager->profileKey(profile,QString::null)))));
			}
			else
			{
				passElem.setAttribute("save","false");
			}

			QDomElement autoElem = doc.documentElement().appendChild(doc.createElement("auto-run")).toElement();
			autoElem.appendChild(doc.createTextNode(QVariant(ui.chbAutoRun->isChecked()).toString()));

			login.write(doc.toByteArray());
			login.close();
		}
	}
}

void LoginDialog::loadCurrentProfileSettings()
{
	Jid streamJid(ui.lneNode->text().trimmed(),ui.cmbDomain->currentText(),CLIENT_NAME);
	QString profile = Jid::encode(streamJid.pBare());
	if (FOptionsManager->profiles().contains(profile))
	{
		QDomDocument doc;
		QFile login(QDir(FOptionsManager->profilePath(profile)).absoluteFilePath(FILE_LOGIN));
		if (login.open(QFile::ReadOnly) && doc.setContent(&login))
		{
			QDomElement pasElem = doc.documentElement().firstChildElement("password");
			if (!pasElem.isNull() && QVariant(pasElem.attribute("save")).toBool())
			{
				ui.chbSavePassword->setChecked(true);
				ui.lnePassword->setText(Options::decrypt(pasElem.text().toLatin1(),FOptionsManager->profileKey(profile,QString::null)).toString());
			}
			else
			{
				ui.lnePassword->setText(QString::null);
				ui.chbSavePassword->setChecked(false);
			}

			QDomElement autoElem = doc.documentElement().firstChildElement("auto-run");
			if (!autoElem.isNull())
			{
				ui.chbAutoRun->setChecked(QVariant(autoElem.text()).toBool());
			}
			else
			{
				ui.chbAutoRun->setChecked(false);
			}
		}
		login.close();
	}
}

void LoginDialog::onShowConnectingAnimation()
{
	if (!ui.pbtConnect->isEnabled())
		IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->insertAutoIcon(ui.lblConnecting,MNI_OPTIONS_LOGIN_ANIMATION,0,0,"pixmap");
	ui.lblConnecting->adjustSize();
}
