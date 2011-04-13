#include "addmetacontactdialog.h"

#include <QClipboard>
#include <QMessageBox>
#include <QApplication>
#include <QDesktopWidget>

#define ADR_GATE_DESCRIPTOR_ID      Action::DR_Parametr1

#define NICK_RESOLVE_TIMEOUT        1000

enum DescriptorStatuses {
	DS_UNAVAILABLE,
	DS_UNREGISTERED,
	DS_DISABLED,
	DS_ENABLED
};

AddMetaContactDialog::AddMetaContactDialog(IRosterChanger *ARosterChanger, IPluginManager *APluginManager, const Jid &AStreamJid, QWidget *AParent) : QDialog(AParent)
{
	ui.setupUi(this);
	setAttribute(Qt::WA_DeleteOnClose,true);
	setWindowTitle(tr("Add Contact"));

	setMinimumWidth(350);
	IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->insertAutoIcon(this,MNI_RCHANGER_ADD_CONTACT,0,0,"windowIcon");
	StyleStorage::staticStorage(RSR_STORAGE_STYLESHEETS)->insertAutoStyle(this,STS_RCHANGER_ADDMETACONTACTDIALOG);

	FMetaContacts = NULL;
	FMetaRoster = NULL;
	FAvatars = NULL;
	FVcardPlugin = NULL;
	FOptionsManager = NULL;
	FRosterChanger = ARosterChanger;

	FShown = false;
	FNickResolved = false;
	FAvatarIndex = -1;
	FStreamJid = AStreamJid;

	FItemsLayout = new QVBoxLayout;
	FItemsLayout->setMargin(0);
	FItemsLayout->addStretch();
	ui.wdtItems->setLayout(FItemsLayout);
	ui.scaItems->setVisible(false);

	ui.tlbPhotoNext->setVisible(false);
	ui.tlbPhotoPrev->setVisible(false);
	ui.lblPhotoIndex->setVisible(false);
	connect(ui.tlbPhotoPrev,SIGNAL(clicked()),SLOT(onPrevPhotoButtonClicked()));
	connect(ui.tlbPhotoNext,SIGNAL(clicked()),SLOT(onNextPhotoButtonClicked()));

	IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->insertAutoIcon(ui.pbtAddItem,MNI_RCHANGER_ADDMETACONTACT_ADD_ITEM);

	ui.dbbButtons->button(QDialogButtonBox::Ok)->setText(tr("Add Contact"));
	connect(ui.dbbButtons,SIGNAL(accepted()),SLOT(onDialogAccepted()));
	connect(ui.dbbButtons,SIGNAL(rejected()),SLOT(reject()));

	initialize(APluginManager);
	createGatewaysMenu();
	resolveClipboardText();
	updateDialogState();
}

AddMetaContactDialog::~AddMetaContactDialog()
{
	emit dialogDestroyed();
}

Jid AddMetaContactDialog::streamJid() const
{
	return FStreamJid;
}

Jid AddMetaContactDialog::contactJid() const
{
	IAddMetaItemWidget *widget = FItemWidgets.value(0);
	return widget!=NULL ? widget->contactJid() : Jid::null;
}

void AddMetaContactDialog::setContactJid(const Jid &AContactJid)
{
	if (FItemWidgets.isEmpty() && AContactJid.isValid())
	{
		IGateServiceDescriptor descriptor = FGateways->descriptorByContact(AContactJid.pBare());
		if (FAvailDescriptors.contains(descriptor.id))
			addContactItem(descriptor);
	}

	IAddMetaItemWidget *widget = FItemWidgets.value(0);
	if (widget)
		widget->setContactJid(AContactJid);
}

QString AddMetaContactDialog::contactText() const
{
	IAddMetaItemWidget *widget = FItemWidgets.value(0);
	return widget!=NULL ? widget->contactText() : QString::null;
}

void AddMetaContactDialog::setContactText(const QString &AContact)
{
	if (FItemWidgets.isEmpty() && !AContact.isEmpty())
	{
		IGateServiceDescriptor descriptor = FGateways->descriptorByContact(AContact);
		if (FAvailDescriptors.contains(descriptor.id))
			addContactItem(descriptor);
	}

	IAddMetaItemWidget *widget = FItemWidgets.value(0);
	if (widget)
		widget->setContactText(AContact);
}

