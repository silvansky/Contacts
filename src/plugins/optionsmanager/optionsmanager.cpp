#include "optionsmanager.h"

#include <QSettings>
#include <QFileInfo>
#include <QDateTime>
#include <QApplication>
#include <QCryptographicHash>
#include <definitions/customborder.h>

#define DIR_PROFILES                    "profiles"
#define DIR_BINARY                      "binary"
#define FILE_PROFILE                    "profile.xml"
#define FILE_OPTIONS                    "options.xml"
#define FILE_BLOCKER                    "blocked"

#define PROFILE_VERSION                 "1.0"

#define ADR_PROFILE                     Action::DR_Parametr1

#define PST_OPTIONS                     "options"
#define PSN_OPTIONS                     "ramblercontacts:options"

OptionsManager::OptionsManager()
{
	FPluginManager = NULL;
	FTrayManager = NULL;
	FMainWindowPlugin = NULL;
	FPrivateStorage = NULL;
	FOptionsDialog = NULL;
	FOptionsDialogBorder = NULL;
	FLoginDialog = NULL;
	FLoginDialogBorder = NULL;

	FAutoSaveTimer.setInterval(30*1000);
	FAutoSaveTimer.setSingleShot(true);
	connect(&FAutoSaveTimer, SIGNAL(timeout()),SLOT(onAutoSaveTimerTimeout()));

	qsrand(QDateTime::currentDateTime().toTime_t());
}

OptionsManager::~OptionsManager()
{
	if (FOptionsDialogBorder)
		FOptionsDialogBorder->deleteLater();
	else
		delete FOptionsDialog;
}

void OptionsManager::pluginInfo(IPluginInfo *APluginInfo)
{
	APluginInfo->name = tr("Options Manager");
	APluginInfo->description = tr("Allows to save, load and manage user preferences");
	APluginInfo ->version = "1.0";
	APluginInfo->author = "Potapov S.A. aka Lion";
	APluginInfo->homePage = "http://contacts.rambler.ru";
	APluginInfo->conflicts.append("{6030FCB2-9F1E-4ea2-BE2B-B66EBE0C4367}"); // ISettings
}

bool OptionsManager::initConnections(IPluginManager *APluginManager, int &AInitOrder)
{
	Q_UNUSED(AInitOrder);

	FPluginManager = APluginManager;
	connect(FPluginManager->instance(),SIGNAL(aboutToQuit()),SLOT(onAboutToQuit()));

	IPlugin *plugin = APluginManager->pluginInterface("IMainWindowPlugin").value(0,NULL);
	if (plugin)
	{
		FMainWindowPlugin = qobject_cast<IMainWindowPlugin *>(plugin->instance());
	}

	plugin = APluginManager->pluginInterface("ITrayManager").value(0,NULL);
	if (plugin)
	{
		FTrayManager = qobject_cast<ITrayManager *>(plugin->instance());
	}

	plugin = APluginManager->pluginInterface("IPrivateStorage").value(0,NULL);
	if (plugin)
	{
		FPrivateStorage = qobject_cast<IPrivateStorage *>(plugin->instance());
		if (FPrivateStorage)
		{
			connect(FPrivateStorage->instance(),SIGNAL(storageOpened(const Jid &)),SLOT(onPrivateStorageOpened(const Jid &)));
			connect(FPrivateStorage->instance(),SIGNAL(dataLoaded(const QString &, const Jid &, const QDomElement &)),
				SLOT(onPrivateStorageDataLoaded(const QString &, const Jid &, const QDomElement &)));
			connect(FPrivateStorage->instance(),SIGNAL(storageAboutToClose(const Jid &)),SLOT(onPrivateStorageAboutToClose(const Jid &)));
		}
	}

	connect(Options::instance(),SIGNAL(optionsChanged(const OptionsNode &)),SLOT(onOptionsChanged(const OptionsNode &)));

	return true;
}

