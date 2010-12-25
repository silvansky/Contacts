#include "addcontactdialog.h"

#include <QSet>
#include <QLabel>
#include <QClipboard>
#include <QHBoxLayout>
#include <QPushButton>
#include <QMessageBox>
#include <QInputDialog>
#include <QApplication>

#define GROUP_NEW                ":group_new:"
#define GROUP_EMPTY              ":empty_group:"

#define URL_ACTION_ADD_ACCOUNT   "AddAction"

#define RESOLVE_WAIT_INTERVAL    1000

AddContactDialog::AddContactDialog(IRosterChanger *ARosterChanger, IPluginManager *APluginManager, const Jid &AStreamJid, QWidget *AParent) : QDialog(AParent)
{
	ui.setupUi(this);
	setAttribute(Qt::WA_DeleteOnClose,true);
	setWindowTitle(tr("Add contact"));
	IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->insertAutoIcon(this,MNI_RCHANGER_ADD_CONTACT,0,0,"windowIcon");
	StyleStorage::staticStorage(RSR_STORAGE_STYLESHEETS)->insertAutoStyle(this,STS_RCHANGER_ADDCONTACTDIALOG);

	FRoster = NULL;
	FAvatars = NULL;
	FVcardPlugin = NULL;
	FRosterChanger = ARosterChanger;

	FShown = false;
	FStreamJid = AStreamJid;

	ui.wdtGateways->setLayout(new QHBoxLayout);
	ui.wdtGateways->layout()->setMargin(0);
	qobject_cast<QHBoxLayout *>(ui.wdtGateways->layout())->addStretch();

	int left, top, right, bottom;
	FServiceIcon = new QLabel(ui.lneContact);
	FServiceIcon->setFixedSize(15,15);
	FServiceIcon->setScaledContents(true);
	ui.lneContact->setLayout(new QHBoxLayout);
	ui.lneContact->getTextMargins(&left, &top, &right, &bottom);
	ui.lneContact->setTextMargins(left,top,right+FServiceIcon->width()+right,bottom);
	qobject_cast<QHBoxLayout *>(ui.lneContact->layout())->setContentsMargins(left,top,right,bottom);
	qobject_cast<QHBoxLayout *>(ui.lneContact->layout())->addStretch();
	qobject_cast<QHBoxLayout *>(ui.lneContact->layout())->addWidget(FServiceIcon);

	ui.lblProfile->setVisible(false);
	ui.cmbProfile->setVisible(false);
	IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->insertAutoIcon(ui.lblSearchAnimate,MNI_RCHANGER_ADDCONTACT_SEARCH_NICK,0,0,"pixmap");

	ui.lblPhoto->clear();
	ui.lblInfo->setVisible(false);
	ui.lblInfoIcon->setVisible(false);
	IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->insertAutoIcon(ui.lblInfoIcon,MNI_RCHANGER_ADDCONTACT_INFO,0,0,"pixmap");
	ui.lblError->setVisible(false);
	ui.lblErrorIcon->setVisible(false);
	IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->insertAutoIcon(ui.lblErrorIcon,MNI_RCHANGER_ADDCONTACT_ERROR,0,0,"pixmap");
	ui.lblAction->setVisible(false);
	connect(ui.lblAction,SIGNAL(linkActivated(const QString &)),SLOT(onActionLinkActivated(const QString &)));
	ui.dbbButtons->button(QDialogButtonBox::Ok)->setText(tr("Add contact"));

	QIcon icon = IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getIcon(MNI_GATEWAYS_SERVICE_RAMBLER,0);
	ui.cmbProfile->addItem(icon,FStreamJid.bare());

	FResolveTimer.setSingleShot(true);
	connect(&FResolveTimer,SIGNAL(timeout()),SLOT(resolveServiceJid()));

	initialize(APluginManager);
	initGroups();
	updateGateways();
	updateServices();
	setRealContactJid(Jid::null);
	setResolveNickState(false);

	QString contact = qApp->clipboard()->text();
	Jid userJid = contact;
	if (FGateways && !FGateways->descriptorsByContact(contact).isEmpty())
		setContactText(contact);
	else if (userJid.isValid() && !userJid.node().isEmpty())
		setContactJid(userJid.bare());

	connect(ui.cmbGroup,SIGNAL(currentIndexChanged(int)),SLOT(onGroupCurrentIndexChanged(int)));
	connect(ui.cmbProfile,SIGNAL(currentIndexChanged(int)),SLOT(onProfileCurrentIndexChanged(int)));

	connect(ui.lneNick,SIGNAL(textEdited(const QString &)),SLOT(onContactNickEdited(const QString &)));
	connect(ui.lneContact,SIGNAL(textEdited(const QString &)),SLOT(onContactTextEdited(const QString &)));

	connect(ui.dbbButtons,SIGNAL(accepted()),SLOT(onDialogAccepted()));
	connect(ui.dbbButtons,SIGNAL(rejected()),SLOT(reject()));
}