QString AddMetaContactDialog::nickName() const
{
	return ui.lneNick->text().trimmed();
}

void AddMetaContactDialog::setNickName(const QString &ANick)
{
	ui.lneNick->setText(ANick);
}

QString AddMetaContactDialog::group() const
{
	return QString::null;
}

void AddMetaContactDialog::setGroup(const QString &AGroup)
{
	Q_UNUSED(AGroup);
}

Jid AddMetaContactDialog::gatewayJid() const
{
	IAddMetaItemWidget *widget = FItemWidgets.value(0);
	return widget!=NULL ? widget->gatewayJid() : Jid::null;
}

void AddMetaContactDialog::setGatewayJid(const Jid &AGatewayJid)
{
	IAddMetaItemWidget *widget = FItemWidgets.value(0);
	if (widget)
		widget->setGatewayJid(AGatewayJid);
}

void AddMetaContactDialog::initialize(IPluginManager *APluginManager)
{
	IPlugin *plugin = APluginManager->pluginInterface("IMetaContacts").value(0,NULL);
	if (plugin)
	{
		FMetaContacts = qobject_cast<IMetaContacts *>(plugin->instance());
		FMetaRoster = FMetaContacts!=NULL ? FMetaContacts->findMetaRoster(FStreamJid) : NULL;
		if (FMetaRoster)
		{
			connect(FMetaRoster->instance(),SIGNAL(metaContactReceived(const IMetaContact &, const IMetaContact &)),
				SLOT(onMetaContactReceived(const IMetaContact &, const IMetaContact &)));
			connect(FMetaRoster->instance(),SIGNAL(metaActionResult(const QString &, const QString &, const QString &)),
				SLOT(onMetaActionResult(const QString &, const QString &, const QString &)));
		}
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
	}

	plugin = APluginManager->pluginInterface("IRostersViewPlugin").value(0,NULL);
	if (plugin)
	{
		IRostersViewPlugin *rostersViewPlugin = qobject_cast<IRostersViewPlugin *>(plugin->instance());
		FRostersView = rostersViewPlugin!=NULL ? rostersViewPlugin->rostersView() : NULL;
	}

	plugin = APluginManager->pluginInterface("IOptionsManager").value(0,NULL);
	if (plugin)
	{
		FOptionsManager = qobject_cast<IOptionsManager *>(plugin->instance());
	}
}

void AddMetaContactDialog::createGatewaysMenu()
{
	if (FGateways)
	{
		Menu *menu = new Menu(ui.pbtAddItem);
		foreach(QString descriptorId, FGateways->availDescriptors())
		{
			IGateServiceDescriptor descriptor = FGateways->descriptorById(descriptorId);
			if (descriptorStatus(descriptor) != DS_UNAVAILABLE)
			{
				Action *action = new Action(menu);
				action->setText(descriptor.name);
				action->setIcon(RSR_STORAGE_MENUICONS,descriptor.iconKey);
				action->setData(ADR_GATE_DESCRIPTOR_ID,descriptorId);
				connect(action,SIGNAL(triggered(bool)),SLOT(onAddItemActionTriggered(bool)));
				menu->addAction(action,AG_DEFAULT,true);
				FAvailDescriptors.append(descriptorId);
			}
		}
		ui.pbtAddItem->setMenu(menu);
	}
}

void AddMetaContactDialog::resolveClipboardText()
{
	if (FGateways)
	{
		setContactText(QApplication::clipboard()->text().trimmed());
		ui.lneNick->setFocus();
	}
}