bool OptionsManager::initObjects()
{
	FProfilesDir.setPath(FPluginManager->homePath());
	if (!FProfilesDir.exists(DIR_PROFILES))
		FProfilesDir.mkdir(DIR_PROFILES);
	FProfilesDir.cd(DIR_PROFILES);

	FChangeProfileAction = new Action(this);
	//FChangeProfileAction->setIcon(RSR_STORAGE_MENUICONS,MNI_OPTIONS_PROFILES);
	FChangeProfileAction->setText(tr("Change User"));
	FChangeProfileAction->setData(Action::DR_SortString,QString("100"));
	connect(FChangeProfileAction,SIGNAL(triggered(bool)),SLOT(onChangeProfileByAction(bool)));

	FShowOptionsDialogAction = new Action(this);
	FShowOptionsDialogAction->setVisible(false);
	//FShowOptionsDialogAction->setIcon(RSR_STORAGE_MENUICONS,MNI_OPTIONS_DIALOG);
	FShowOptionsDialogAction->setText(tr("Options"));
#ifdef Q_WS_MAC
	FShowOptionsDialogAction->setShortcut(tr("Ctrl+,"));
#endif
	FShowOptionsDialogAction->setShortcutContext(Qt::ApplicationShortcut);
	FShowOptionsDialogAction->setData(Action::DR_SortString,QString("300"));
	connect(FShowOptionsDialogAction,SIGNAL(triggered(bool)),SLOT(onShowOptionsDialogByAction(bool)));

	if (FMainWindowPlugin)
	{
		FMainWindowPlugin->mainWindow()->mainMenu()->addAction(FShowOptionsDialogAction,AG_MMENU_OPTIONS_SHOWDIALOG,true);
		FMainWindowPlugin->mainWindow()->mainMenu()->addAction(FChangeProfileAction,AG_MMENU_OPTIONS_CHANGEPROFILE,true);
	}

	if (FTrayManager)
	{
		//FTrayManager->contextMenu()->addAction(FChangeProfileAction,AG_TMTM_OPTIONS,true);
		FTrayManager->contextMenu()->addAction(FShowOptionsDialogAction,AG_TMTM_OPTIONS,true);
	}

	return true;
}

bool OptionsManager::initSettings()
{
	Options::setDefaultValue(OPV_MISC_AUTOSTART, false);
	Options::setDefaultValue(OPV_MISC_OPTIONS_SAVE_ON_SERVER, true);
	Options::setDefaultValue(OPV_MISC_OPTIONS_DIALOG_LASTNODE, QString(OPN_COMMON));

	IOptionsDialogNode dnode = { ONO_COMMON, OPN_COMMON, tr("Common Settings"), MNI_OPTIONS_DIALOG };
	insertOptionsDialogNode(dnode);
	insertOptionsHolder(this);

	return true;
}

bool OptionsManager::startPlugin()
{
	LoginDialog *dialog = qobject_cast<LoginDialog *>(showLoginDialog());
	if (dialog)
	{
		dialog->loadLastProfile();
		dialog->connectIfReady();
	}
	return true;
}

QMultiMap<int, IOptionsWidget *> OptionsManager::optionsWidgets(const QString &ANodeId, QWidget *AParent)
{
	QMultiMap<int, IOptionsWidget *> widgets;
	if (ANodeId == OPN_COMMON)
	{
		widgets.insertMulti(OWO_COMMON_AUTOSTART, optionsHeaderWidget(QString::null, tr("Common settings"),AParent));
		widgets.insertMulti(OWO_COMMON_AUTOSTART, optionsNodeWidget(Options::node(OPV_MISC_AUTOSTART), tr("Launch application on system start up"), AParent));

		widgets.insertMulti(OWO_COMMON_SINC, optionsHeaderWidget(QString::null, tr("Backing store your chat history and preferences"),AParent));
		widgets.insertMulti(OWO_COMMON_SINC_OPTIONS, optionsNodeWidget(Options::node(OPV_MISC_OPTIONS_SAVE_ON_SERVER), tr("Sync preferences on my several computers"), AParent));
	}
	return widgets;
}

