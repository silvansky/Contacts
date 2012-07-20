#include "addcontactdialog.h"

#include <QSet>
#include <QLabel>
#include <QListView>
#include <QClipboard>
#include <QHBoxLayout>
#include <QPushButton>
#include <QInputDialog>
#include <QApplication>
#include <QTextDocument>
#include <utils/log.h>

#ifdef Q_WS_MAC
# include <utils/macwidgets.h>
#endif

#define GROUP_NEW                ":group_new:"
#define GROUP_EMPTY              ":empty_group:"

enum DialogState {
	STATE_ADDRESS,
	STATE_CONFIRM,
	STATE_PARAMS
};

AddContactDialog::AddContactDialog(IRoster *ARoster, IRosterChanger *ARosterChanger, IPluginManager *APluginManager, QWidget *AParent) : QDialog(AParent)
{
	ui.setupUi(this);
	setAttribute(Qt::WA_DeleteOnClose,true);
	setWindowTitle(tr("New contact"));
	StyleStorage::staticStorage(RSR_STORAGE_STYLESHEETS)->insertAutoStyle(this,STS_RCHANGER_ADDCONTACTDIALOG);

	CustomBorderContainer *border = CustomBorderStorage::staticStorage(RSR_STORAGE_CUSTOMBORDER)->addBorder(this, CBS_DIALOG);
	if (border)
	{
		border->setAttribute(Qt::WA_DeleteOnClose, true);
		border->setMaximizeButtonVisible(false);
		border->setMinimizeButtonVisible(false);
		border->setResizable(false);
		connect(border, SIGNAL(closeClicked()), SLOT(reject()));
		connect(this, SIGNAL(rejected()), border, SLOT(close()));
		connect(this, SIGNAL(accepted()), border, SLOT(close()));
	}
	else
	{
		ui.lblCaption->setVisible(false);
	}

#ifdef Q_WS_MAC
	ui.buttonsLayout->addWidget(ui.pbtContinue);
	ui.buttonsLayout->setSpacing(16);
	setWindowGrowButtonEnabled(this->window(), false);
#endif

	ui.lneAddressContact->setAttribute(Qt::WA_MacShowFocusRect, false);
	ui.lneParamsNick->setAttribute(Qt::WA_MacShowFocusRect, false);

	FGateways = NULL;
	FAvatars = NULL;
	FMetaRoster = NULL;
	FRostersView = NULL;
	FVcardPlugin = NULL;
	FOptionsManager = NULL;
	FMessageProcessor = NULL;

	FRoster = ARoster;
	FRosterChanger = ARosterChanger;

	FDialogState = -1;
	FSelectProfileWidget = NULL;

	ui.cmbParamsGroup->setView(new QListView);
	ui.wdtConfirmAddresses->setLayout(new QVBoxLayout);
	ui.wdtConfirmAddresses->layout()->setMargin(0);
	ui.wdtConfirmAddresses->layout()->setSpacing(10);

	IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->insertAutoIcon(ui.lblErrorIcon,MNI_RCHANGER_ADDCONTACT_ERROR,0,0,"pixmap");

	connect(FRoster->instance(),SIGNAL(itemReceived(const IRosterItem &, const IRosterItem &)),
		SLOT(onRosterItemReceived(const IRosterItem &, const IRosterItem &)));

	connect(ui.cmbParamsGroup,SIGNAL(currentIndexChanged(int)),SLOT(onGroupCurrentIndexChanged(int)));
	connect(ui.lneParamsNick,SIGNAL(textEdited(const QString &)),SLOT(onContactNickEdited(const QString &)));
	connect(ui.lneAddressContact,SIGNAL(textEdited(const QString &)),SLOT(onContactTextEdited(const QString &)));

	connect(ui.pbtBack, SIGNAL(clicked()), SLOT(onBackButtonclicked()));
	connect(ui.pbtContinue, SIGNAL(clicked()), SLOT(onContinueButtonClicked()));
	connect(ui.pbtCancel, SIGNAL(clicked()), SLOT(onCancelButtonClicked()));

	ui.lblError->setVisible(false);
	ui.lblErrorIcon->setVisible(false);

	ui.pbtBack->setText(tr("Back"));
	ui.pbtCancel->setText(tr("Cancel"));
	ui.pbtContinue->setEnabled(false);

	initialize(APluginManager);
	initGroups();

	updatePageAddress();
	setErrorMessage(QString::null,false);
	setDialogState(STATE_ADDRESS);

	QString contact = qApp->clipboard()->text();
	if (FGateways && !FGateways->gateAvailDescriptorsByContact(contact).isEmpty())
	{
		setContactText(contact);
		ui.lneAddressContact->selectAll();
	}

	ui.lneAddressContact->installEventFilter(this);
	ui.lneParamsNick->installEventFilter(this);
	ui.cmbParamsGroup->installEventFilter(this);
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
		QString contact = FGateways!=NULL ? FGateways->legacyIdFromUserJid(streamJid(),AContactJid) : AContactJid.bare();
		setContactText(contact);
		if (FGateways && FGateways->streamServices(streamJid()).contains(AContactJid.domain()))
			setGatewayJid(AContactJid.domain());
		else if (AContactJid.isValid())
			setGatewayJid(streamJid());
		else
			setGatewayJid(Jid::null);
	}
}