void AddMetaContactDialog::addContactItem(const IGateServiceDescriptor &ADescriptor, const QString &AContact)
{
	if (FGateways)
	{
		switch(descriptorStatus(ADescriptor))
		{
		case DS_UNREGISTERED:
			{
				static bool blocked = false;
				if (!blocked)
				{
					IDiscoIdentity identity;
					identity.category = "gateway";
					identity.type = ADescriptor.type;
					QList<Jid> availGates = FGateways->availServices(FStreamJid,identity);
					if (!availGates.isEmpty())
					{
						QDialog *dialog = FGateways->showAddLegacyAccountDialog(FStreamJid,availGates.at(0),this);
						if (dialog->exec() == QDialog::Accepted)
						{
							blocked = true;
							addContactItem(ADescriptor,AContact);
							blocked = false;
						}
					}
				}
			}
			break;
		case DS_ENABLED:
			{
				IAddMetaItemWidget *widget = FRosterChanger->newAddMetaItemWidget(FStreamJid,ADescriptor.id,ui.wdtItems);
				widget->instance()->setFocus();
				widget->setContactText(AContact);
				connect(widget->instance(),SIGNAL(adjustSizeRequested()),SLOT(onItemWidgetAdjustSizeRequested()));
				connect(widget->instance(),SIGNAL(deleteButtonClicked()),SLOT(onItemWidgetDeleteButtonClicked()));
				connect(widget->instance(),SIGNAL(showOptionsRequested()),SLOT(onItemWidgetShowOptionsRequested()));
				connect(widget->instance(),SIGNAL(contactJidChanged(const Jid &)),SLOT(onItemWidgetContactJidChanged(const Jid &)));
				FItemWidgets.append(widget);
				FItemsLayout->insertWidget(FItemsLayout->count()-1,widget->instance());
				QTimer::singleShot(0,this,SLOT(onAdjustDialogSize()));
			}
			break;
		case DS_DISABLED:
			{

			}
			break;
		default:
			break;
		}
		updateDialogState();
	}
}

int AddMetaContactDialog::descriptorStatus(const IGateServiceDescriptor &ADescriptor) const
{
	if (FGateways && !ADescriptor.id.isEmpty())
	{
		if (ADescriptor.needGate)
		{
			IDiscoIdentity identity;
			identity.category = "gateway";
			identity.type = ADescriptor.type;
			if (!FGateways->availServices(FStreamJid,identity).isEmpty())
			{
				if (ADescriptor.needLogin)
				{
					foreach(Jid gateJid, FGateways->streamServices(FStreamJid,identity))
					{
						if (FGateways->isServiceEnabled(FStreamJid,gateJid))
							return DS_ENABLED;
					}
					return DS_UNREGISTERED;
				}
				return DS_ENABLED;
			}
			return DS_UNAVAILABLE;
		}
		return DS_ENABLED;
	}
	return DS_UNAVAILABLE;
}

QString AddMetaContactDialog::defaultContactNick(const Jid &AContactJid) const
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

void AddMetaContactDialog::updateDialogState()
{
	FValidContacts.clear();
	FAvatarContacts.clear();

	bool isAcceptable = !FItemWidgets.isEmpty();
	foreach(IAddMetaItemWidget *widget, FItemWidgets)
	{
		Jid contactJid = widget->contactJid().bare();
		if (contactJid.isValid())
		{
			FValidContacts.append(contactJid);
			if (FVcardPlugin && !FNoVcardContacts.contains(contactJid))
			{
				if (FContactAvatars.contains(contactJid))
				{
					FAvatarContacts.append(contactJid);
				}
				else if (FVcardPlugin->hasVCard(contactJid))
				{
					static const QList<QString> nickFields = QList<QString>() << VVN_FULL_NAME << VVN_NICKNAME << VVN_GIVEN_NAME << VVN_FAMILY_NAME;

					IVCard *vcard = FVcardPlugin->vcard(contactJid);
					QImage avatar = vcard->photoImage();
					if (!avatar.isNull())
					{
						avatar = ImageManager::roundSquared(avatar, 36, 2);
						FAvatarContacts.append(contactJid);
						FContactAvatars.insert(contactJid,avatar);
					}

					if (!FNickResolved && ui.lneNick->text().trimmed().isEmpty())
					{
						QString nick;
						for (int i=0; nick.isEmpty() && i<nickFields.count(); i++)
							nick = vcard->value(nickFields.at(i));
						ui.lneNick->setText(nick.isEmpty() ? defaultContactNick(contactJid) : nick);
						ui.lneNick->selectAll();
						ui.lneNick->setFocus();
						FNickResolved = true;
					}

					vcard->unlock();
				}
				else
				{
					FVcardPlugin->requestVCard(FStreamJid,contactJid);
				}
			}
		}
		else
		{
			isAcceptable = false;
		}
	}

	setAvatarIndex(qMin(FAvatarIndex>=0 ? FAvatarIndex : 0, FAvatarContacts.count()-1));

	ui.dbbButtons->button(QDialogButtonBox::Ok)->setEnabled(isAcceptable);
}