AddContactDialog::~AddContactDialog()
{
	emit dialogDestroyed();
}

Jid AddContactDialog::streamJid() const
{
	return FStreamJid;
}

Jid AddContactDialog::contactJid() const
{
	return FContactJid;
}

void AddContactDialog::setContactJid(const Jid &AContactJid)
{
	if (FContactJid != AContactJid.bare())
	{
		QString contact = AContactJid.bare();
		if (FGateways && FGateways->streamServices(FStreamJid).contains(AContactJid.domain()))
		{
			contact = FGateways->legacyIdFromUserJid(AContactJid);
			FPreferGateJid = AContactJid.domain();
		}
		else
		{
			FPreferGateJid = Jid::null;
		}
		setContactText(contact);
		startResolve(0);
	}
}

QString AddContactDialog::contactText() const
{
	return ui.lneContact->text();
}

void AddContactDialog::setContactText(const QString &AText)
{
	ui.lneContact->setText(AText);
}

QString AddContactDialog::nickName() const
{
	QString nick = ui.lneNick->text().trimmed();
	if (nick.isEmpty())
	{
		Jid userJid = contactText();
		if (!userJid.node().isEmpty())
		{
			nick = userJid.node();
			nick[0] = nick[0].toUpper();
		}
	}
	return nick;
}

void AddContactDialog::setNickName(const QString &ANick)
{
	ui.lneNick->setText(ANick);
}

QString AddContactDialog::group() const
{
	return ui.cmbGroup->itemData(ui.cmbGroup->currentIndex()).isNull() ? ui.cmbGroup->currentText() : QString::null;
}

void AddContactDialog::setGroup(const QString &AGroup)
{
	int index = ui.cmbGroup->findText(AGroup);
	if (AGroup.isEmpty())
		ui.cmbGroup->setCurrentIndex(0);
	else if (index < 0)
		ui.cmbGroup->insertItem(ui.cmbGroup->count()-1,AGroup);
	else if (index > 0)
		ui.cmbGroup->setCurrentIndex(index);
}

Jid AddContactDialog::gatewayJid() const
{
	return ui.cmbProfile->itemData(ui.cmbProfile->currentIndex()).toString();
}

void AddContactDialog::setGatewayJid(const Jid &AGatewayJid)
{
	int index = ui.cmbProfile->findData(AGatewayJid.pDomain());
	ui.cmbProfile->setCurrentIndex(index>0 ? index : 0);
}

