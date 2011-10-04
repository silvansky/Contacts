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
#include <utils/balloontip.h>
#include <utils/log.h>

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
	StyleStorage::staticStorage(RSR_STORAGE_STYLESHEETS)->insertAutoStyle(this,STS_RCHANGER_ADDCONTACTDIALOG);

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

	FShown = false;
	FServiceFailed = false;
	FDialogState = -1;
	FSelectProfileWidget = NULL;

	ui.cmbParamsGroup->setView(new QListView);
	ui.wdtConfirmAddresses->setLayout(new QVBoxLayout);
	ui.wdtConfirmAddresses->layout()->setMargin(0);
	ui.wdtConfirmAddresses->layout()->setSpacing(10);

	IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->insertAutoIcon(ui.lblErrorIcon,MNI_RCHANGER_ADDCONTACT_ERROR,0,0,"pixmap");

	connect(FRoster->instance(),SIGNAL(received(const IRosterItem &, const IRosterItem &)),
		SLOT(onRosterItemReceived(const IRosterItem &, const IRosterItem &)));

	connect(ui.cmbParamsGroup,SIGNAL(currentIndexChanged(int)),SLOT(onGroupCurrentIndexChanged(int)));
	connect(ui.lneParamsNick,SIGNAL(textEdited(const QString &)),SLOT(onContactNickEdited(const QString &)));
	connect(ui.lneAddressContact,SIGNAL(textEdited(const QString &)),SLOT(onContactTextEdited(const QString &)));

	connect(ui.pbtBack, SIGNAL(clicked()), SLOT(onBackButtonclicked()));
	connect(ui.pbtContinue, SIGNAL(clicked()), SLOT(onContinueButtonclicked()));
	connect(ui.pbtCancel, SIGNAL(clicked()), SLOT(onCancelButtonclicked()));

	ui.lblError->setVisible(false);
	ui.lblErrorIcon->setVisible(false);

	ui.pbtBack->setText(tr("Back"));
	ui.pbtCancel->setText(tr("Cancel"));
	ui.pbtContinue->setEnabled(false);

#ifdef Q_WS_MAC
	ui.buttonsLayout->addWidget(ui.pbtContinue);
	ui.buttonsLayout->setSpacing(16);
#endif

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
	BalloonTip::hideBalloon();
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

void AddContactDialog::selectRosterIndex()
{
	if (FRostersView)
	{
		IRostersModel *rmodel = FRostersView->rostersModel();
		IRosterIndex *sroot = rmodel!=NULL ? rmodel->streamRoot(streamJid()) : NULL;
		if (sroot)
		{
			QMultiMap<int, QVariant> findData;
			if (FMetaRoster!=NULL)
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

bool AddContactDialog::isDescriptorAcceptable(const IGateServiceDescriptor &ADescriptor)
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
			ui.pbtBack->setVisible(false);
			ui.pbtContinue->setText(tr("Continue"));
		}
		else if (AState == STATE_CONFIRM)
		{
			ui.wdtPageAddress->setVisible(false);
			ui.wdtPageConfirm->setVisible(true);
			ui.wdtPageParams->setVisible(false);
			ui.wdtSelectProfile->setVisible(false);
			ui.pbtBack->setVisible(true);
			ui.pbtContinue->setText(tr("Continue"));
		}
		else if (AState == STATE_PARAMS)
		{
			ui.wdtPageAddress->setVisible(false);
			ui.wdtPageConfirm->setVisible(false);
			ui.wdtPageParams->setVisible(true);
			ui.wdtSelectProfile->setVisible(true);
			ui.pbtBack->setVisible(true);
			ui.pbtContinue->setText(tr("Add Contact"));

			resolveContactJid();
			resolveLinkedContactsJid();
		}

		FDialogState = AState;
		adjustSize();
		QTimer::singleShot(1, this, SLOT(onAdjustDialogSize()));
	}
}

void AddContactDialog::setDialogEnabled(bool AEnabled)
{
	ui.wdtPageAddress->setEnabled(AEnabled);
	ui.wdtPageConfirm->setEnabled(AEnabled);
	ui.wdtPageParams->setEnabled(AEnabled);
	ui.wdtSelectProfile->setEnabled(AEnabled);
}