QString AddContactDialog::contactText() const
{
	return ui.lneAddressContact->text();
}

void AddContactDialog::setContactText(const QString &AText)
{
	ui.lneAddressContact->setText(AText);
	onContactTextEdited(AText);
	setDialogState(STATE_ADDRESS);
}

QString AddContactDialog::nickName() const
{
	QString nick = ui.lneParamsNick->text().trimmed();
	if (nick.isEmpty())
		nick = defaultContactNick(contactText());
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
	return FSelectProfileWidget!=NULL ? FSelectProfileWidget->selectedProfile() : FGatewayJid;
}

void AddContactDialog::setGatewayJid(const Jid &AGatewayJid)
{
	if (AGatewayJid == streamJid())
	{
		FGatewayJid = AGatewayJid;
	}
	else if (FGateways && FGateways->streamServices(streamJid()).contains(AGatewayJid))
	{
		if (FSelectProfileWidget)
		{
			FSelectProfileWidget->setAutoSelectProfile(false);
			FSelectProfileWidget->setSelectedProfile(AGatewayJid);
		}
		FGatewayJid = AGatewayJid;
	}
	else if (!AGatewayJid.isValid())
	{
		if (FSelectProfileWidget)
			FSelectProfileWidget->setAutoSelectProfile(true);
		FGatewayJid = Jid::null;
	}
}

QString AddContactDialog::parentMetaContactId() const
{
	return FParentMetaId;
}

void AddContactDialog::setParentMetaContactId(const QString &AMetaId)
{
	FParentMetaId = AMetaId;
	ui.lneParamsNick->setVisible(FParentMetaId.isEmpty());
	ui.cmbParamsGroup->setVisible(FParentMetaId.isEmpty());
}