bool OptionsManager::isOpened() const
{
	return !FProfile.isEmpty();
}

QList<QString> OptionsManager::profiles() const
{
	QList<QString> profileList;

	foreach(QString dirName, FProfilesDir.entryList(QDir::Dirs|QDir::NoDotAndDotDot))
		if (FProfilesDir.exists(dirName + "/" FILE_PROFILE))
			profileList.append(dirName);

	return profileList;
}

QString OptionsManager::profilePath(const QString &AProfile) const
{
	return FProfilesDir.absoluteFilePath(AProfile);
}

QString OptionsManager::lastActiveProfile() const
{
	QDateTime lastModified;
	QString lastProfile;
	foreach(QString profile, profiles())
	{
		QFileInfo info(profilePath(profile) + "/" FILE_OPTIONS);
		if (info.exists() && info.lastModified()>lastModified)
		{
			lastProfile = profile;
			lastModified = info.lastModified();
		}
	}
	return lastProfile;
}

QString OptionsManager::currentProfile() const
{
	return FProfile;
}

QByteArray OptionsManager::currentProfileKey() const
{
	return FProfileKey;
}

bool OptionsManager::setCurrentProfile(const QString &AProfile, const QString &APassword)
{
	if (AProfile.isEmpty())
	{
		closeProfile();
		return true;
	}
	else if (AProfile == currentProfile())
	{
		return true;
	}
	else if (checkProfilePassword(AProfile, APassword))
	{
		closeProfile();
		FProfileLocker = new QtLockedFile(QDir(profilePath(AProfile)).absoluteFilePath(FILE_BLOCKER));
		if (FProfileLocker->open(QFile::WriteOnly) && FProfileLocker->lock(QtLockedFile::WriteLock, false))
		{
			QDir profileDir(profilePath(AProfile));
			if (!profileDir.exists(DIR_BINARY))
				profileDir.mkdir(DIR_BINARY);

			QFile optionsFile(profileDir.filePath(FILE_OPTIONS));
			if (!optionsFile.open(QFile::ReadOnly) || !FProfileOptions.setContent(optionsFile.readAll(),true))
			{
				FProfileOptions.clear();
				FProfileOptions.appendChild(FProfileOptions.createElement("options")).toElement();
			}
			optionsFile.close();

			if (profileKey(AProfile,APassword).size() < 16)
				changeProfilePassword(AProfile,APassword,APassword);

			openProfile(AProfile, APassword);
			return true;
		}
		FProfileLocker->close();
		delete FProfileLocker;
	}
	return false;
}

QByteArray OptionsManager::profileKey(const QString &AProfile, const QString &APassword) const
{
	if (checkProfilePassword(AProfile, APassword))
	{
		QDomNode keyText = profileDocument(AProfile).documentElement().firstChildElement("key").firstChild();
		while (!keyText.isNull() && !keyText.isText())
			keyText = keyText.nextSibling();

		QByteArray keyValue = QByteArray::fromBase64(keyText.toText().data().toLatin1());
		return Options::decrypt(keyValue, QCryptographicHash::hash(APassword.toUtf8(),QCryptographicHash::Md5)).toByteArray();
	}
	return QByteArray();
}

bool OptionsManager::checkProfilePassword(const QString &AProfile, const QString &APassword) const
{
	QDomDocument profileDoc = profileDocument(AProfile);
	if (!profileDoc.isNull())
	{
		QDomNode passText = profileDoc.documentElement().firstChildElement("password").firstChild();
		while (!passText.isNull() && !passText.isText())
			passText = passText.nextSibling();

		if (passText.isNull() && APassword.isEmpty())
			return true;

		QByteArray passHash = QCryptographicHash::hash(APassword.toUtf8(),QCryptographicHash::Sha1);
		return passHash.toHex() == passText.toText().data().toLatin1();
	}
	return false;
}