void AddContactDialog::setRealContactJid(const Jid &AContactJid)
{
	if (FAvatars)
		FAvatars->insertAutoAvatar(ui.lblParamsPhoto,AContactJid,QSize(48, 48),"pixmap");
	FContactJid = AContactJid.bare();
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
		//BalloonTip::hideBalloon();
		if (!AMessage.isEmpty())
		{
			QPoint p = ui.lneAddressContact->mapToGlobal(QPoint(0, 0));
			p += QPoint(ui.lneAddressContact->width(), ui.lneAddressContact->height() / 2);
			BalloonTip::showBalloon(IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getIcon(MNI_RCHANGER_ADDCONTACT_ERROR),
						QString::null,
						AMessage,
						p,
						0,
						true,
						BalloonTip::ArrowLeft, parentWidget() ? parentWidget() : this);
		}
		//ui.lblError->setText(AMessage);
		//ui.lblError->setVisible(!AMessage.isEmpty());
		//ui.lblErrorIcon->setVisible(!AMessage.isEmpty());
		ui.lneAddressContact->setProperty("error", !AMessage.isEmpty() && AInvalidInput ? true : false);
		StyleStorage::updateStyle(this);
		//QTimer::singleShot(1,this,SLOT(onAdjustDialogSize()));
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
			if (!(descriptor.needGate && descriptor.readOnly))
				confirmDescriptors.append(descriptor);
			else
				readOnlyDescriptor = descriptor;
			confirmTypes += descriptor.type;
			confirmLinked += descriptor.linkedDescriptors;
			confirmBlocked += descriptor.blockedDescriptors;
		}
	}

	for (QList<IGateServiceDescriptor>::iterator it=confirmDescriptors.begin(); it!=confirmDescriptors.end(); )
	{
		if (confirmLinked.contains(it->id) || confirmBlocked.contains(it->id))
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
		if (isDescriptorAcceptable(descriptor))
		{
			updatePageParams(descriptor);
			setDialogState(STATE_PARAMS);
		}
		else
			LogError(QString("[AddContactDialog] gateway descriptor %1 not acceptable for contact %2").arg(descriptor.name, contactText()));
	}
	else if (!readOnlyDescriptor.id.isEmpty())
	{
		setErrorMessage(tr("You can add this contact only on %1 site.").arg(readOnlyDescriptor.name),false);
	}
	else
	{
		setErrorMessage(tr("Could not find such address. Check that you have not done a mistake."),true);
	}
}

void AddContactDialog::resolveContactJid()
{
	QString errMessage;
	bool nextResolve = false;

	QString contact = FGateways!=NULL ? FGateways->normalizedContactLogin(FDescriptor,contactText()) : contactText().trimmed();

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
		errMessage = tr("Select a contact-list in which you want to add a contact.");
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
		IRosterItem ritem = FRoster->rosterItem(contactJid());
		if (!ritem.isValid)
		{
			if (FVcardPlugin)
				FVcardPlugin->requestVCard(streamJid(), contactJid());
			setResolveNickState(true);
		}
		else
		{
			ui.pbtContinue->setText(tr("Open"));
			setNickName(!ritem.name.isEmpty() ? ritem.name : defaultContactNick(contactText()));
			setGroup(ritem.groups.toList().value(0));
			errMessage = tr("This contact is already present in your contact-list.");
		}
		setErrorMessage(errMessage,false);
	}
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
						if (!FGateways->serviceDescriptor(streamJid(),gate).readOnly)
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

void AddContactDialog::showEvent(QShowEvent *AEvent)
{
	if (!FShown)
	{
		FShown = true;
		QTimer::singleShot(1,this,SLOT(onAdjustDialogSize()));
	}
	QDialog::showEvent(AEvent);
}

void AddContactDialog::mousePressEvent(QMouseEvent *AEvent)
{
	BalloonTip::hideBalloon();
	QDialog::mousePressEvent(AEvent);
}

void AddContactDialog::moveEvent(QMoveEvent *AEvent)
{
	BalloonTip::hideBalloon();
	QDialog::moveEvent(AEvent);
}

bool AddContactDialog::event(QEvent *AEvent)
{
	if (AEvent->type() == QEvent::ParentChange)
	{
		CustomBorderContainer * border = qobject_cast<CustomBorderContainer*>(parentWidget());
		if (border)
		{
			connect(border, SIGNAL(moved()), SLOT(onHideErrorBalloon()));
			border->installEventFilter(this);
		}
	}
	return QDialog::event(AEvent);
}