void AddContactDialog::initialize(IPluginManager *APluginManager)
{
	IPlugin *plugin = APluginManager->pluginInterface("IRosterPlugin").value(0,NULL);
	if (plugin)
	{
		IRosterPlugin *rosterPlugin = qobject_cast<IRosterPlugin *>(plugin->instance());
		FRoster = rosterPlugin!=NULL ? rosterPlugin->getRoster(FStreamJid) : NULL;
	}

	plugin = APluginManager->pluginInterface("IAvatars").value(0,NULL);
	if (plugin)
	{
		FAvatars = qobject_cast<IAvatars *>(plugin->instance());
	}

	plugin = APluginManager->pluginInterface("IVCardPlugin").value(0,NULL);
	if (plugin)
	{
		FVcardPlugin = qobject_cast<IVCardPlugin *>(plugin->instance());
		if (FVcardPlugin)
		{
			connect(FVcardPlugin->instance(), SIGNAL(vcardReceived(const Jid &)),SLOT(onVCardReceived(const Jid &)));
			connect(FVcardPlugin->instance(), SIGNAL(vcardError(const Jid &, const QString &)),SLOT(onVCardError(const Jid &, const QString &)));
		}
	}

	plugin = APluginManager->pluginInterface("IGateways").value(0,NULL);
	if (plugin)
	{
		FGateways = qobject_cast<IGateways *>(plugin->instance());
		if (FGateways)
		{
			connect(FGateways->instance(),SIGNAL(loginReceived(const QString &, const QString &)),SLOT(onServiceLoginReceived(const QString &, const QString &)));
			connect(FGateways->instance(),SIGNAL(userJidReceived(const QString &, const Jid &)),SLOT(onLegacyContactJidReceived(const QString &, const Jid &)));
			connect(FGateways->instance(),SIGNAL(serviceEnableChanged(const Jid &, const Jid &, bool)),SLOT(onServiceEnableChanged(const Jid &, const Jid &, bool)));
			connect(FGateways->instance(),SIGNAL(errorReceived(const QString &, const QString &)),SLOT(onGatewayErrorReceived(const QString &, const QString &)));
		}
	}
}

void AddContactDialog::initGroups()
{
	QList<QString> groups = FRoster!=NULL ? FRoster->groups().toList() : QList<QString>();
	qSort(groups);
	ui.cmbGroup->addItem(tr("<Common Group>"),QString(GROUP_EMPTY));
	ui.cmbGroup->addItems(groups);
	ui.cmbGroup->addItem(tr("New Group..."),QString(GROUP_NEW));

	int last = ui.cmbGroup->findText(Options::node(OPV_ROSTER_ADDCONTACTDIALOG_LASTGROUP).value().toString());
	if (last>=0 && last<ui.cmbGroup->count()-1)
		ui.cmbGroup->setCurrentIndex(last);
}

void AddContactDialog::updateGateways()
{
	if (FGateways)
	{
		IDiscoIdentity identity;
		identity.category = "gateway";
		QList<Jid> removeList = FEnabledGateways;
		foreach(Jid serviceJid, FGateways->availServices(FStreamJid,identity))
		{
			IGateServiceDescriptor descriptor = FGateways->serviceDescriptor(FStreamJid,serviceJid);
			if (!FDisabledGateways.contains(serviceJid) && (!descriptor.needLogin || FGateways->isServiceEnabled(FStreamJid, serviceJid)))
			{
				if (!FEnabledGateways.contains(serviceJid))
				{
					if (descriptor.needLogin && !FLoginRequests.values().contains(serviceJid))
					{
						QString id = FGateways->sendLoginRequest(FStreamJid,serviceJid);
						if (!id.isEmpty())
							FLoginRequests.insert(id,serviceJid);
					}
					QIcon icon = IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getIcon(descriptor.iconKey);
					ui.cmbProfile->addItem(icon,serviceJid.pDomain(),serviceJid.pDomain());
					FEnabledGateways.append(serviceJid);
				}
				removeList.removeAll(serviceJid);
			}
		}

		bool resolve = false;
		foreach(Jid serviceJid, removeList)
		{
			int index = ui.cmbProfile->findData(serviceJid.pDomain());
			ui.cmbProfile->removeItem(index);
			FEnabledGateways.removeAll(serviceJid);
			resolve = true;
		}

		if (FEnabledGateways.isEmpty() && ui.cmbProfile->isVisible())
		{
			ui.lblProfile->setVisible(false);
			ui.cmbProfile->setVisible(false);
			QTimer::singleShot(1,this,SLOT(onAdjustDialogSize()));
		}
		else if (!FEnabledGateways.isEmpty() && !ui.cmbProfile->isVisible())
		{
			ui.lblProfile->setVisible(true);
			ui.cmbProfile->setVisible(true);
			QTimer::singleShot(1,this,SLOT(onAdjustDialogSize()));
		}

		if (resolve)
		{
			startResolve(RESOLVE_WAIT_INTERVAL/2);
		}
	}	
}