bool OptionsManager::changeProfilePassword(const QString &AProfile, const QString &AOldPassword, const QString &ANewPassword)
{
	if (checkProfilePassword(AProfile, AOldPassword))
	{
		QDomDocument profileDoc = profileDocument(AProfile);

		QDomElement passElem = profileDoc.documentElement().firstChildElement("password");
		if (passElem.isNull())
			passElem = profileDoc.documentElement().appendChild(profileDoc.createElement("password")).toElement();

		QDomNode passText = passElem.firstChild();
		while (!passText.isNull() && !passText.isText())
			passText = passText.nextSibling();

		QByteArray newPassHash = QCryptographicHash::hash(ANewPassword.toUtf8(),QCryptographicHash::Sha1);
		if (passText.isNull())
			passElem.appendChild(passElem.ownerDocument().createTextNode(newPassHash.toHex()));
		else
			passText.toText().setData(newPassHash.toHex());

		QDomNode keyText = profileDoc.documentElement().firstChildElement("key").firstChild();
		while (!keyText.isNull() && !keyText.isText())
			keyText = keyText.nextSibling();

		QByteArray keyValue = QByteArray::fromBase64(keyText.toText().data().toLatin1());
		keyValue = Options::decrypt(keyValue, QCryptographicHash::hash(AOldPassword.toUtf8(),QCryptographicHash::Md5)).toByteArray();
		if (keyValue.size() < 16)
		{
			keyValue.resize(16);
			for (int i=0; i<keyValue.size(); i++)
				keyValue[i] = qrand();
		}
		keyValue = Options::encrypt(keyValue, QCryptographicHash::hash(ANewPassword.toUtf8(),QCryptographicHash::Md5));
		keyText.toText().setData(keyValue.toBase64());

		return saveProfile(AProfile, profileDoc);
	}
	return false;
}

bool OptionsManager::addProfile(const QString &AProfile, const QString &APassword)
{
	if (!profiles().contains(AProfile))
	{
		if (FProfilesDir.exists(AProfile) || FProfilesDir.mkdir(AProfile))
		{
			QDomDocument profileDoc;
			profileDoc.appendChild(profileDoc.createElement("profile"));
			profileDoc.documentElement().setAttribute("version",PROFILE_VERSION);

			QByteArray passHash = QCryptographicHash::hash(APassword.toUtf8(),QCryptographicHash::Sha1);
			QDomNode passElem = profileDoc.documentElement().appendChild(profileDoc.createElement("password"));
			passElem.appendChild(profileDoc.createTextNode(passHash.toHex()));

			QByteArray keyData(16,0);
			for (int i=0; i<keyData.size(); i++)
				keyData[i] = qrand();
			keyData = Options::encrypt(keyData, QCryptographicHash::hash(APassword.toUtf8(),QCryptographicHash::Md5));

			QDomNode keyElem = profileDoc.documentElement().appendChild(profileDoc.createElement("key"));
			keyElem.appendChild(profileDoc.createTextNode(keyData.toBase64()));

			if (saveProfile(AProfile, profileDoc))
			{
				emit profileAdded(AProfile);
				return true;
			}
		}
	}
	return false;
}

bool OptionsManager::renameProfile(const QString &AProfile, const QString &ANewName)
{
	if (!FProfilesDir.exists(ANewName) && FProfilesDir.rename(AProfile, ANewName))
	{
		emit profileRenamed(AProfile,ANewName);
		return true;
	}
	return false;
}

bool OptionsManager::removeProfile(const QString &AProfile)
{
	QDir profileDir(profilePath(AProfile));
	if (profileDir.exists())
	{
		if (AProfile == currentProfile())
			closeProfile();

		if (profileDir.remove(FILE_PROFILE))
		{
			emit profileRemoved(AProfile);
			return true;
		}
	}
	return false;
}

QList<QString> OptionsManager::serverOptions() const
{
	return FServerOptions;
}

