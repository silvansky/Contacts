#include "addcontactdialog.h"

#include <QSet>
#include <QLabel>
#include <QClipboard>
#include <QHBoxLayout>
#include <QPushButton>
#include <QMessageBox>
#include <QInputDialog>
#include <QApplication>
#include <QListView>

#define GROUP_NEW                ":group_new:"
#define GROUP_EMPTY              ":empty_group:"

#define URL_ACTION_ADD_ACCOUNT   "AddAction"

#define RESOLVE_WAIT_INTERVAL    1000

enum DialogState {
	STATE_ADDRESS,
	STATE_CONFIRM,
	STATE_PARAMS
};

AddContactDialog::AddContactDialog(IRoster *ARoster, IRosterChanger *ARosterChanger, IPluginManager *APluginManager, QWidget *AParent) : QDialog(AParent)
{
	ui.setupUi(this);
	setAttribute(Qt::WA_DeleteOnClose,true);
	setWindowTitle(tr("Adding a contact"));
	IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->insertAutoIcon(this,MNI_RCHANGER_ADD_CONTACT,0,0,"windowIcon");
	StyleStorage::staticStorage(RSR_STORAGE_STYLESHEETS)->insertAutoStyle(this,STS_RCHANGER_ADDCONTACTDIALOG);

	ui.lneAddressContact->setAttribute(Qt::WA_MacShowFocusRect, false);
	ui.lneParamsNick->setAttribute(Qt::WA_MacShowFocusRect, false);

	ui.cmbParamsGroup->setView(new CustomListView);

	FRoster = NULL;
	FAvatars = NULL;
	FVcardPlugin = NULL;
	FRoster = ARoster;
	FRosterChanger = ARosterChanger;

	FShown = false;
	FDialogState = -1;

	IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->insertAutoIcon(ui.lblAddressError,MNI_RCHANGER_ADDCONTACT_ERROR,0,0,"pixmap");
	
	FResolveTimer.setSingleShot(true);
	connect(&FResolveTimer,SIGNAL(timeout()),SLOT(resolveServiceJid()));

	connect(ui.cmbParamsGroup,SIGNAL(currentIndexChanged(int)),SLOT(onGroupCurrentIndexChanged(int)));
	connect(ui.lneParamsNick,SIGNAL(textEdited(const QString &)),SLOT(onContactNickEdited(const QString &)));
	connect(ui.lneAddressContact,SIGNAL(textEdited(const QString &)),SLOT(onContactTextEdited(const QString &)));

	connect(ui.dbbButtons,SIGNAL(accepted()),SLOT(onDialogAccepted()));
	connect(ui.dbbButtons,SIGNAL(rejected()),SLOT(reject()));

	initialize(APluginManager);
	initGroups();
	setDialogState(STATE_ADDRESS);

	QString contact = qApp->clipboard()->text();
	Jid userJid = contact;
	if (FGateways && !FGateways->gateAvailDescriptorsByContact(contact).isEmpty())
		setContactText(contact);
	else if (userJid.isValid() && !userJid.node().isEmpty())
		setContactJid(userJid.bare());
}

AddContactDialog::~AddContactDialog()
{
	emit dialogDestroyed();
}

Jid AddContactDialog::streamJid() const
{
	return FRoster->streamJid();
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
		if (FGateways && FGateways->streamServices(streamJid()).contains(AContactJid.domain()))
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
	return ui.lneAddressContact->text();
}

void AddContactDialog::setContactText(const QString &AText)
{
	ui.lneAddressContact->setText(AText);
}

QString AddContactDialog::nickName() const
{
	QString nick = ui.lneParamsNick->text().trimmed();
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
	ui.lneParamsNick->setText(ANick);
}

QString AddContactDialog::group() const
{
	return ui.cmbParamsGroup->itemData(ui.cmbParamsGroup->currentIndex()).isNull() ? ui.cmbParamsGroup->currentText() : QString::null;
}

void AddContactDialog::setGroup(const QString &AGroup)
{
	int index = ui.cmbParamsGroup->findText(AGroup);
	if (AGroup.isEmpty())
		ui.cmbParamsGroup->setCurrentIndex(0);
	else if (index < 0)
		ui.cmbParamsGroup->insertItem(ui.cmbParamsGroup->count()-1,AGroup);
	else if (index > 0)
		ui.cmbParamsGroup->setCurrentIndex(index);
}

Jid AddContactDialog::gatewayJid() const
{
	return Jid::null;
}

void AddContactDialog::setGatewayJid(const Jid &AGatewayJid)
{

}