void AddContactDialog::updateServices(const Jid &AServiceJid)
{
	if (FGateways)
	{
		foreach(QString name, FGateways->availDescriptors())
		{
			if (!FServices.contains(name))
			{
				bool show = false;
				IGateServiceDescriptor descriptor = FGateways->descriptorByName(name);
				if (descriptor.needGate)
				{
					IDiscoIdentity identity;
					identity.category = "gateway";
					identity.type = descriptor.type;
					show = !FGateways->availServices(FStreamJid,identity).toSet().intersect(FEnabledGateways.toSet()).isEmpty();
				}
				else
				{
					show = true;
				}
				if (show)
				{
					QLabel *label = new QLabel(ui.wdtGateways);
					label->setToolTip(name);
					IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->insertAutoIcon(label,descriptor.iconKey,0,0,"pixmap");
					ui.wdtGateways->layout()->addWidget(label);
					qobject_cast<QHBoxLayout *>(ui.wdtGateways->layout())->addStretch();
					FServices.insert(name,AServiceJid);
				}
			}
		}
	}
	else
	{
		ui.wdtGateways->setVisible(false);
	}
}

QString AddContactDialog::normalContactText(const QString &AText) const
{
	return AText.trimmed().toLower();
}

QString AddContactDialog::defaultContactNick(const Jid &AContactJid) const
{
	QString nick = AContactJid.node();
	nick = nick.isEmpty() ? AContactJid.full() : nick;
	if (!nick.isEmpty())
	{
		nick[0] = nick[0].toUpper();
		for (int pos = nick.indexOf('_'); pos>=0; pos = nick.indexOf('_',pos+1))
		{
			if (pos+1 < nick.length())
				nick[pos+1] = nick[pos+1].toUpper();
			nick.replace(pos,1,' ');
		}
	}
	return nick.trimmed();
}

QList<Jid> AddContactDialog::suitableServices(const IGateServiceDescriptor &ADescriptor) const
{
	IDiscoIdentity identity;
	identity.category = "gateway";
	identity.type = ADescriptor.type;
	QList<Jid> services = FGateways!=NULL ? (ADescriptor.needLogin ? FGateways->streamServices(FStreamJid,identity) : FGateways->availServices(FStreamJid,identity)) : QList<Jid>();
	QList<Jid> gates = (FEnabledGateways.toSet() & services.toSet()).toList();
	
	QList<Jid>::iterator it = gates.begin();
	while (it != gates.end())
	{
		if (!ADescriptor.prefix.isEmpty() && !it->pDomain().startsWith(ADescriptor.prefix))
			it = gates.erase(it);
		else
			it++;
	}

	return gates;
}

QList<Jid> AddContactDialog::suitableServices(const QList<IGateServiceDescriptor> &ADescriptors) const
{
	QList<Jid> gates;
	for (int i=0; i<ADescriptors.count(); i++)
		gates += suitableServices(ADescriptors.at(i));
	return gates.toSet().toList();
}

void AddContactDialog::startResolve(int ATimeout)
{
	setRealContactJid(Jid::null);
	setGatewaysEnabled(false);
	setResolveNickState(false);
	setContactAcceptable(false);
	setInfoMessage(QString::null);
	setErrorMessage(QString::null);
	setActionLink(QString::null,QUrl());
	FResolveTimer.start(ATimeout);
}

void AddContactDialog::setInfoMessage(const QString &AMessage)
{
	if (AMessage.isEmpty() && ui.lblInfo->isVisible())
		QTimer::singleShot(1,this,SLOT(onAdjustDialogSize()));
	else if (!AMessage.isEmpty() && !ui.lblInfo->isVisible())
		QTimer::singleShot(1,this,SLOT(onAdjustDialogSize()));

	ui.lblInfo->setText(AMessage);
	ui.lblInfo->setVisible(!AMessage.isEmpty());
	ui.lblInfoIcon->setVisible(!AMessage.isEmpty());
}