void AddContactDialog::executeRequiredContactChecks()
{
	if (FDialogState==STATE_ADDRESS && ui.pbtContinue->isEnabled())
		QTimer::singleShot(0,this,SLOT(onContinueButtonClicked()));
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
		}
	}

	plugin = APluginManager->pluginInterface("IGateways").value(0,NULL);
	if (plugin)
	{
		FGateways = qobject_cast<IGateways *>(plugin->instance());
		if (FGateways)
		{
			connect(FGateways->instance(),SIGNAL(userJidReceived(const QString &, const Jid &)),SLOT(onLegacyContactJidReceived(const QString &, const Jid &)));
			connect(FGateways->instance(),SIGNAL(errorReceived(const QString &, const QString &)),SLOT(onGatewayErrorReceived(const QString &, const QString &)));
		}
	}

	plugin = APluginManager->pluginInterface("IMetaContacts").value(0,NULL);
	if (plugin)
	{
		IMetaContacts *mcontacts = qobject_cast<IMetaContacts *>(plugin->instance());
		FMetaRoster = mcontacts!=NULL ? mcontacts->findMetaRoster(streamJid()) : NULL;
		if (FMetaRoster)
		{
			connect(FMetaRoster->instance(),SIGNAL(metaActionResult(const QString &, const QString &, const QString &)),
				SLOT(onMetaActionResult(const QString &, const QString &, const QString &)));
		}
	}

	plugin = APluginManager->pluginInterface("IOptionsManager").value(0,NULL);
	if (plugin)
	{
		FOptionsManager = qobject_cast<IOptionsManager *>(plugin->instance());
	}

	plugin = APluginManager->pluginInterface("IMessageProcessor").value(0,NULL);
	if (plugin)
	{
		FMessageProcessor = qobject_cast<IMessageProcessor *>(plugin->instance());
	}

	plugin = APluginManager->pluginInterface("IRostersViewPlugin").value(0,NULL);
	if (plugin)
	{
		IRostersViewPlugin *rostersViewPlugin = qobject_cast<IRostersViewPlugin *>(plugin->instance());
		FRostersView = rostersViewPlugin!=NULL ? rostersViewPlugin->rostersView() : NULL;
	}
}

void AddContactDialog::initGroups()
{
	QList<QString> groups = FRoster->groups().toList();
	qSort(groups);
	ui.cmbParamsGroup->addItem(tr("<Common Group>"),QString(GROUP_EMPTY));
	ui.cmbParamsGroup->addItems(groups);
	ui.cmbParamsGroup->addItem(tr("New Group..."),QString(GROUP_NEW));

	int last = ui.cmbParamsGroup->findText(Options::node(OPV_ROSTER_ADDCONTACTDIALOG_LASTGROUP).value().toString());
	if (last>=0 && last<ui.cmbParamsGroup->count()-1)
		ui.cmbParamsGroup->setCurrentIndex(last);
}

void AddContactDialog::selectRosterIndex() const
{
	if (FRostersView)
	{
		IRostersModel *rmodel = FRostersView->rostersModel();
		IRosterIndex *sroot = rmodel!=NULL ? rmodel->streamRoot(streamJid()) : NULL;
		if (sroot)
		{
			QMultiMap<int, QVariant> findData;
			if (FMetaRoster!=NULL && FMetaRoster->isEnabled())
			{
				findData.insert(RDR_TYPE,RIT_METACONTACT);
				findData.insert(RDR_META_ID,FMetaRoster->itemMetaContact(contactJid()));
			}
			else
			{
				findData.insert(RDR_TYPE,RIT_CONTACT);
				findData.insert(RDR_PREP_BARE_JID,contactJid().pBare());
			}

			IRosterIndex *index = sroot->findChilds(findData,true).value(0);
			if (index)
			{
				QModelIndex modelIndex = FRostersView->mapFromModel(rmodel->modelIndexByRosterIndex(index));
				FRostersView->instance()->clearSelection();
				FRostersView->instance()->scrollTo(modelIndex);
				FRostersView->instance()->setCurrentIndex(modelIndex);
				FRostersView->instance()->selectionModel()->select(modelIndex,QItemSelectionModel::Select);
			}
		}
	}
}

void AddContactDialog::showChatDialogAndAccept()
{
	selectRosterIndex();
	if (FMessageProcessor)
		FMessageProcessor->createMessageWindow(streamJid(),contactJid(),Message::Chat,IMessageHandler::SM_SHOW);
	accept();
}

bool AddContactDialog::isContactPresentInRoster() const
{
	if (contactJid().isValid())
	{
		IRosterItem ritem = FRoster->rosterItem(contactJid());
		if (ritem.isValid && !FLinkedContacts.isEmpty())
		{
			if (FParentMetaId.isEmpty() || FParentMetaId==FContactMetaId)
				return false;
		}
		return ritem.isValid;
	}
	return false;
}