void OptionsManager::insertServerOption(const QString &APath)
{
	if (!APath.isEmpty() && !FServerOptions.contains(APath))
		FServerOptions.append(APath);
}

void OptionsManager::removeServerOption(const QString &APath)
{
	FServerOptions.removeAll(APath);
}

QDialog *OptionsManager::showLoginDialog(QWidget *AParent)
{
	if (!FLoginDialog)
	{
		FLoginDialog = new LoginDialog(FPluginManager,AParent);
		connect(FLoginDialog,SIGNAL(rejected()),SLOT(onLoginDialogRejected()));
		connect(FLoginDialog,SIGNAL(accepted()),SLOT(onLoginDialogAccepted()));
		if (FLoginDialogBorder)
			FLoginDialogBorder->deleteLater();
		FLoginDialogBorder = CustomBorderStorage::staticStorage(RSR_STORAGE_CUSTOMBORDER)->addBorder(FLoginDialog, CBS_DIALOG);
		if (FLoginDialogBorder)
		{
			FLoginDialogBorder->setAttribute(Qt::WA_DeleteOnClose, true);
			FLoginDialogBorder->setResizable(false);
			FLoginDialogBorder->setMinimizeButtonVisible(false);
			FLoginDialogBorder->setMaximizeButtonVisible(false);
			connect(FLoginDialogBorder, SIGNAL(closeClicked()), FLoginDialog, SLOT(reject()));
			connect(FLoginDialog, SIGNAL(accepted()), FLoginDialogBorder, SLOT(close()));
			connect(FLoginDialog, SIGNAL(rejected()), FLoginDialogBorder, SLOT(close()));
		}
		WidgetManager::showActivateRaiseWindow(FLoginDialogBorder ? (QWidget*)FLoginDialogBorder: (QWidget*)FLoginDialog);
		if (FLoginDialogBorder)
			FLoginDialogBorder->adjustSize();
		else
			FLoginDialog->adjustSize();
	}
	return FLoginDialog;
}

QDialog *OptionsManager::showEditProfilesDialog(QWidget *AParent)
{
	if (FEditProfilesDialog.isNull())
		FEditProfilesDialog = new EditProfilesDialog(this,AParent);
	WidgetManager::showActivateRaiseWindow(FEditProfilesDialog);
	return FEditProfilesDialog;
}

QList<IOptionsHolder *> OptionsManager::optionsHolders() const
{
	return FOptionsHolders;
}

void OptionsManager::insertOptionsHolder(IOptionsHolder *AHolder)
{
	if (!FOptionsHolders.contains(AHolder))
	{
		FOptionsHolders.append(AHolder);
		emit optionsHolderInserted(AHolder);
	}
}

void OptionsManager::removeOptionsHolder(IOptionsHolder *AHolder)
{
	if (FOptionsHolders.contains(AHolder))
	{
		FOptionsHolders.removeAll(AHolder);
		emit optionsHolderRemoved(AHolder);
	}
}

QList<IOptionsDialogNode> OptionsManager::optionsDialogNodes() const
{
	return FOptionsDialogNodes.values();
}

IOptionsDialogNode OptionsManager::optionsDialogNode(const QString &ANodeId) const
{
	return FOptionsDialogNodes.value(ANodeId);
}

void OptionsManager::insertOptionsDialogNode(const IOptionsDialogNode &ANode)
{
	if (!ANode.nodeId.isEmpty())
	{
		FOptionsDialogNodes[ANode.nodeId] = ANode;
		emit optionsDialogNodeInserted(ANode);
	}
}

void OptionsManager::removeOptionsDialogNode(const QString &ANodeId)
{
	if (FOptionsDialogNodes.contains(ANodeId))
	{
		emit optionsDialogNodeRemoved(FOptionsDialogNodes.take(ANodeId));
	}
}