void AddContactDialog::setErrorMessage(const QString &AMessage)
{
	if (AMessage.isEmpty() && ui.lblError->isVisible())
		QTimer::singleShot(1,this,SLOT(onAdjustDialogSize()));
	else if (!AMessage.isEmpty() && !ui.lblError->isVisible())
		QTimer::singleShot(1,this,SLOT(onAdjustDialogSize()));

	ui.lblError->setText(AMessage);
	ui.lblError->setVisible(!AMessage.isEmpty());
	ui.lblErrorIcon->setVisible(!AMessage.isEmpty());
}

void AddContactDialog::setActionLink(const QString &AMessage, const QUrl &AUrl)
{
	if (AMessage.isEmpty() && ui.lblAction->isVisible())
		QTimer::singleShot(1,this,SLOT(onAdjustDialogSize()));
	else if (!AMessage.isEmpty() && !ui.lblAction->isVisible())
		QTimer::singleShot(1,this,SLOT(onAdjustDialogSize()));

	ui.lblAction->setText(QString("<a href='%1'>%2</a>").arg(AUrl.toString()).arg(AMessage));
	ui.lblAction->setVisible(!AMessage.isEmpty());
}

void AddContactDialog::setGatewaysEnabled(bool AEnabled)
{
	ui.cmbProfile->setEnabled(AEnabled);
}

void AddContactDialog::setContactAcceptable(bool AAcceptable)
{
	ui.lneNick->setEnabled(AAcceptable);
	ui.cmbGroup->setEnabled(AAcceptable);
	ui.dbbButtons->button(QDialogButtonBox::Ok)->setEnabled(AAcceptable);
}

void AddContactDialog::setRealContactJid(const Jid &AContactJid)
{
	if (FAvatars)
		FAvatars->insertAutoAvatar(ui.lblPhoto,AContactJid,QSize(50,50),"pixmap");
	FContactJid = AContactJid.bare();
}

void AddContactDialog::setResolveNickState(bool AResolve)
{
	FResolveNick = AResolve;
	if (AResolve)
		ui.lneNick->setText(QString::null);
	ui.lblSearchAnimate->setVisible(AResolve);
	ui.lblSearchNick->setVisible(AResolve);
}

void AddContactDialog::resolveServiceJid()
{
	QUrl actionUrl;
	QString errMessage;
	QString actionMessage;
	bool nextResolve = false;

	QString contact = normalContactText(contactText());
	if (ui.lneContact->text() != contact)
		ui.lneContact->setText(contact);

	IGateServiceDescriptor descriptor = FGateways!=NULL ? FGateways->descriptorByContact(contact) : IGateServiceDescriptor();
	if (descriptor.valid)
	{
		bool offerAccount = false;
		QList<Jid> gateways = suitableServices(descriptor);
		if (!gateways.isEmpty())
		{
			nextResolve = true;
			Jid gate = !FPreferGateJid.isValid() ? Options::node(OPV_ROSTER_ADDCONTACTDIALOG_LASTPROFILE).value().toString() : FPreferGateJid;
			setGatewayJid(gateways.contains(gate) ? gate : gateways.first());
		}
		else if (!descriptor.needGate)
		{
			nextResolve = true;
			offerAccount = true;
			setGatewayJid(Jid::null);
		}
		else if (descriptor.needLogin)
		{
			offerAccount = true;
			errMessage = tr("To add a user to %1, you must enter your %1 account.").arg(descriptor.name);
			setGatewayJid(Jid::null);
		}
		else
		{
			offerAccount = false;
			errMessage = tr("%1 service is unavailable").arg(descriptor.name);
			setGatewayJid(Jid::null);
		}
		if (FGateways && offerAccount && descriptor.needLogin)
		{
			IDiscoIdentity identity;
			identity.category = "gateway";
			identity.type = descriptor.type;
			QList<Jid> freeServices = (FGateways->availServices(FStreamJid,identity).toSet() - FGateways->streamServices(FStreamJid,identity).toSet()).toList();
			if (!freeServices.isEmpty())
			{
				actionUrl.setScheme(URL_ACTION_ADD_ACCOUNT);
				actionUrl.setPath(freeServices.first().pDomain());
				actionMessage = tr("Enter your %1 account").arg(descriptor.name);
			}
		}
		IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->insertAutoIcon(FServiceIcon,descriptor.iconKey,0,0,"pixmap");
	}
	else if (!contact.isEmpty())
	{
		Jid userJid = contact;
		if (userJid.isValid() && !userJid.node().isEmpty())
			nextResolve = true;
		else
			errMessage = tr("Invalid address. Please check the address and try again.");
		setGatewayJid(FPreferGateJid);
		IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->insertAutoIcon(FServiceIcon,MNI_GATEWAYS_SERVICE_RAMBLER,0,0,"pixmap");
	}
	else
	{
		IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->removeAutoIcon(FServiceIcon);
	}

	FPreferGateJid = Jid::null;
	setErrorMessage(errMessage);
	setActionLink(actionMessage, actionUrl);
	setGatewaysEnabled(!contact.isEmpty() && errMessage.isEmpty());

	if (nextResolve)
		resolveContactJid();
	else
		setContactAcceptable(!contact.isEmpty() && errMessage.isEmpty());
}