QString AddContactDialog::defaultContactNick(const Jid &AContactJid) const
{
	QString nick = AContactJid.node();
	nick = nick.isEmpty() ? AContactJid.domain() : nick;
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

QString AddContactDialog::confirmDescriptorText(const IGateServiceDescriptor &ADescriptor)
{
	QString text;
	if (ADescriptor.id == GSID_ICQ)
		text = tr("This is an ICQ number");
	else if (ADescriptor.id == GSID_SMS)
		text = tr("This is a phone number");
	else if (ADescriptor.id == GSID_MAIL)
		text = tr("This is a e-mail address");
	else
		text = tr("This is a %1 address").arg(ADescriptor.name);
	return text;
}

bool AddContactDialog::acceptDescriptor(const IGateServiceDescriptor &ADescriptor)
{
	if (FGateways)
	{
		switch (FGateways->gateDescriptorStatus(streamJid(),ADescriptor))
		{
		case IGateways::GDS_UNREGISTERED:
			{
				QDialog *dialog = FGateways->showAddLegacyAccountDialog(streamJid(),FGateways->gateDescriptorRegistrator(streamJid(),ADescriptor));
				return dialog->exec()==QDialog::Accepted ? true : false;
			}
			break;
		case IGateways::GDS_UNAVAILABLE:
			setErrorMessage(tr("%1 service is temporarily unavailable, please try later.").arg(ADescriptor.name),false);
			return false;
		default:
			return true;
		}
	}
	return false;
}

void AddContactDialog::updatePageAddress()
{
	setResolveNickState(false);
	setNickName(QString::null);
	setRealContactJid(Jid::null);
}

void AddContactDialog::updatePageConfirm(const QList<IGateServiceDescriptor> &ADescriptors)
{
	qDeleteAll(FConfirmButtons.keys());
	FConfirmButtons.clear();
	ui.lblConfirmInfo->setText(tr("Refine entered address: <b>%1</b>").arg(Qt::escape(contactText())));
	for(int index=0; index<ADescriptors.count(); index++)
	{
		const IGateServiceDescriptor &descriptor = ADescriptors.at(index);
		QRadioButton *button = new QRadioButton(ui.wdtConfirmAddresses);
		button->setText(confirmDescriptorText(descriptor));
		button->setAutoExclusive(true);
		button->setChecked(index == 0);
		FConfirmButtons.insert(button,descriptor);
		ui.wdtConfirmAddresses->layout()->addWidget(button);
	}
}

void AddContactDialog::updatePageParams(const IGateServiceDescriptor &ADescriptor)
{
	FDescriptor = ADescriptor;

	IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->insertAutoIcon(ui.lblParamsServiceIcon,FDescriptor.iconKey,0,0,"pixmap");
	ui.lblParamsContact->setText(FGateways!=NULL ? FGateways->formattedContactLogin(FDescriptor,contactText()) : contactText());

	if (FGateways)
	{
		delete FSelectProfileWidget;
		FSelectProfileWidget = new SelectProfileWidget(FRoster,FGateways,FOptionsManager,FDescriptor,ui.wdtSelectProfile);
		connect(FSelectProfileWidget,SIGNAL(profilesChanged()),SLOT(onSelectedProfileChanched()));
		connect(FSelectProfileWidget,SIGNAL(selectedProfileChanged()),SLOT(onSelectedProfileChanched()));
		ui.wdtSelectProfile->layout()->addWidget(FSelectProfileWidget);

		FSelectProfileWidget->setAutoSelectProfile(FGatewayJid.isEmpty());
		FSelectProfileWidget->setSelectedProfile(FGatewayJid);
	}
}

void AddContactDialog::setDialogState(int AState)
{
	if (AState != FDialogState)
	{
		FDialogState = AState;
		if (AState == STATE_ADDRESS)
		{
			ui.wdtPageAddress->setVisible(true);
			ui.wdtPageConfirm->setVisible(false);
			ui.wdtPageParams->setVisible(false);
			ui.wdtSelectProfile->setVisible(false);
			ui.pbtBack->setVisible(false);
			ui.pbtContinue->setText(tr("Continue"));
			ui.pbtContinue->setEnabled(!ui.lneAddressContact->text().isEmpty());
		}
		else if (AState == STATE_CONFIRM)
		{
			ui.wdtPageAddress->setVisible(false);
			ui.wdtPageConfirm->setVisible(true);
			ui.wdtPageParams->setVisible(false);
			ui.wdtSelectProfile->setVisible(false);
			ui.pbtBack->setVisible(true);
			ui.pbtContinue->setText(tr("Continue"));
			ui.pbtContinue->setEnabled(true);
		}
		else if (AState == STATE_PARAMS)
		{
			ui.wdtPageAddress->setVisible(false);
			ui.wdtPageConfirm->setVisible(false);
			ui.wdtPageParams->setVisible(true);
			ui.wdtSelectProfile->setVisible(FGatewayJid.isEmpty());
			ui.pbtBack->setVisible(true);
			resolveLinkedContactsJid();
			resolveContactJid();
		}
	}
}

void AddContactDialog::setDialogEnabled(bool AEnabled)
{
	ui.wdtPageAddress->setEnabled(AEnabled);
	ui.wdtPageConfirm->setEnabled(AEnabled);
	ui.wdtPageParams->setEnabled(AEnabled);
	ui.wdtSelectProfile->setEnabled(AEnabled);
	ui.pbtBack->setEnabled(AEnabled);
	ui.pbtContinue->setEnabled(AEnabled);
}

void AddContactDialog::setRealContactJid(const Jid &AContactJid)
{
	if (FAvatars)
		FAvatars->insertAutoAvatar(ui.lblParamsPhoto,AContactJid,QSize(48, 48),"pixmap");
	FContactJid = AContactJid.bare();
	FContactMetaId = FMetaRoster!=NULL ? FMetaRoster->itemMetaContact(FContactJid) : QString::null;
}

void AddContactDialog::setResolveNickState(bool AResolve)
{
	if (AResolve && ui.lneParamsNick->text().isEmpty())
	{
		setNickName(defaultContactNick(contactText()));
		ui.lneParamsNick->setFocus();
		ui.lneParamsNick->selectAll();
		FResolveNick = true;
	}
	else
	{
		FResolveNick = false;
	}
}

void AddContactDialog::setErrorMessage(const QString &AMessage, bool AInvalidInput)
{
	if (ui.lblError->text() != AMessage)
	{
		ui.lblError->setText(AMessage);
		ui.lblError->setVisible(!AMessage.isEmpty());
		ui.lblErrorIcon->setVisible(!AMessage.isEmpty());
		ui.lneAddressContact->setProperty("error", !AMessage.isEmpty() && AInvalidInput ? true : false);
		StyleStorage::updateStyle(this);
	}
}

void AddContactDialog::resolveDescriptor()
{
	QList<QString> confirmTypes;
	QList<QString> confirmLinked;
	QList<QString> confirmBlocked;
	IGateServiceDescriptor readOnlyDescriptor;
	QList<IGateServiceDescriptor> confirmDescriptors;

	foreach(const IGateServiceDescriptor &descriptor, FGateways!=NULL ? FGateways->gateHomeDescriptorsByContact(contactText()) : confirmDescriptors)
	{
		if (!confirmTypes.contains(descriptor.type) && !confirmLinked.contains(descriptor.id) && !confirmBlocked.contains(descriptor.id))
		{
			if (!(FGateways->gateDescriptorRestrictions(streamJid(),descriptor) & GSR_ADD_CONTACT))
				confirmDescriptors.append(descriptor);
			else
				readOnlyDescriptor = descriptor;
			confirmTypes += descriptor.type;
			confirmLinked += descriptor.linkedDescriptors;
			confirmBlocked += descriptor.blockedDescriptors;
		}
	}

	IGateServiceDescriptor gateDescriptor = FGateways!=NULL && FGatewayJid.isValid() ? FGateways->serviceDescriptor(streamJid(),FGatewayJid) : IGateServiceDescriptor();
	for (QList<IGateServiceDescriptor>::iterator it=confirmDescriptors.begin(); it!=confirmDescriptors.end(); )
	{
		if (!gateDescriptor.id.isEmpty() && it->type!=gateDescriptor.type)
			it = confirmDescriptors.erase(it);
		else if (confirmLinked.contains(it->id) || confirmBlocked.contains(it->id))
			it = confirmDescriptors.erase(it);
		else
			it++;
	}

	if (confirmDescriptors.count() > 1)
	{
		updatePageConfirm(confirmDescriptors);
		setDialogState(STATE_CONFIRM);
	}
	else if (!confirmDescriptors.isEmpty())
	{
		IGateServiceDescriptor descriptor = confirmDescriptors.value(0);
		if (acceptDescriptor(descriptor))
		{
			updatePageParams(descriptor);
			setDialogState(STATE_PARAMS);
		}
	}
	else if (!readOnlyDescriptor.id.isEmpty())
	{
		if (readOnlyDescriptor.id == GSID_ICQ)
			setErrorMessage(tr("Currently you can’t add %1 contacts.").arg(readOnlyDescriptor.name),false);
		else
			setErrorMessage(tr("You can add this contact only on %1 site.").arg(readOnlyDescriptor.name),false);
	}
	else
	{
		setErrorMessage(tr("Could not find such address."),true);
	}
}

void AddContactDialog::resolveContactJid()
{
	QString errMessage;
	bool nextResolve = false;

	setRealContactJid(Jid::null);
	ui.pbtContinue->setText(tr("Add Contact"));
	QString contact = FGateways!=NULL ? FGateways->normalizedContactLogin(FDescriptor,contactText()) : contactText().trimmed();

	Jid gateJid = gatewayJid();
	if (gateJid == streamJid())
	{
		nextResolve = true;
		setRealContactJid(contact);
	}
	else if (FGateways && gateJid.isValid())
	{
		FContactJidRequest = FGateways->sendUserJidRequest(streamJid(),gateJid,contact);
		if (FContactJidRequest.isEmpty())
			errMessage = tr("Failed to request contact JID from transport.");
	}
	else if (FSelectProfileWidget->profiles().isEmpty())
	{
		errMessage = tr("Service '%1' is not available now.").arg(FDescriptor.name);
	}
	else
	{
		errMessage = tr("Select a contact-list in which you want to add a contact.");
	}

	setErrorMessage(errMessage,false);

	if (nextResolve)
		resolveContactName();
	else
		ui.pbtContinue->setEnabled(false);
}

void AddContactDialog::resolveContactName()
{
	if (FVcardPlugin && contactJid().isValid())
	{
		FVcardPlugin->requestVCard(streamJid(), contactJid());
		setResolveNickState(true);
	}
	resolveReady();
}

void AddContactDialog::resolveLinkedContactsJid()
{
	FLinkedContacts.clear();
	FLinkedJidRequests.clear();

	if (FGateways)
	{
		foreach(QString descriptorId, FDescriptor.linkedDescriptors)
		{
			IGateServiceDescriptor descriptor = FGateways->gateDescriptorById(descriptorId);
			if (FGateways->gateDescriptorStatus(streamJid(),descriptor) == IGateways::GDS_ENABLED)
			{
				QString contact = FGateways->normalizedContactLogin(FDescriptor,contactText());
				if (descriptor.needGate)
				{
					IDiscoIdentity identity;
					identity.category = "gateway";
					identity.type = descriptor.type;
					QList<Jid> gates = FGateways->streamServices(streamJid(),identity);
					foreach(Jid gate, gates)
					{
						const IGateServiceDescriptor &descriptor = FGateways->serviceDescriptor(streamJid(),gate);
						if (!(FGateways->gateDescriptorRestrictions(streamJid(),descriptor) & GSR_ADD_CONTACT))
						{
							QString requestId = FGateways->sendUserJidRequest(streamJid(),gate,contact);
							if (!requestId.isEmpty())
							{
								FLinkedJidRequests.insert(requestId,gate);
								break;
							}
						}
					}
				}
				else if (!FLinkedContacts.contains(contact) && !FRoster->rosterItem(contact).isValid)
				{
					FLinkedContacts.append(contact);
				}
			}
		}
	}
}

void AddContactDialog::resolveReady()
{
	if (FDialogState == STATE_PARAMS)
	{
		if (FLinkedJidRequests.isEmpty() && isContactPresentInRoster())
		{
			IRosterItem ritem = FRoster->rosterItem(contactJid());
			ui.pbtContinue->setText(tr("Open"));
			setNickName(!ritem.name.isEmpty() ? ritem.name : defaultContactNick(contactText()));
			setGroup(ritem.groups.toList().value(0));
			setErrorMessage(tr("This contact is already present in your contact-list."),false);
		}
		ui.pbtContinue->setEnabled(contactJid().isValid() && FLinkedJidRequests.isEmpty());
	}
}

bool AddContactDialog::event(QEvent *AEvent)
{
	if (AEvent->type() == QEvent::LayoutRequest)
		QTimer::singleShot(0,this,SLOT(onAdjustDialogSize()));
	return QDialog::event(AEvent);
}

void AddContactDialog::onBackButtonclicked()
{
	setErrorMessage(QString::null,false);
	updatePageAddress();
	setDialogState(STATE_ADDRESS);
}

void AddContactDialog::onContinueButtonClicked()
{
	if (FDialogState == STATE_ADDRESS)
	{
		resolveDescriptor();
	}
	else if (FDialogState == STATE_CONFIRM)
	{
		for (QMap<QRadioButton *, IGateServiceDescriptor>::const_iterator it=FConfirmButtons.constBegin(); it!=FConfirmButtons.constEnd(); it++)
		{
			if (it.key()->isChecked())
			{
				if (acceptDescriptor(it.value()))
				{
					updatePageParams(it.value());
					setDialogState(STATE_PARAMS);
				}
				break;
			}
		}
	}
	else if (FDialogState == STATE_PARAMS)
	{
		if (contactJid().isValid())
		{
			if (isContactPresentInRoster())
			{
				showChatDialogAndAccept();
			}
			else if (FMetaRoster && FMetaRoster->isEnabled())
			{
				IMetaContact contact;
				contact.name = nickName();
				contact.groups += group();
				contact.items = FLinkedContacts.toSet();

				if (FContactMetaId.isEmpty())
					contact.items += contactJid();

				FContactCreateRequest = FMetaRoster->createContact(contact);
				if (!FContactCreateRequest.isEmpty())
				{
					foreach(Jid itemJid, contact.items)
						FRosterChanger->subscribeContact(streamJid(),itemJid,QString::null,true);
					setDialogEnabled(false);
				}
				else
				{
					FContactCreateRequest = "createError";
					onMetaActionResult(FContactCreateRequest,ErrorHandler::conditionByCode(ErrorHandler::INTERNAL_SERVER_ERROR),tr("Failed to send request to the server"));
				}
			}
			else
			{
				foreach(Jid linkedJid, FLinkedContacts)
				{
					if (linkedJid != contactJid())
					{
						FRoster->setItem(linkedJid,nickName(),QSet<QString>()<<group());
						FRosterChanger->subscribeContact(streamJid(),linkedJid,QString::null);
					}
				}
				FRoster->setItem(contactJid(),nickName(),QSet<QString>()<<group());
				FRosterChanger->subscribeContact(streamJid(),contactJid(),QString::null);
			}
		}
	}
}

void AddContactDialog::onCancelButtonClicked()
{
	reject();
}

void AddContactDialog::onAdjustDialogSize()
{
	if (parentWidget())
		parentWidget()->adjustSize();
	else
		adjustSize();
}

void AddContactDialog::onContactTextEdited(const QString &AText)
{
	setErrorMessage(QString::null,false);
	ui.pbtContinue->setEnabled(!AText.isEmpty());
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
		CustomInputDialog *dialog = new CustomInputDialog(CustomInputDialog::String);
		dialog->setCaptionText(tr("Create new group"));
		dialog->setInfoText(tr("Enter group name:"));
		dialog->setAcceptButtonText(tr("Create"));
		dialog->setRejectButtonText(tr("Cancel"));
		connect(dialog, SIGNAL(stringAccepted(const QString&)), SLOT(onNewGroupNameSelected(const QString&)));
		dialog->show();
		ui.cmbParamsGroup->setCurrentIndex(0);
	}
}