QWidget *OptionsManager::showOptionsDialog(const QString &ANodeId, QWidget *AParent)
{
	if (isOpened())
	{
		if (!FOptionsDialog)
		{
			FOptionsDialog = new OptionsDialog(this,AParent);
			connect(FOptionsDialog, SIGNAL(applied()), SLOT(onOptionsDialogApplied()));
			connect(FOptionsDialog, SIGNAL(dialogDestroyed()), SLOT(onOptionsDialogDestroyed()));
			FOptionsDialogBorder = CustomBorderStorage::staticStorage(RSR_STORAGE_CUSTOMBORDER)->addBorder(FOptionsDialog, CBS_OPTIONSDIALOG);
			if (FOptionsDialogBorder)
			{
				FOptionsDialogBorder->setAttribute(Qt::WA_DeleteOnClose, true);
				FOptionsDialogBorder->setMaximizeButtonVisible(false);
				FOptionsDialogBorder->setResizable(false);
				connect(FOptionsDialog, SIGNAL(accepted()), FOptionsDialogBorder, SLOT(closeWidget()));
				connect(FOptionsDialog, SIGNAL(rejected()), FOptionsDialogBorder, SLOT(closeWidget()));
				connect(FOptionsDialogBorder, SIGNAL(closeClicked()), FOptionsDialog, SLOT(reject()));
			}
		}
		FOptionsDialog->showNode(ANodeId.isNull() ? Options::node(OPV_MISC_OPTIONS_DIALOG_LASTNODE).value().toString() : ANodeId);
		WidgetManager::showActivateRaiseWindow(FOptionsDialogBorder ? (QWidget*)FOptionsDialogBorder : (QWidget*)FOptionsDialog);
		FOptionsDialog->adjustSize();
		FOptionsDialog->layout()->update();
		if (FOptionsDialogBorder)
		{
			FOptionsDialogBorder->layout()->update();
			//FOptionsDialogBorder->resize(FOptionsDialogBorder->sizeHint());
			FOptionsDialogBorder->adjustSize();
		}
	}
	return FOptionsDialogBorder ? (QWidget*)FOptionsDialogBorder : (QWidget*)FOptionsDialog;
}

IOptionsContainer *OptionsManager::optionsContainer(QWidget *AParent) const
{
	return new OptionsContainer(this,AParent);
}

IOptionsWidget *OptionsManager::optionsHeaderWidget(const QString &AIconKey, const QString &ACaption, QWidget *AParent) const
{
	return new OptionsHeader(AIconKey,ACaption,AParent);
}

IOptionsWidget *OptionsManager::optionsNodeWidget(const OptionsNode &ANode, const QString &ACaption, QWidget *AParent) const
{
	return new OptionsWidget(ANode, ACaption, AParent);
}

void OptionsManager::openProfile(const QString &AProfile, const QString &APassword)
{
	if (!isOpened())
	{
		FProfile = AProfile;
		FProfileKey = profileKey(AProfile, APassword);
		Options::setOptions(FProfileOptions, profilePath(AProfile) + "/" DIR_BINARY, FProfileKey);
		FShowOptionsDialogAction->setVisible(true);
		FChangeProfileAction->setText(tr("Change User (%1)").arg(Jid(Jid::decode(AProfile)).node()));
		emit profileOpened(AProfile);
	}
}

bool OptionsManager::saveProfile(const QString &AProfile, const QDomDocument &AProfileDoc) const
{
	QFile file(profilePath(AProfile) + "/" FILE_PROFILE);
	if (file.open(QFile::WriteOnly|QFile::Truncate))
	{
		file.write(AProfileDoc.toString(2).toUtf8());
		file.close();
		return true;
	}
	return false;
}

void OptionsManager::closeProfile()
{
	if (isOpened())
	{
		emit profileClosed(currentProfile());
		FAutoSaveTimer.stop();
		if (FOptionsDialog)
		{
			if (FOptionsDialogBorder)
				FOptionsDialogBorder->close();
			else
				FOptionsDialog->close();
		}
		FShowOptionsDialogAction->setVisible(false);
		FChangeProfileAction->setText(tr("Change User"));
		Options::setOptions(QDomDocument(), QString::null, QByteArray());
		saveOptions();
		FProfile.clear();
		FProfileKey.clear();
		FProfileOptions.clear();
		FProfileLocker->unlock();
		FProfileLocker->close();
		FProfileLocker->remove();
		delete FProfileLocker;
	}
}