void AddMetaContactDialog::setDialogEnabled(bool AEnabled)
{
	ui.scaItems->setEnabled(AEnabled);
	ui.lneNick->setEnabled(AEnabled);
	ui.pbtAddItem->setEnabled(AEnabled);

	if (!AEnabled)
	{
		ui.tlbPhotoPrev->setEnabled(AEnabled);
		ui.tlbPhotoNext->setEnabled(AEnabled);
		ui.dbbButtons->button(QDialogButtonBox::Ok)->setEnabled(AEnabled);
	}
	else
	{
		updateDialogState();
	}
}

void AddMetaContactDialog::setAvatarIndex(int AIndex)
{
	if (AIndex >= 0 && AIndex<FAvatarContacts.count())
	{
		QImage avatar = FContactAvatars.value(FAvatarContacts.value(AIndex));
		ui.lblPhoto->setPixmap(QPixmap::fromImage(avatar));
		FAvatarIndex = AIndex;
	}
	else
	{
		if (FAvatars)
		{
			QImage avatar = ImageManager::roundSquared(FAvatars->avatarImage(Jid::null,false), 36, 2);
			ui.lblPhoto->setPixmap(QPixmap::fromImage(avatar));
		}
		else
		{
			ui.lblPhoto->clear();
		}
		FAvatarIndex = -1;
	}

	ui.tlbPhotoPrev->setEnabled(FAvatarContacts.count()>1);
	ui.tlbPhotoNext->setEnabled(FAvatarContacts.count()>1);
	ui.lblPhotoIndex->setText(QString("%1/%2").arg(FAvatarIndex+1).arg(FAvatarContacts.count()));
}

void AddMetaContactDialog::showEvent(QShowEvent *AEvent)
{
	QDialog::showEvent(AEvent);
	if (!FShown)
	{
		FShown = true;
		QTimer::singleShot(0,this,SLOT(onAdjustDialogSize()));
	}
}

void AddMetaContactDialog::onDialogAccepted()
{
	if (FMetaRoster && !FItemWidgets.isEmpty())
	{
		IMetaContact contact;

		if (ui.lneNick->text().trimmed().isEmpty())
		{
			contact.name = defaultContactNick(contactJid());
			ui.lneNick->setText(contact.name);
		}
		else
		{
			contact.name = ui.lneNick->text().trimmed();
		}

		foreach(IAddMetaItemWidget *widget, FItemWidgets)
			contact.items += widget->contactJid().bare();

		FCreateActiontId = FMetaRoster->createContact(contact);
		if (!FCreateActiontId.isEmpty())
		{
			setDialogEnabled(false);
		}
		else
		{
			onMetaActionResult(FCreateActiontId,ErrorHandler::coditionByCode(ErrorHandler::INTERNAL_SERVER_ERROR),tr("Failed to send request to the server"));
		}
	}
}

void AddMetaContactDialog::onNickResolveTimeout()
{
	if (!FNickResolved && contactJid().isValid() && ui.lneNick->text().trimmed().isEmpty())
	{
		ui.lneNick->setText(defaultContactNick(contactJid()));
		ui.lneNick->selectAll();
		ui.lneNick->setFocus();
		FNickResolved = true;
	}
}

void AddMetaContactDialog::onAdjustDialogSize()
{
	if (!FItemWidgets.isEmpty())
	{
		int maxHeight = qApp->desktop()->availableGeometry(this).height()/2;
		int hintHeight = ui.wdtItems->sizeHint().height();
		ui.scaItems->setFixedHeight(hintHeight < maxHeight ? hintHeight : maxHeight);
		ui.scaItems->setMinimumWidth(ui.wdtItems->sizeHint().width());

		ui.scaItems->setVisible(true);
		ui.pbtAddItem->setText(tr("Add another address"));
	}
	else
	{
		ui.scaItems->setVisible(false);
		ui.pbtAddItem->setText(tr("Specify contact's address"));
	}

	foreach(IAddMetaItemWidget *widget, FItemWidgets)
		widget->setCorrectSizes(ui.lblNick->width(),ui.wdtPhoto->width());

	QTimer::singleShot(0,this,SLOT(onAdjustBorderSize()));
}

void AddMetaContactDialog::onAdjustBorderSize()
{
	if (parentWidget())
		parentWidget()->adjustSize();
	else
		adjustSize();
}

