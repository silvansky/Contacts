#include "addcontactdialog.h"

#include <QSet>
#include <QLabel>
#include <QListView>
#include <QClipboard>
#include <QHBoxLayout>
#include <QPushButton>
#include <QMessageBox>
#include <QInputDialog>
#include <QApplication>
#include <QTextDocument>

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
	setWindowTitle(tr("Adding a contact"));
	IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->insertAutoIcon(this,MNI_RCHANGER_ADD_CONTACT,0,0,"windowIcon");
	StyleStorage::staticStorage(RSR_STORAGE_STYLESHEETS)->insertAutoStyle(this,STS_RCHANGER_ADDCONTACTDIALOG);

	ui.lneAddressContact->setAttribute(Qt::WA_MacShowFocusRect, false);
	ui.lneParamsNick->setAttribute(Qt::WA_MacShowFocusRect, false);

	FGateways = NULL;
	FAvatars = NULL;
	FMetaRoster = NULL;
	FVcardPlugin = NULL;
	FOptionsManager = NULL;
	FMessageProcessor = NULL;

	FRoster = ARoster;
	FRosterChanger = ARosterChanger;

	FShown = false;
	FServiceFailed = false;
	FDialogState = -1;
	FSelectProfileWidget = NULL;

	ui.cmbParamsGroup->setView(new QListView);
	ui.wdtConfirmAddresses->setLayout(new QVBoxLayout);
	ui.wdtConfirmAddresses->layout()->setMargin(0);

	IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->insertAutoIcon(ui.lblErrorIcon,MNI_RCHANGER_ADDCONTACT_ERROR,0,0,"pixmap");
	
	connect(FRoster->instance(),SIGNAL(received(const IRosterItem &, const IRosterItem &)),
		SLOT(onRosterItemReceived(const IRosterItem &, const IRosterItem &)));

	connect(ui.cmbParamsGroup,SIGNAL(currentIndexChanged(int)),SLOT(onGroupCurrentIndexChanged(int)));
	connect(ui.lneParamsNick,SIGNAL(textEdited(const QString &)),SLOT(onContactNickEdited(const QString &)));
	connect(ui.lneAddressContact,SIGNAL(textEdited(const QString &)),SLOT(onContactTextEdited(const QString &)));

	ui.dbbButtons->button(QDialogButtonBox::Reset)->setText(tr("Back"));
	connect(ui.dbbButtons,SIGNAL(clicked(QAbstractButton *)),SLOT(onDialogButtonClicked(QAbstractButton *)));

	initialize(APluginManager);
	initGroups();
	
	updatePageAddress();
	setErrorMessage(QString::null,false);
	setDialogState(STATE_ADDRESS);

	QString contact = qApp->clipboard()->text();
	if (FGateways && !FGateways->gateAvailDescriptorsByContact(contact).isEmpty())
		setContactText(contact);
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
		Jid serviceJid = AContactJid.domain();
		if (FGateways && FGateways->availServices(streamJid()).contains(serviceJid))
			contact = FGateways->legacyIdFromUserJid(AContactJid);
		setContactText(contact);
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
	return FSelectProfileWidget!=NULL ? FSelectProfileWidget->selectedProfile() : Jid::null;
}

void AddContactDialog::setGatewayJid(const Jid &AGatewayJid)
{
	if (FSelectProfileWidget)
		FSelectProfileWidget->setSelectedProfile(AGatewayJid);
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
	QString login = FGateways!=NULL ? FGateways->normalizeContactLogin(ADescriptor.id,contactText(),true) : contactText();
	if (ADescriptor.id == GSID_ICQ)
		text = tr("This is an ICQ number: %1").arg(login);
	else if (ADescriptor.id == GSID_SMS)
		text = tr("This is a phone number: %1").arg(login);
	else if (ADescriptor.id == GSID_MAIL)
		text = tr("This is a e-mail address: %1").arg(login);
	else
		text = tr("This is a %1 address: %2").arg(ADescriptor.name).arg(login);

	return text;
}