bool OptionsManager::saveOptions() const
{
	if (isOpened())
	{
		QFile file(QDir(profilePath(currentProfile())).filePath(FILE_OPTIONS));
		if (file.open(QIODevice::WriteOnly|QIODevice::Truncate))
		{
			file.write(FProfileOptions.toString(2).toUtf8());
			file.close();
			return true;
		}
	}
	return false;
}

bool OptionsManager::loadServerOptions(const Jid &AStreamJid)
{
	if (FPrivateStorage && AStreamJid.isValid())
	{
		return !FPrivateStorage->loadData(AStreamJid,PST_OPTIONS,PSN_OPTIONS).isEmpty();
	}
	return false;
}

bool OptionsManager::saveServerOptions(const Jid &AStreamJid)
{
	if (FPrivateStorage && AStreamJid.isValid())
	{
		QDomDocument doc;
		doc.appendChild(doc.createElement("options"));

		if (FPrivateStorage->hasData(AStreamJid,PST_OPTIONS,PSN_OPTIONS))
			doc.documentElement().appendChild(FPrivateStorage->getData(AStreamJid,PST_OPTIONS,PSN_OPTIONS).cloneNode(true));
		else
			doc.documentElement().appendChild(doc.createElementNS(PSN_OPTIONS,PST_OPTIONS)).toElement();

		QDomElement root = doc.documentElement().firstChildElement();
		foreach(QString path, FServerOptions)
			Options::exportNode(path,root);

		return !FPrivateStorage->saveData(AStreamJid,root).isEmpty();
	}
	return false;
}

QDomDocument OptionsManager::profileDocument(const QString &AProfile) const
{
	QDomDocument doc;
	QFile file(profilePath(AProfile) + "/" FILE_PROFILE);
	if (file.open(QFile::ReadOnly))
	{
		doc.setContent(file.readAll(),true);
		file.close();
	}
	return doc;
}

void OptionsManager::importOldSettings()
{
	IPlugin *plugin = FPluginManager->pluginInterface("IAccountManager").value(0);
	IAccountManager *accountManager = plugin!=NULL ? qobject_cast<IAccountManager *>(plugin->instance()) : NULL;
	if (accountManager)
	{
		foreach(QString dirName, FProfilesDir.entryList(QDir::Dirs|QDir::NoDotAndDotDot))
		{
			QFile settings(FProfilesDir.absoluteFilePath(dirName + "/settings.xml"));
			if (!FProfilesDir.exists(dirName + "/" FILE_PROFILE) && settings.open(QFile::ReadOnly))
			{
				QDomDocument doc;
				if (doc.setContent(&settings,true) && addProfile(dirName,QString::null) && setCurrentProfile(dirName,QString::null))
				{
					QDomElement accountElem = doc.documentElement().firstChildElement("plugin");
					while (!accountElem.isNull() &&  QUuid(accountElem.attribute("pluginId"))!=QUuid(ACCOUNTMANAGER_UUID))
						accountElem = accountElem.nextSiblingElement("plugin");

					accountElem = accountElem.firstChildElement("account");
					while (!accountElem.isNull())
					{
						IAccount *account = accountManager->appendAccount(accountElem.attribute("ns"));
						if (account)
						{
							QByteArray key = QUuid(accountElem.attribute("ns")).toString().toUtf8();
							QByteArray password = QByteArray::fromBase64(qUncompress(QByteArray::fromBase64(accountElem.firstChildElement("password").attribute("value").toLocal8Bit())));
							for (int i = 0; i<password.size(); ++i)
								password[i] = password[i] ^ key[i % key.size()];

							account->setName(accountElem.firstChildElement("name").attribute("value"));
							account->setStreamJid(accountElem.firstChildElement("streamJid").attribute("value"));
							account->setPassword(QString::fromUtf8(password));

							QDomElement connectElem = doc.documentElement().firstChildElement("plugin");
							while (!connectElem.isNull() &&  QUuid(connectElem.attribute("pluginId"))!=QUuid(accountElem.firstChildElement("connectionId").attribute("value")))
								connectElem = connectElem.nextSiblingElement("plugin");

							connectElem = connectElem.firstChildElement("connection");
							while (!connectElem.isNull() && connectElem.attribute("ns")!=accountElem.attribute("ns"))
								connectElem = connectElem.nextSiblingElement("connection");

							if (!connectElem.isNull())
							{
								OptionsNode cnode = account->optionsNode().node("connection",account->optionsNode().value("connection-type").toString());
								cnode.setValue(connectElem.firstChildElement("host").attribute("value"),"host");
								cnode.setValue(connectElem.firstChildElement("port").attribute("value").toInt(),"port");
								cnode.setValue(QVariant(connectElem.firstChildElement("useSSL").attribute("value")).toBool(),"use-ssl");
								cnode.setValue(QVariant(connectElem.firstChildElement("ingnoreSSLErrors").attribute("value")).toBool(),"ignore-ssl-errors");
							}
						}
						accountElem = accountElem.nextSiblingElement("account");
					}
					setCurrentProfile(QString::null,QString::null);
				}
				settings.close();
			}
		}
	}
}