void AddMetaContactDialog::onPrevPhotoButtonClicked()
{
	if (FAvatarIndex > 0)
		setAvatarIndex(FAvatarIndex-1);
	else
		setAvatarIndex(FAvatarContacts.count()-1);
}

void AddMetaContactDialog::onNextPhotoButtonClicked()
{
	if (FAvatarIndex < FAvatarContacts.count()-1)
		setAvatarIndex(FAvatarIndex+1);
	else
		setAvatarIndex(0);
}

void AddMetaContactDialog::onAddItemActionTriggered(bool)
{
	Action *action = qobject_cast<Action *>(sender());
	if (action)
	{
		addContactItem(FGateways->descriptorById(action->data(ADR_GATE_DESCRIPTOR_ID).toString()));
	}
}

void AddMetaContactDialog::onItemWidgetAdjustSizeRequested()
{
	QTimer::singleShot(0,this,SLOT(onAdjustDialogSize()));
}

void AddMetaContactDialog::onItemWidgetDeleteButtonClicked()
{
	AddMetaItemWidget *widget = qobject_cast<AddMetaItemWidget *>(sender());
	if (FItemWidgets.contains(widget))
	{
		FItemWidgets.removeAll(widget);
		ui.wdtItems->layout()->removeWidget(widget);
		delete widget;
		updateDialogState();
		QTimer::singleShot(0,this,SLOT(onAdjustDialogSize()));
	}
}

void AddMetaContactDialog::onItemWidgetShowOptionsRequested()
{
	if (FOptionsManager)
	{
		FOptionsManager->showOptionsDialog(OPN_GATEWAYS_ACCOUNTS);
	}
}

void AddMetaContactDialog::onItemWidgetContactJidChanged(const Jid &AContactJid)
{
	if (AContactJid.isValid() && !FNickResolved)
		QTimer::singleShot(NICK_RESOLVE_TIMEOUT,this,SLOT(onNickResolveTimeout()));
	updateDialogState();
}

void AddMetaContactDialog::onVCardReceived(const Jid &AContactJid)
{
	if (FValidContacts.contains(AContactJid))
		updateDialogState();
}

void AddMetaContactDialog::onVCardError(const Jid &AContactJid, const QString &AError)
{
	Q_UNUSED(AError);
	if (FValidContacts.contains(AContactJid))
		FNoVcardContacts.append(AContactJid);
}

void AddMetaContactDialog::onMetaContactReceived(const IMetaContact &AContact, const IMetaContact &ABefore)
{
	Q_UNUSED(ABefore);
	if (AContact.items.contains(contactJid()))
	{
		if (FRostersView)
		{
			IRostersModel *rmodel = FRostersView->rostersModel();
			IRosterIndex *sroot = rmodel!=NULL ? rmodel->streamRoot(FStreamJid) : NULL;
			if (sroot)
			{
				QMultiMap<int, QVariant> findData;
				findData.insert(RDR_TYPE,RIT_METACONTACT);
				findData.insert(RDR_INDEX_ID,AContact.id);
				IRosterIndex *index = sroot->findChild(findData,true).value(0);
				if (index)
				{
					QModelIndex modelIndex = FRostersView->mapFromModel(rmodel->modelIndexByRosterIndex(index));
					FRostersView->instance()->setCurrentIndex(modelIndex);
					FRostersView->instance()->clearSelection();
					FRostersView->instance()->scrollTo(modelIndex);
					FRostersView->instance()->selectionModel()->select(modelIndex,QItemSelectionModel::Select);
				}
			}
		}

		IMetaTabWindow *window = FMetaContacts->newMetaTabWindow(FStreamJid,AContact.id);
		if (window)
			window->showTabPage();

		accept();
	}
}

void AddMetaContactDialog::onMetaActionResult(const QString &AActionId, const QString &AErrCond, const QString &AErrMessage)
{
	if (AActionId == FCreateActiontId)
	{
		if (!AErrCond.isEmpty())
		{
			QMessageBox::warning(this,tr("Failed to create contact"),tr("Failed to add contact due to an error: %1").arg(AErrMessage));
			setDialogEnabled(true);
		}
		else
		{
			foreach(IAddMetaItemWidget *widget, FItemWidgets)
				FRosterChanger->subscribeContact(FStreamJid,widget->contactJid().bare());
		}
	}
}