bool AddContactDialog::registerDescriptorIfNeed(const IGateServiceDescriptor &ADescriptor)
{
	if (ADescriptor.needGate)
	{
		if (FGateways)
		{
			IDiscoIdentity identity;
			identity.category = "gateway";
			identity.type = ADescriptor.type;
			if (FGateways->streamServices(streamJid(),identity).isEmpty())
			{
				QList<Jid> availGates = FGateways->availServices(streamJid(),identity);
				if (!availGates.isEmpty())
				{
					QDialog *dialog = FGateways->showAddLegacyAccountDialog(streamJid(),availGates.first());
					return dialog->exec()==QDialog::Accepted;
				}
				return false;
			}
			return true;
		}
		return false;
	}
	return true;
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
	ui.lblParamsContact->setText(FGateways!=NULL ? FGateways->normalizeContactLogin(FDescriptor.id,contactText(),true) : contactText());
	
	if (FGateways)
	{
		delete FSelectProfileWidget;
		FSelectProfileWidget = new SelectProfileWidget(FRoster,FGateways,FOptionsManager,FDescriptor,ui.wdtSelectProfile);
		connect(FSelectProfileWidget,SIGNAL(profilesChanged()),SLOT(onSelectedProfileChanched()));
		connect(FSelectProfileWidget,SIGNAL(selectedProfileChanged()),SLOT(onSelectedProfileChanched()));
		connect(FSelectProfileWidget,SIGNAL(adjustSizeRequested()),SLOT(onAdjustDialogSize()));
		ui.wdtSelectProfile->layout()->addWidget(FSelectProfileWidget);
	}
}

void AddContactDialog::setDialogState(int AState)
{
	if (AState != FDialogState)
	{
		if (AState == STATE_ADDRESS)
		{
			ui.wdtPageAddress->setVisible(true);
			ui.wdtPageConfirm->setVisible(false);
			ui.wdtPageParams->setVisible(false);
			ui.wdtSelectProfile->setVisible(false);
			ui.dbbButtons->button(QDialogButtonBox::Reset)->setEnabled(false);
			ui.dbbButtons->button(QDialogButtonBox::Ok)->setText(tr("Continue"));
		}
		else if (AState == STATE_CONFIRM)
		{
			ui.wdtPageAddress->setVisible(false);
			ui.wdtPageConfirm->setVisible(true);
			ui.wdtPageParams->setVisible(false);
			ui.wdtSelectProfile->setVisible(false);
			ui.dbbButtons->button(QDialogButtonBox::Reset)->setEnabled(true);
			ui.dbbButtons->button(QDialogButtonBox::Ok)->setText(tr("Continue"));
		}
		else if (AState == STATE_PARAMS)
		{
			resolveContactJid();
			ui.wdtPageAddress->setVisible(false);
			ui.wdtPageConfirm->setVisible(false);
			ui.wdtPageParams->setVisible(true);
			ui.wdtSelectProfile->setVisible(true);
			ui.dbbButtons->button(QDialogButtonBox::Reset)->setEnabled(true);
			ui.dbbButtons->button(QDialogButtonBox::Ok)->setText(tr("Add Contact"));
		}
		FDialogState = AState;
		QTimer::singleShot(1,this,SLOT(onAdjustDialogSize()));
	}
}

void AddContactDialog::setDialogEnabled(bool AEnabled)
{
	ui.wdtPageAddress->setEnabled(AEnabled);
	ui.wdtPageConfirm->setEnabled(AEnabled);
	ui.wdtPageParams->setEnabled(AEnabled);
	ui.wdtSelectProfile->setEnabled(AEnabled);
	ui.dbbButtons->setEnabled(AEnabled);
}

void AddContactDialog::setRealContactJid(const Jid &AContactJid)
{
	if (FAvatars)
		FAvatars->insertAutoAvatar(ui.lblParamsPhoto,AContactJid,QSize(50,50),"pixmap");
	FContactJid = AContactJid.bare();
}

void AddContactDialog::setResolveNickState(bool AResolve)
{
	if (AResolve && ui.lneParamsNick->text().isEmpty())
	{
		setNickName(defaultContactNick(contactJid()));
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
		setStyleSheet(styleSheet());
		QTimer::singleShot(1,this,SLOT(onAdjustDialogSize()));
		ui.dbbButtons->button(QDialogButtonBox::Ok)->setEnabled(AMessage.isEmpty() && !contactText().isEmpty());
	}
}

void AddContactDialog::resolveDescriptor()
{
	QList<QString> confirmTypes;
	QList<IGateServiceDescriptor> confirmDescriptors;
	foreach(const IGateServiceDescriptor &descriptor, FGateways!=NULL ? FGateways->gateHomeDescriptorsByContact(contactText()) : confirmDescriptors)
	{
		if (!confirmTypes.contains(descriptor.type))
		{
			confirmTypes.append(descriptor.type);
			confirmDescriptors.append(descriptor);
		}
	}

	if (confirmDescriptors.count() > 1)
	{
		updatePageConfirm(confirmDescriptors);
		setDialogState(STATE_CONFIRM);
	}
	else if (!confirmDescriptors.isEmpty())
	{
		IGateServiceDescriptor descriptor = confirmDescriptors.value(0);
		if (registerDescriptorIfNeed(descriptor))
		{
			updatePageParams(descriptor);
			setDialogState(STATE_PARAMS);
		}
	}
	else
	{
		setErrorMessage(tr("Invalid address. Please check the address and try again."),true);
	}
}