void OptionsManager::onOptionsChanged(const OptionsNode &ANode)
{
	if (ANode.path() == OPV_MISC_AUTOSTART)
	{
#ifdef Q_WS_WIN
		QSettings reg("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run", QSettings::NativeFormat);
		if (ANode.value().toBool())
			reg.setValue(CLIENT_NAME, QDir::toNativeSeparators(QApplication::applicationFilePath()));
		else
			reg.remove(CLIENT_NAME);
#endif
	}
	FAutoSaveTimer.start();
}

void OptionsManager::onOptionsDialogApplied()
{
	saveOptions();
}

void OptionsManager::onOptionsDialogDestroyed()
{
	FOptionsDialog = NULL;
	FOptionsDialogBorder = NULL;
}

void OptionsManager::onChangeProfileByAction(bool)
{
	showLoginDialog();
}

void OptionsManager::onShowOptionsDialogByAction(bool)
{
	showOptionsDialog();
}

void OptionsManager::onLoginDialogRejected()
{
	FLoginDialog = NULL;
	FLoginDialogBorder = NULL;
	if (!isOpened())
		FPluginManager->quit();
}

void OptionsManager::onLoginDialogAccepted()
{
	FLoginDialog = NULL;
	FLoginDialogBorder = NULL;
}

void OptionsManager::onAutoSaveTimerTimeout()
{
	saveOptions();
}

void OptionsManager::onPrivateStorageOpened(const Jid &AStreamJid)
{
	if (Options::node(OPV_MISC_OPTIONS_SAVE_ON_SERVER).value().toBool())
		loadServerOptions(AStreamJid);
}

void OptionsManager::onPrivateStorageDataLoaded(const QString &AId, const Jid &AStreamJid, const QDomElement &AElement)
{
	Q_UNUSED(AId); Q_UNUSED(AStreamJid);
	if (AElement.tagName()==PST_OPTIONS && AElement.namespaceURI()==PSN_OPTIONS)
	{
		foreach(QString path, FServerOptions)
			Options::importNode(path,AElement);
	}
}

void OptionsManager::onPrivateStorageAboutToClose(const Jid &AStreamJid)
{
	if (Options::node(OPV_MISC_OPTIONS_SAVE_ON_SERVER).value().toBool())
		saveServerOptions(AStreamJid);
}

void OptionsManager::onAboutToQuit()
{
	closeProfile();
}

Q_EXPORT_PLUGIN2(plg_optionsmanager, OptionsManager)