void AddContactDialog::resolveContactJid()
{
	QString errMessage;
	bool nextResolve = false;

	QString contact = normalContactText(contactText());
	Jid userJid = contact;

	Jid gateJid = gatewayJid();
	QList<Jid> gateways = suitableServices(FGateways!=NULL ? FGateways->descriptorsByContact(contact) : QList<IGateServiceDescriptor>());
	if (!gateJid.isEmpty() && !gateways.contains(gateJid))
	{
		errMessage = tr("You cannot add this contact to selected profile");
	}
	else if (FGateways && FEnabledGateways.contains(gateJid))
	{
		FContactJidRequest = FGateways->sendUserJidRequest(FStreamJid,gateJid,contact);
		if (FContactJidRequest.isEmpty())
			errMessage = tr("Unable to determine the contact ID");
	}
	else if (gateJid.isEmpty() && userJid.isValid() && !userJid.node().isEmpty())
	{
		nextResolve = true;
		setRealContactJid(contact);
	}
	else
	{
		errMessage = tr("Selected profile is not acceptable");
	}

	setErrorMessage(errMessage);
	setContactAcceptable(!contact.isEmpty() && errMessage.isEmpty());

	if (nextResolve)
		resolveContactName();
	else
		setInfoMessage(QString::null);
}

void AddContactDialog::resolveContactName()
{
	QString infoMessage;
	if (FContactJid.isValid())
	{
		IRosterItem ritem = FRoster!=NULL ? FRoster->rosterItem(FContactJid) : IRosterItem();
		if (!ritem.isValid)
		{
			if (FVcardPlugin && FVcardPlugin->requestVCard(FStreamJid, FContactJid))
				setResolveNickState(true);
			else
				setNickName(defaultContactNick(contactText()));
		}
		else
		{
			setNickName(!ritem.name.isEmpty() ? ritem.name : defaultContactNick(contactText()));
			setGroup(ritem.groups.toList().value(0));
			infoMessage = tr("This address is already in your contact-list.");
		}
	}
	setInfoMessage(infoMessage);
}

void AddContactDialog::showEvent(QShowEvent *AEvent)
{
	if (!FShown)
	{
		FShown = true;
		FResolveTimer.start(0);
		QTimer::singleShot(1,this,SLOT(onAdjustDialogSize()));
	}
	QDialog::showEvent(AEvent);
}

void AddContactDialog::onDialogAccepted()
{
	if (FRoster && FContactJid.isValid())
	{
		IRosterItem ritem = FRoster->rosterItem(FContactJid);
		FRoster->setItem(contactJid(),nickName(),QSet<QString>()<<group());
		FRosterChanger->subscribeContact(FStreamJid,contactJid(),QString::null);
		accept();
	}
}