bool AddContactDialog::eventFilter(QObject *AObject, QEvent *AEvent)
{
//	static bool f = true;
//	if ((AEvent->type() == QEvent::MouseButtonPress) || (AEvent->type() == QEvent::ActivationChange) /*&& f*/)
//	{
//		//f = false;
//		qDebug() << "AddContactDialog::eventFilter: object" << AObject->objectName() << AObject->metaObject()->className() << "event" << AEvent->type();
//		if (AEvent->type() == QEvent::ActivationChange)
//		{
//			QWidget * w = qobject_cast<QWidget*>(AObject);
//			if (w)
//			{
//				qDebug() << "active: " << w->isActiveWindow();
//				//bool h = QDialog::eventFilter(AObject, AEvent);
//				if (!w->isActiveWindow())
//					BalloonTip::hideBalloon();
//				//f = true;
//				return true;
//			}
//		}
//		bool h = QDialog::eventFilter(AObject, AEvent);
//		BalloonTip::hideBalloon();
//		//f = true;
//		return h;
//	}
	return QDialog::eventFilter(AObject, AEvent);
}

void AddContactDialog::onBackButtonclicked()
{
	//BalloonTip::hideBalloon();
	setErrorMessage(QString::null,false);
	updatePageAddress();
	setDialogState(STATE_ADDRESS);
}

void AddContactDialog::onContinueButtonclicked()
{
	BalloonTip::hideBalloon();
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
				if (isDescriptorAcceptable(it.value()))
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
			if (FRoster->rosterItem(contactJid()).isValid)
			{
				selectRosterIndex();
				if (FMessageProcessor)
					FMessageProcessor->createMessageWindow(streamJid(),contactJid(),Message::Chat,IMessageHandler::SM_SHOW);
				accept();
			}
			else if (FMetaRoster && FMetaRoster->isEnabled())
			{
				IMetaContact contact;
				contact.name = nickName();
				contact.groups += group();
				contact.items += contactJid();
				contact.items += FLinkedContacts.toSet();

				FContactCreateRequest = FMetaRoster->createContact(contact);
				if (!FContactCreateRequest.isEmpty())
				{
					foreach(Jid itemJid, contact.items)
						FRosterChanger->subscribeContact(streamJid(),itemJid,QString::null,true);
					setDialogEnabled(false);
				}
				else
				{
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
				accept();
			}
		}
	}
}

void AddContactDialog::onCancelButtonclicked()
{
	BalloonTip::hideBalloon();
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
	BalloonTip::hideBalloon();
	setErrorMessage(QString::null,false);
	ui.pbtContinue->setEnabled(!AText.isEmpty());
}

void AddContactDialog::onContactNickEdited(const QString &AText)
{
	BalloonTip::hideBalloon();
	Q_UNUSED(AText);
	setResolveNickState(false);
}

void AddContactDialog::onGroupCurrentIndexChanged(int AIndex)
{
	BalloonTip::hideBalloon();
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
	if (FContactJidRequest==AId)
	{
		if (FDialogState==STATE_PARAMS)
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
	}
}

void AddContactDialog::onGatewayErrorReceived(const QString &AId, const QString &AError)
{
	Q_UNUSED(AError);
	if (FContactJidRequest==AId)
	{
		if (FDialogState==STATE_PARAMS)
		{
			setRealContactJid(Jid::null);
			setErrorMessage(tr("Failed to request contact JID from transport."),false);
		}
	}
	else if (FLinkedJidRequests.contains(AId))
	{
		FLinkedJidRequests.remove(AId);
	}
}

void AddContactDialog::onRosterItemReceived(const IRosterItem &AItem, const IRosterItem &ABefore)
{
	Q_UNUSED(ABefore);
	if (AItem.itemJid == contactJid())
	{
		if (FMetaRoster==NULL || !FMetaRoster->isEnabled())
		{
			selectRosterIndex();
			if (FMessageProcessor)
				FMessageProcessor->createMessageWindow(streamJid(),contactJid(),Message::Chat,IMessageHandler::SM_SHOW);
			accept();
		}
	}
}

void AddContactDialog::onMetaActionResult(const QString &AActionId, const QString &AErrCond, const QString &AErrMessage)
{
	Q_UNUSED(AErrCond);
	if (FContactCreateRequest == AActionId)
	{
		QString metaId = FMetaRoster->itemMetaContact(contactJid());
		if (!metaId.isEmpty())
		{
			if (!FParentMetaId.isEmpty() && !FMetaRoster->metaContact(FParentMetaId).id.isEmpty())
			{
				FMetaRoster->mergeContacts(FParentMetaId,QList<QString>()<<metaId);
			}
			else if (FMessageProcessor)
			{
				selectRosterIndex();
				FMessageProcessor->createMessageWindow(streamJid(),contactJid(),Message::Chat,IMessageHandler::SM_SHOW);
			}
			accept();
		}
		else
		{
			setErrorMessage(tr("Failed to add contact due to an error: %1").arg(AErrMessage),false);
			setDialogEnabled(true);
		}
	}
}

void AddContactDialog::onHideErrorBalloon()
{
	BalloonTip::hideBalloon();
}