void AddContactDialog::onNewGroupNameSelected(const QString &AGroup)
{
	if (!AGroup.isEmpty())
	{
		int index = ui.cmbParamsGroup->findText(AGroup);
		if (index < 0)
		{
			ui.cmbParamsGroup->blockSignals(true);
			ui.cmbParamsGroup->insertItem(1,AGroup);
			ui.cmbParamsGroup->blockSignals(false);
			index = 1;
		}
		ui.cmbParamsGroup->setCurrentIndex(index);
	}
}

void AddContactDialog::onSelectedProfileChanched()
{
	if (FDialogState == STATE_PARAMS)
		resolveContactJid();
}

void AddContactDialog::onVCardReceived(const Jid &AContactJid)
{
	if (AContactJid && contactJid())
	{
		if (FResolveNick)
		{
			IVCard *vcard = FVcardPlugin->vcard(contactJid());
			QString nick = vcard->value(VVN_NICKNAME);
			vcard->unlock();
			setResolveNickState(false);
			setNickName(nick.isEmpty() ? defaultContactNick(contactText()) : nick);
			ui.lneParamsNick->selectAll();
		}
	}
}

void AddContactDialog::onLegacyContactJidReceived(const QString &AId, const Jid &AUserJid)
{
	if (FContactJidRequest == AId)
	{
		if (FDialogState == STATE_PARAMS)
		{
			setRealContactJid(AUserJid);
			resolveContactName();
		}
	}
	else if (FLinkedJidRequests.contains(AId))
	{
		if (!FRoster->rosterItem(AUserJid).isValid)
			FLinkedContacts.append(AUserJid);
		FLinkedJidRequests.remove(AId);
		resolveReady();
	}
}