void AddContactDialog::resolveContactJid()
{
	QString errMessage;
	bool nextResolve = false;

	QString contact = FGateways!=NULL ? FGateways->normalizeContactLogin(FDescriptor.id,contactText()) : contactText().trimmed();

	Jid gateJid = FSelectProfileWidget->selectedProfile();
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
		errMessage = tr("Select your return address.");
	}

	setErrorMessage(errMessage,false);

	if (nextResolve)
		resolveContactName();
}

void AddContactDialog::resolveContactName()
{
	if (contactJid().isValid())
	{
		QString errMessage;
		IRosterItem ritem = FRoster!=NULL ? FRoster->rosterItem(contactJid()) : IRosterItem();
		if (!ritem.isValid)
		{
			if (FVcardPlugin)
				FVcardPlugin->requestVCard(streamJid(), contactJid());
			setResolveNickState(true);
		}
		else
		{
			setNickName(!ritem.name.isEmpty() ? ritem.name : defaultContactNick(contactText()));
			setGroup(ritem.groups.toList().value(0));
			errMessage = tr("This contact is already present in your contact-list.");
		}
		setErrorMessage(errMessage,false);
	}
}

void AddContactDialog::showEvent(QShowEvent *AEvent)
{
	if (!FShown)
	{
		FShown = true;
		QTimer::singleShot(1,this,SLOT(onAdjustDialogSize()));
	}
	QDialog::showEvent(AEvent);
}

void AddContactDialog::onDialogButtonClicked(QAbstractButton *AButton)
{
	if (ui.dbbButtons->buttonRole(AButton) == QDialogButtonBox::AcceptRole)
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
					if (registerDescriptorIfNeed(it.value()))
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
				if (FMetaRoster && FMetaRoster->isEnabled())
				{
					IMetaContact contact;
					contact.name = nickName();
					contact.groups += group();
					contact.items += contactJid();
					FCreateRequest = FMetaRoster->createContact(contact);
					if (!FCreateRequest.isEmpty())
					{
						setDialogEnabled(false);
						FRosterChanger->subscribeContact(streamJid(),contactJid(),QString::null,true);
					}
					else
					{
						onMetaActionResult(FCreateRequest,ErrorHandler::coditionByCode(ErrorHandler::INTERNAL_SERVER_ERROR),tr("Failed to send request to the server"));
					}
				}
				else
				{
					FRoster->setItem(contactJid(),nickName(),QSet<QString>()<<group());
					FRosterChanger->subscribeContact(streamJid(),contactJid(),QString::null);
					accept();
				}
			}
		}
	}
	else if (ui.dbbButtons->buttonRole(AButton) == QDialogButtonBox::ResetRole)
	{
		setErrorMessage(QString::null,false);
		updatePageAddress();
		setDialogState(STATE_ADDRESS);
	}
	else if (ui.dbbButtons->buttonRole(AButton) == QDialogButtonBox::RejectRole)
	{
		reject();
	}
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

void AddContactDialog::onSelectedProfileChanched()
{
	if (FDialogState == STATE_PARAMS)
	{
		resolveContactJid();
	}
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
	if (FDialogState==STATE_PARAMS && FContactJidRequest==AId)
	{
		setRealContactJid(AUserJid);
		resolveContactName();
	}
}

void AddContactDialog::onGatewayErrorReceived(const QString &AId, const QString &AError)
{
	Q_UNUSED(AError);
	if (FDialogState==STATE_PARAMS && FContactJidRequest==AId)
	{
		setRealContactJid(Jid::null);
		setErrorMessage(tr("Failed to request contact JID from transport."),false);
	}
}

void AddContactDialog::onRosterItemReceived(const IRosterItem &AItem, const IRosterItem &ABefore)
{
	Q_UNUSED(ABefore);
	if (AItem.itemJid == contactJid())
	{
		if (FMessageProcessor)
			FMessageProcessor->createWindow(streamJid(),contactJid(),Message::Chat,IMessageHandler::SM_SHOW);
		accept();
	}
}

void AddContactDialog::onMetaActionResult(const QString &AActionId, const QString &AErrCond, const QString &AErrMessage)
{
	if (FCreateRequest == AActionId)
	{
		if (!AErrCond.isEmpty())
		{
			setErrorMessage(tr("Failed to add contact due to an error: %1").arg(AErrMessage),false);
			setDialogEnabled(true);
		}
	}
}