void AddContactDialog::initialize(IPluginManager *APluginManager)
{
	IPlugin *plugin = APluginManager->pluginInterface("IAvatars").value(0,NULL);
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
	ui.cmbParamsGroup->addItem(tr("<Common Group>"),QString(GROUP_EMPTY));
	ui.cmbParamsGroup->addItems(groups);
	ui.cmbParamsGroup->addItem(tr("New Group..."),QString(GROUP_NEW));

	int last = ui.cmbParamsGroup->findText(Options::node(OPV_ROSTER_ADDCONTACTDIALOG_LASTGROUP).value().toString());
	if (last>=0 && last<ui.cmbParamsGroup->count()-1)
		ui.cmbParamsGroup->setCurrentIndex(last);
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
	QList<Jid> gates = FGateways!=NULL ? FGateways->availServices(streamJid(),identity) : QList<Jid>();

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

void AddContactDialog::setDialogState(int AState)
{
	if (AState != FDialogState)
	{
		if (AState == STATE_ADDRESS)
		{
			setRealContactJid(Jid::null);
			setResolveNickState(false);
			setErrorMessage(QString::null);
			ui.wdtPageAddress->setVisible(true);
			ui.wdtPageConfirm->setVisible(false);
			ui.wdtPageParams->setVisible(false);
			ui.dbbButtons->button(QDialogButtonBox::Ok)->setText(tr("Continue"));
		}
		else if (AState == STATE_CONFIRM)
		{
			ui.wdtPageAddress->setVisible(false);
			ui.wdtPageConfirm->setVisible(true);
			ui.wdtPageParams->setVisible(false);
			ui.dbbButtons->button(QDialogButtonBox::Ok)->setText(tr("Continue"));
		}
		else if (AState == STATE_PARAMS)
		{
			ui.wdtPageAddress->setVisible(false);
			ui.wdtPageConfirm->setVisible(true);
			ui.wdtPageParams->setVisible(false);
			ui.dbbButtons->button(QDialogButtonBox::Ok)->setText(tr("Add Contact"));
		}
		QTimer::singleShot(1,this,SLOT(onAdjustDialogSize()));
	}
}

void AddContactDialog::startResolve(int ATimeout)
{
	setRealContactJid(Jid::null);
	setGatewaysEnabled(false);
	setResolveNickState(false);
	setErrorMessage(QString::null);
	FResolveTimer.start(ATimeout);
}

void AddContactDialog::setErrorMessage(const QString &AMessage)
{
	if (AMessage.isEmpty() && ui.lblAddressError->isVisible())
		QTimer::singleShot(1,this,SLOT(onAdjustDialogSize()));
	else if (!AMessage.isEmpty() && !ui.lblAddressError->isVisible())
		QTimer::singleShot(1,this,SLOT(onAdjustDialogSize()));

	ui.lblAddressError->setText(AMessage);
	ui.lblAddressError->setVisible(!AMessage.isEmpty());
	ui.lblAddressErrorIcon->setVisible(!AMessage.isEmpty());
}

void AddContactDialog::setGatewaysEnabled(bool AEnabled)
{

}

void AddContactDialog::setRealContactJid(const Jid &AContactJid)
{
	if (FAvatars)
		FAvatars->insertAutoAvatar(ui.lblParamsPhoto,AContactJid,QSize(50,50),"pixmap");
	FContactJid = AContactJid.bare();
}

void AddContactDialog::setResolveNickState(bool AResolve)
{
	if (AResolve)
		ui.lneParamsNick->setText(QString::null);
	FResolveNick = AResolve;
}

void AddContactDialog::resolveDescriptor()
{
	QList<QString> confirmTypes;
	QList<IGateServiceDescriptor> confirmDescriptors;
	QList<IGateServiceDescriptor> unavailDescriptors;
	foreach(const IGateServiceDescriptor &descriptor, FGateways!=NULL ? FGateways->gateHomeDescriptorsByContact(contactText()) : confirmDescriptors)
	{
		if (!confirmTypes.contains(descriptor.type))
		{
			if (FGateways->gateDescriptorStatus(streamJid(),descriptor)!=IGateways::GDS_UNAVAILABLE)
			{
				confirmTypes.append(descriptor.type);
				confirmDescriptors.append(descriptor);
			}
			else
			{
				unavailDescriptors.append(descriptor);
			}
		}
	}

	QString errMessage;
	if (confirmDescriptors.count() > 1)
	{
		FConfirmDescriptors = confirmDescriptors;
		setDialogState(STATE_CONFIRM);
	}
	else if (!confirmDescriptors.isEmpty())
	{
		FDescriptor = confirmDescriptors.value(0);
		setDialogState(STATE_PARAMS);
	}
	else if (!unavailDescriptors.isEmpty())
	{
		setErrorMessage(tr("Service '%1' is not available now.").arg(unavailDescriptors.value(0).name));
	}
	else
	{
		setErrorMessage(tr("Invalid address. Please check the address and try again."));
	}
}

void AddContactDialog::resolveServiceJid()
{
	QString errMessage;
	bool nextResolve = false;

	QString contact = normalContactText(contactText());
	if (ui.lneAddressContact->text() != contact)
		ui.lneAddressContact->setText(contact);

	IGateServiceDescriptor descriptor = FGateways!=NULL ? FGateways->gateHomeDescriptorsByContact(contact).value(0) : IGateServiceDescriptor();
	if (!descriptor.id.isEmpty())
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
			QList<Jid> freeServices = (FGateways->availServices(streamJid(),identity).toSet() - FGateways->streamServices(streamJid(),identity).toSet()).toList();
			if (!freeServices.isEmpty())
			{
				//actionUrl.setScheme(URL_ACTION_ADD_ACCOUNT);
				//actionUrl.setPath(freeServices.first().pDomain());
				//actionMessage = tr("Enter your %1 account").arg(descriptor.name);
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
	setGatewaysEnabled(!contact.isEmpty() && errMessage.isEmpty());

	if (nextResolve)
		resolveContactJid();
}

void AddContactDialog::resolveContactJid()
{
	QString errMessage;
	bool nextResolve = false;

	QString contact = normalContactText(contactText());
	Jid userJid = contact;

	Jid gateJid = gatewayJid();
	QList<Jid> gateways = suitableServices(FGateways!=NULL ? FGateways->gateAvailDescriptorsByContact(contact) : QList<IGateServiceDescriptor>());
	if (!gateJid.isEmpty() && !gateways.contains(gateJid))
	{
		errMessage = tr("You cannot add this contact to selected profile");
	}
	else if (FGateways && FEnabledGateways.contains(gateJid))
	{
		FContactJidRequest = FGateways->sendUserJidRequest(streamJid(),gateJid,contact);
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

	if (nextResolve)
		resolveContactName();
}

void AddContactDialog::resolveContactName()
{
	QString infoMessage;
	if (FContactJid.isValid())
	{
		IRosterItem ritem = FRoster!=NULL ? FRoster->rosterItem(FContactJid) : IRosterItem();
		if (!ritem.isValid)
		{
			if (FVcardPlugin && FVcardPlugin->requestVCard(streamJid(), FContactJid))
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
	if (FDialogState == STATE_ADDRESS)
	{

	}
	else if (FDialogState == STATE_CONFIRM)
	{

	}
	else if (FDialogState == STATE_PARAMS)
	{
		if (FRoster && FContactJid.isValid())
		{
			IRosterItem ritem = FRoster->rosterItem(FContactJid);
			FRoster->setItem(contactJid(),nickName(),QSet<QString>()<<group());
			FRosterChanger->subscribeContact(streamJid(),contactJid(),QString::null);
			accept();
		}
	}
}

void AddContactDialog::onAdjustDialogSize()
{
	adjustSize();
}

void AddContactDialog::onContactTextEdited(const QString &AText)
{
	ui.dbbButtons->button(QDialogButtonBox::Ok)->setEnabled(!AText.isEmpty());
}

void AddContactDialog::onContactNickEdited(const QString &AText)
{
	Q_UNUSED(AText);
	setResolveNickState(false);
}

void AddContactDialog::onGroupCurrentIndexChanged(int AIndex)
{
	if (ui.cmbParamsGroup->itemData(AIndex).toString() == GROUP_NEW)
	{
		int index = 0;
		QString newGroup = QInputDialog::getText(this,tr("Create group"),tr("New group name")).trimmed();
		if (!newGroup.isEmpty())
		{
			index = ui.cmbParamsGroup->findText(newGroup);
			if (index < 0)
			{
				ui.cmbParamsGroup->blockSignals(true);
				ui.cmbParamsGroup->insertItem(1,newGroup);
				ui.cmbParamsGroup->blockSignals(false);
				index = 1;
			}
			else if (!ui.cmbParamsGroup->itemData(AIndex).isNull())
			{
				index = 0;
			}
		}
		ui.cmbParamsGroup->setCurrentIndex(index);
	}
}

void AddContactDialog::onProfileCurrentIndexChanged(int AIndex)
{
	Q_UNUSED(AIndex);
	resolveContactJid();
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
		FEnabledGateways.append(serviceJid);
		FDisabledGateways.removeAll(serviceJid);
	}
}

void AddContactDialog::onLegacyContactJidReceived(const QString &AId, const Jid &AUserJid)
{
	if (FContactJidRequest == AId)
	{
		setRealContactJid(AUserJid);
		resolveContactName();
	}
}

void AddContactDialog::onServiceEnableChanged(const Jid &AStreamJid, const Jid &AServiceJid, bool AEnabled)
{
	Q_UNUSED(AEnabled);
	if (AStreamJid == streamJid())
	{
		FDisabledGateways.removeAll(AServiceJid);
	}
}

void AddContactDialog::onGatewayErrorReceived(const QString &AId, const QString &AError)
{
	if (FContactJidRequest == AId)
	{
		setRealContactJid(Jid::null);
		setErrorMessage(tr("Unable to determine the contact ID: %1").arg(AError));
	}
	else if (FLoginRequests.contains(AId))
	{
		Jid serviceJid = FLoginRequests.take(AId);
		FDisabledGateways.append(serviceJid);
		FEnabledGateways.removeAll(serviceJid);
	}
}