void AddContactDialog::onGatewayErrorReceived(const QString &AId, const QString &AError)
{
	Q_UNUSED(AError);
	if (FContactJidRequest == AId)
	{
		if (FDialogState == STATE_PARAMS)
			setErrorMessage(tr("Failed to request contact JID from transport."),false);
	}
	else if (FLinkedJidRequests.contains(AId))
	{
		FLinkedJidRequests.remove(AId);
		resolveReady();
	}
}

void AddContactDialog::onRosterItemReceived(const IRosterItem &AItem, const IRosterItem &ABefore)
{
	Q_UNUSED(ABefore);
	if (AItem.itemJid == contactJid())
	{
		if (FMetaRoster==NULL || !FMetaRoster->isEnabled())
			showChatDialogAndAccept();
	}
}

void AddContactDialog::onMetaActionResult(const QString &AActionId, const QString &AErrCond, const QString &AErrMessage)
{
	Q_UNUSED(AErrCond);
	if (FContactCreateRequest == AActionId)
	{
		QString metaId = FMetaRoster->itemMetaContact(FContactMetaId.isEmpty() ? contactJid() : FLinkedContacts.value(0));
		if (!metaId.isEmpty())
		{
			if (!FContactMetaId.isEmpty())
				FContactMergeRequest = FMetaRoster->mergeContacts(FContactMetaId,QList<QString>()<<metaId);
			else if (!FParentMetaId.isEmpty() && !FMetaRoster->metaContact(FParentMetaId).id.isEmpty())
				FContactMergeRequest = FMetaRoster->mergeContacts(FParentMetaId,QList<QString>()<<metaId);
			else
				FContactMergeRequest = QString::null;

			if (FContactMergeRequest.isEmpty())
				showChatDialogAndAccept();
		}
		else
		{
			setErrorMessage(tr("Failed to add contact due to an error: %1").arg(AErrMessage),false);
			setDialogEnabled(true);
		}
	}
	else if (FContactMergeRequest == AActionId)
	{
		showChatDialogAndAccept();
	}
}