void AddContactDialog::onAdjustDialogSize()
{
	adjustSize();
}

void AddContactDialog::onContactTextEdited(const QString &AText)
{
	Q_UNUSED(AText);
	startResolve(RESOLVE_WAIT_INTERVAL);
}

void AddContactDialog::onContactNickEdited(const QString &AText)
{
	Q_UNUSED(AText);
	setResolveNickState(false);
}

void AddContactDialog::onGroupCurrentIndexChanged(int AIndex)
{
	if (ui.cmbGroup->itemData(AIndex).toString() == GROUP_NEW)
	{
		int index = 0;
		QString newGroup = QInputDialog::getText(this,tr("Create group"),tr("New group name")).trimmed();
		if (!newGroup.isEmpty())
		{
			index = ui.cmbGroup->findText(newGroup);
			if (index < 0)
			{
				ui.cmbGroup->blockSignals(true);
				ui.cmbGroup->insertItem(1,newGroup);
				ui.cmbGroup->blockSignals(false);
				index = 1;
			}
			else if (!ui.cmbGroup->itemData(AIndex).isNull())
			{
				index = 0;
			}
		}
		ui.cmbGroup->setCurrentIndex(index);
	}
}

void AddContactDialog::onProfileCurrentIndexChanged(int AIndex)
{
	Q_UNUSED(AIndex);
	resolveContactJid();
}

void AddContactDialog::onActionLinkActivated(const QString &ALink)
{
	QUrl url = ALink;
	if (FGateways && url.scheme() == URL_ACTION_ADD_ACCOUNT)
	{
		FGateways->showAddLegacyAccountDialog(FStreamJid,url.path(),this);
	}
}

void AddContactDialog::onVCardReceived(const Jid &AContactJid)
{
	if (AContactJid && FContactJid)
	{
		if (FResolveNick)
		{
			IVCard *vcard = FVcardPlugin->vcard(FContactJid);
			QString nick = vcard->value(VVN_NICKNAME);
			setNickName(nick.isEmpty() ? defaultContactNick(contactText()) : nick);
			vcard->unlock();
			setResolveNickState(false);
		}
	}
}

void AddContactDialog::onVCardError(const Jid &AContactJid, const QString &AError)
{
	Q_UNUSED(AError);
	if (AContactJid && FContactJid)
	{
		if (FResolveNick)
		{
			setNickName(defaultContactNick(contactText()));
			setResolveNickState(false);
		}
	}
}

void AddContactDialog::onServiceLoginReceived(const QString &AId, const QString &ALogin)
{
	if (FLoginRequests.contains(AId))
	{
		Jid serviceJid = FLoginRequests.take(AId);
		int index = ui.cmbProfile->findData(serviceJid.pDomain());
		if (index >= 0)
			ui.cmbProfile->setItemText(index, ALogin);
		FEnabledGateways.append(serviceJid);
		FDisabledGateways.removeAll(serviceJid);
	}
}

void AddContactDialog::onLegacyContactJidReceived(const QString &AId, const Jid &AUserJid)
{
	if (FContactJidRequest == AId)
	{
		setRealContactJid(AUserJid);
		setContactAcceptable(true);
		resolveContactName();
	}
}

void AddContactDialog::onServiceEnableChanged(const Jid &AStreamJid, const Jid &AServiceJid, bool AEnabled)
{
	Q_UNUSED(AEnabled);
	if (AStreamJid == FStreamJid)
	{
		FDisabledGateways.removeAll(AServiceJid);
		updateGateways();
		updateServices();
	}
}

void AddContactDialog::onGatewayErrorReceived(const QString &AId, const QString &AError)
{
	if (FContactJidRequest == AId)
	{
		setRealContactJid(Jid::null);
		setContactAcceptable(false);
		setErrorMessage(tr("Unable to determine the contact ID: %1").arg(AError));
	}
	else if (FLoginRequests.contains(AId))
	{
		Jid serviceJid = FLoginRequests.take(AId);
		FDisabledGateways.append(serviceJid);
		FEnabledGateways.removeAll(serviceJid);
	}
}
