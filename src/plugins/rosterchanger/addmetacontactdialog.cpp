#include "addmetacontactdialog.h"

#include <QClipboard>
#include <QApplication>
#include <QDesktopWidget>
#include <utils/custominputdialog.h>

#define ADR_GATE_DESCRIPTOR_ID      Action::DR_Parametr1

#define NICK_RESOLVE_TIMEOUT        1000

AddMetaContactDialog::AddMetaContactDialog(IMetaRoster *AMetaRoster, IRosterChanger *ARosterChanger, IPluginManager *APluginManager, QWidget *AParent) : QDialog(AParent)
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
	FMetaRoster = AMetaRoster;
	FRosterChanger = ARosterChanger;

	FShown = false;
	FNickResolved = false;
	FAvatarIndex = -1;

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

	connect(FMetaRoster->instance(),SIGNAL(metaActionResult(const QString &, const QString &, const QString &)),
		SLOT(onMetaActionResult(const QString &, const QString &, const QString &)));

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
	return FMetaRoster->streamJid();
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
		IGateServiceDescriptor descriptor = FGateways->gateHomeDescriptorsByContact(AContactJid.pBare()).value(0);
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
		IGateServiceDescriptor descriptor = FGateways->gateHomeDescriptorsByContact(AContact).value(0);
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

QString AddMetaContactDialog::parentMetaContactId() const
{
	return FParentMetaId;
}

void AddMetaContactDialog::setParentMetaContactId(const QString &AMetaId)
{
	FParentMetaId = AMetaId;
	ui.lneNick->setEnabled(FParentMetaId.isEmpty());
}

void AddMetaContactDialog::initialize(IPluginManager *APluginManager)
{
	IPlugin *plugin = APluginManager->pluginInterface("IMetaContacts").value(0,NULL);
	if (plugin)
	{
		FMetaContacts = qobject_cast<IMetaContacts *>(plugin->instance());
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
		foreach(const IGateServiceDescriptor &descriptor, FGateways->gateDescriptors())
		{
			if (!(descriptor.needGate && descriptor.readOnly) && FGateways->gateDescriptorStatus(streamJid(),descriptor) != IGateways::GDS_UNAVAILABLE)
			{
				Action *action = new Action(menu);
				action->setText(descriptor.name);
				action->setIcon(RSR_STORAGE_MENUICONS,descriptor.iconKey);
				action->setData(ADR_GATE_DESCRIPTOR_ID,descriptor.id);
				connect(action,SIGNAL(triggered(bool)),SLOT(onAddItemActionTriggered(bool)));
				menu->addAction(action,AG_DEFAULT,true);
				FAvailDescriptors.append(descriptor.id);
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
		switch(FGateways->gateDescriptorStatus(streamJid(),ADescriptor))
		{
		case IGateways::GDS_UNREGISTERED:
			{
				static bool blocked = false;
				if (!blocked)
				{
					QList<Jid> availGates = FGateways->gateDescriptorServices(streamJid(),ADescriptor);
					if (!availGates.isEmpty())
					{
						QDialog *dialog = FGateways->showAddLegacyAccountDialog(streamJid(),availGates.at(0),this);
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
		case IGateways::GDS_ENABLED:
			{
				IAddMetaItemWidget *widget = FRosterChanger->newAddMetaItemWidget(streamJid(),ADescriptor.id,ui.wdtItems);
				widget->instance()->setFocus();
				widget->setContactText(AContact);
				connect(widget->instance(),SIGNAL(adjustSizeRequested()),SLOT(onItemWidgetAdjustSizeRequested()));
				connect(widget->instance(),SIGNAL(deleteButtonClicked()),SLOT(onItemWidgetDeleteButtonClicked()));
				connect(widget->instance(),SIGNAL(contactJidChanged(const Jid &)),SLOT(onItemWidgetContactJidChanged(const Jid &)));
				FItemWidgets.append(widget);
				FItemsLayout->insertWidget(FItemsLayout->count()-1,widget->instance());
				QTimer::singleShot(0,this,SLOT(onAdjustDialogSize()));
			}
			break;
		case IGateways::GDS_DISABLED:
			{

			}
			break;
		default:
			break;
		}
		updateDialogState();
	}
}

QString AddMetaContactDialog::defaultContactNick(const Jid &AContactJid) const
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
					FVcardPlugin->requestVCard(streamJid(),contactJid);
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
	ui.lneNick->setEnabled(AEnabled && FParentMetaId.isEmpty());
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
			QImage avatar = ImageManager::roundSquared(FAvatars->avatarImage(Jid::null,false,false), 36, 2);
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

		FCreateActionId = FMetaRoster->createContact(contact);
		if (!FCreateActionId.isEmpty())
		{
			foreach(Jid itemJid, contact.items)
				FRosterChanger->subscribeContact(streamJid(),itemJid);
			setDialogEnabled(false);
		}
		else
		{
			onMetaActionResult(FCreateActionId,ErrorHandler::conditionByCode(ErrorHandler::INTERNAL_SERVER_ERROR),tr("Failed to send request to the server"));
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
		addContactItem(FGateways->gateDescriptorById(action->data(ADR_GATE_DESCRIPTOR_ID).toString()));
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

void AddMetaContactDialog::onMetaActionResult(const QString &AActionId, const QString &AErrCond, const QString &AErrMessage)
{
	if (AActionId == FCreateActionId)
	{
		if (AErrCond.isEmpty())
		{
			QString metaId = FMetaRoster->itemMetaContact(contactJid());
			if (!FParentMetaId.isEmpty() && !FMetaRoster->metaContact(FParentMetaId).id.isEmpty())
			{
				FMetaRoster->mergeContacts(FParentMetaId,QList<QString>()<<metaId);
			}
			else
			{
				IMetaTabWindow *window = FMetaContacts->newMetaTabWindow(streamJid(),metaId);
				if (window)
					window->showTabPage();
			}
			accept();
		}
		else
		{
			CustomInputDialog *dialog = new CustomInputDialog(CustomInputDialog::Info);
			dialog->setCaptionText(tr("Failed to create contact"));
			dialog->setInfoText(tr("Failed to add contact due to an error: %1").arg(AErrMessage));
			dialog->setAcceptButtonText(tr("Ok"));
			dialog->setDeleteOnClose(true);
			dialog->show();
			setDialogEnabled(true);
		}
	}
}
