#include "metaprofiledialog.h"

#include <QTimer>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QDesktopServices>
#include <utils/graphicseffectsstorage.h>
#include <definitions/graphicseffects.h>
#include <definitions/menuicons.h>

#define MAX_STATUS_TEXT_SIZE    60

MetaProfileDialog::MetaProfileDialog(IPluginManager *APluginManager, IMetaContacts *AMetaContacts, IMetaRoster *AMetaRoster, const QString &AMetaId, QWidget *AParent) : QDialog(AParent)
{
	ui.setupUi(this);

	setMinimumWidth(400);
	ui.lneName->setAttribute(Qt::WA_MacShowFocusRect, false);

	FGateways = NULL;
	FStatusIcons = NULL;
	FStatusChanger = NULL;
	FRosterChanger = NULL;
	FVCardPlugin = NULL;

	FMetaId = AMetaId;
	FMetaRoster = AMetaRoster;
	FMetaContacts = AMetaContacts;

	ui.lblCaption->setText(tr("Contact Profile"));

	FBorder = CustomBorderStorage::staticStorage(RSR_STORAGE_CUSTOMBORDER)->addBorder(this, CBS_DIALOG);
	if (FBorder)
	{
		FBorder->setResizable(false);
		FBorder->setMinimizeButtonVisible(false);
		FBorder->setMaximizeButtonVisible(false);
		FBorder->setAttribute(Qt::WA_DeleteOnClose,true);
		FBorder->setWindowTitle(ui.lblCaption->text());
		connect(this, SIGNAL(accepted()), FBorder, SLOT(closeWidget()));
		connect(this, SIGNAL(rejected()), FBorder, SLOT(closeWidget()));
		connect(FBorder, SIGNAL(closeClicked()), SLOT(reject()));
		setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
	}
	else
		setAttribute(Qt::WA_DeleteOnClose,true);

	ui.sawContents->setLayout(new QVBoxLayout);
	ui.sawContents->layout()->setMargin(0);
	ui.sawContents->layout()->setSpacing(14);

	StyleStorage::staticStorage(RSR_STORAGE_STYLESHEETS)->insertAutoStyle(this,STS_METACONTACTS_METAPROFILEDIALOG);
	GraphicsEffectsStorage::staticStorage(RSR_STORAGE_GRAPHICSEFFECTS)->installGraphicsEffect(this, GFX_LABELS);
	GraphicsEffectsStorage::staticStorage(RSR_STORAGE_GRAPHICSEFFECTS)->installGraphicsEffect(ui.lblStatusIcon, GFX_STATUSICONS);

	connect(ui.lblBirthdayPostcard,SIGNAL(linkActivated(const QString &)),SLOT(onLabelLinkActivated(const QString &)));

	connect(FMetaRoster->instance(),SIGNAL(metaContactReceived(const IMetaContact &, const IMetaContact &)),
		SLOT(onMetaContactReceived(const IMetaContact &, const IMetaContact &)));
	connect(FMetaRoster->instance(),SIGNAL(metaAvatarChanged(const QString &)),SLOT(onMetaAvatarChanged(const QString &)));
	connect(FMetaRoster->instance(),SIGNAL(metaPresenceChanged(const QString &)),SLOT(onMetaPresenceChanged(const QString &)));

	ui.pbtClose->setFocus();
	connect(ui.lneName,SIGNAL(editingFinished()),SLOT(onContactNameEditFinished()));
	connect(ui.pbtAddContact,SIGNAL(clicked()),SLOT(onAddContactButtonClicked()));
	connect(ui.pbtClose,SIGNAL(clicked()),SLOT(reject()));

	initialize(APluginManager);

	updateBirthday();
	onMetaAvatarChanged(FMetaId);
	onMetaPresenceChanged(FMetaId);
	onMetaContactReceived(FMetaRoster->metaContact(FMetaId),IMetaContact());
}

MetaProfileDialog::~MetaProfileDialog()
{
	if (FBorder)
		FBorder->deleteLater();
	delete FDeleteContactDialog;
	emit dialogDestroyed();
}

Jid MetaProfileDialog::streamJid() const
{
	return FMetaRoster->streamJid();
}

QString MetaProfileDialog::metaContactId() const
{
	return FMetaId;
}

void MetaProfileDialog::initialize(IPluginManager *APluginManager)
{
	IPlugin *plugin = APluginManager->pluginInterface("IRosterChanger").value(0,NULL);
	if (plugin)
		FRosterChanger = qobject_cast<IRosterChanger *>(plugin->instance());

	plugin = APluginManager->pluginInterface("IGateways").value(0,NULL);
	if (plugin)
		FGateways = qobject_cast<IGateways *>(plugin->instance());

	plugin = APluginManager->pluginInterface("IStatusIcons").value(0,NULL);
	if (plugin)
		FStatusIcons = qobject_cast<IStatusIcons *>(plugin->instance());

	plugin = APluginManager->pluginInterface("IStatusChanger").value(0,NULL);
	if (plugin)
		FStatusChanger = qobject_cast<IStatusChanger *>(plugin->instance());

	plugin = APluginManager->pluginInterface("IVCardPlugin").value(0,NULL);
	if (plugin)
		FVCardPlugin = qobject_cast<IVCardPlugin *>(plugin->instance());

	ui.pbtAddContact->setEnabled(FRosterChanger!=NULL);
}

void MetaProfileDialog::updateBirthday()
{
	QDate birthday;
	if (FVCardPlugin)
	{
		IMetaContact contact = FMetaRoster->metaContact(FMetaId);
		QList<Jid> orderedItems = FMetaContacts->itemOrders(contact.items.toList()).values();
		foreach(Jid itemJid, orderedItems)
		{
			if (FVCardPlugin->hasVCard(itemJid))
			{
				IVCard *vcard = FVCardPlugin->vcard(itemJid);
				birthday = QDate::fromString(vcard->value(VVN_BIRTHDAY),Qt::ISODate);
				if (!birthday.isValid())
					birthday = QDate::fromString(vcard->value(VVN_BIRTHDAY),Qt::TextDate);
				vcard->unlock();

				if (birthday.isValid())
					break;
			}
			else
			{
				FVCardPlugin->requestVCard(streamJid(),itemJid);
			}
		}
	}

	if (birthday.isValid())
	{
		ui.lblBirthday->setText(birthday.toString(Qt::SystemLocaleLongDate));
		ui.lblBirthdayPostcard->setText(QString("<a href='%2'>%1</a>").arg(tr("Send a postcard")).arg("http://cards.rambler.ru/list_cards.html?card_group_id=434"));
	}
	ui.lblBirthdayLabel->setVisible(birthday.isValid());
	ui.lblBirthday->setVisible(birthday.isValid());
	ui.lblBirthdayPostcard->setVisible(birthday.isValid());
}

void MetaProfileDialog::updateLeftLabelsSizes()
{
	int maxWidth = ui.lblName->sizeHint().width();
	if (ui.lblBirthdayLabel->isVisible())
		maxWidth = qMax(ui.lblBirthdayLabel->sizeHint().width(),maxWidth);
	for (QMap<int, MetaContainer>::const_iterator it=FMetaContainers.constBegin(); it!=FMetaContainers.constEnd(); it++)
		maxWidth = qMax(it->metaLabel->sizeHint().width(),maxWidth);

	ui.lblName->setMinimumWidth(maxWidth);
	ui.lblBirthdayLabel->setMinimumWidth(maxWidth);
	for (QMap<int, MetaContainer>::const_iterator it=FMetaContainers.constBegin(); it!=FMetaContainers.constEnd(); it++)
		it->metaLabel->setMinimumWidth(maxWidth);
}

QString MetaProfileDialog::metaLabelText(const IMetaItemDescriptor &ADescriptor) const
{
	if (ADescriptor.metaOrder == MIO_SMS)
		return tr("Phone");
	else if (ADescriptor.metaOrder == MIO_MAIL)
		return tr("E-mail");
	return ADescriptor.name;
}

bool MetaProfileDialog::eventFilter(QObject *AObject, QEvent *AEvent)
{
	if (AObject->objectName()=="wdtItem" && (AEvent->type()==QEvent::Enter || AEvent->type() == QEvent::Leave))
	{
		CloseButton *cbtDelete = AObject->findChild<CloseButton *>();
		if (cbtDelete)
			cbtDelete->setVisible(AEvent->type()==QEvent::Enter);
	}
	return QDialog::eventFilter(AObject,AEvent);
}

void MetaProfileDialog::onAdjustDialogSize()
{
	updateLeftLabelsSizes();
	ui.scaContacts->setFixedHeight(qMin(ui.sawContents->minimumSizeHint().height(),350));
	QTimer::singleShot(0,this,SLOT(onAdjustBorderSize()));
}

void MetaProfileDialog::onAdjustBorderSize()
{
	if (FBorder)
		FBorder->adjustSize();
	else
		adjustSize();
}

void MetaProfileDialog::onContactNameEditFinished()
{
	QString name = ui.lneName->text().trimmed();
	if (!name.isEmpty())
	{
		FMetaRoster->renameContact(FMetaId,name);
	}
}

void MetaProfileDialog::onAddContactButtonClicked()
{
	if (FRosterChanger)
	{
		QWidget *widget = FRosterChanger->showAddContactDialog(streamJid());
		if (widget)
		{
			IAddContactDialog * dialog = NULL;
			if (!(dialog = qobject_cast<IAddContactDialog*>(widget)))
			{
				if (CustomBorderContainer * border = qobject_cast<CustomBorderContainer*>(widget))
					dialog = qobject_cast<IAddContactDialog*>(border->widget());
			}
			if (dialog)
			{
				IMetaContact contact = FMetaRoster->metaContact(FMetaId);
				dialog->setGroup(contact.groups.toList().value(0));
				dialog->setNickName(ui.lneName->text());
				dialog->setParentMetaContactId(FMetaId);
			}
		}
	}
}

void MetaProfileDialog::onDeleteContactButtonClicked()
{
	CloseButton *button = qobject_cast<CloseButton *>(sender());
	if (button && FMetaRoster->isOpen())
	{
		delete FDeleteContactDialog;
		FDeleteContactDialog = new CustomInputDialog(CustomInputDialog::None);
		FDeleteContactDialog->setWindowTitle(tr("Delete contact address"));
		FDeleteContactDialog->setCaptionText(FDeleteContactDialog->windowTitle());
		FDeleteContactDialog->setInfoText(tr("Record \"%1\" and the history of communication with it will be deleted. Operation can not be undone.").arg("<b>"+button->property("itemName").toString()+"</b>"));
		FDeleteContactDialog->setProperty("itemJid", button->property("itemJid"));
		FDeleteContactDialog->setAcceptButtonText(tr("Delete"));
		FDeleteContactDialog->setRejectButtonText(tr("Cancel"));
		FDeleteContactDialog->setAcceptIsDefault(false);
		connect(FDeleteContactDialog, SIGNAL(accepted()), SLOT(onDeleteContactDialogAccepted()));
		connect(FMetaRoster->instance(),SIGNAL(metaRosterClosed()),FDeleteContactDialog,SLOT(deleteLater()));
		FDeleteContactDialog->show();
	}
}

void MetaProfileDialog::onDeleteContactDialogAccepted()
{
	CustomInputDialog *dialog = qobject_cast<CustomInputDialog *>(sender());
	if (dialog)
	{
		FMetaContacts->deleteContactWithNotify(FMetaRoster,FMetaId,dialog->property("itemJid").toString());
	}
}

void MetaProfileDialog::onLabelLinkActivated(const QString &ALink)
{
	QDesktopServices::openUrl(ALink);
}

void MetaProfileDialog::onMetaAvatarChanged(const QString &AMetaId)
{
	if (AMetaId == FMetaId)
	{
		QImage avatar = ImageManager::roundSquared(FMetaRoster->metaAvatarImage(FMetaId, true, false),48,2);
		if (avatar.isNull())
		{
			avatar = IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getImage(MNI_AVATAR_EMPTY_FEMALE, 1);
		}
		ui.lblAvatar->setPixmap(QPixmap::fromImage(avatar));
	}
}

void MetaProfileDialog::onMetaPresenceChanged(const QString &AMetaId)
{
	if (AMetaId == FMetaId)
	{
		IPresenceItem pitem = FMetaRoster->metaPresenceItem(FMetaId);
		QIcon icon = FStatusIcons!=NULL ? FStatusIcons->iconByStatus(pitem.show,SUBSCRIPTION_BOTH,false) : QIcon();
		ui.lblStatusIcon->setPixmap(icon.pixmap(icon.availableSizes().value(0)));
		ui.lblStatusName->setText(FStatusChanger!=NULL ? FStatusChanger->nameByShow(pitem.show) : QString::null);

		QString status = pitem.status.left(MAX_STATUS_TEXT_SIZE);
		status += status.size() < pitem.status.size() ? "..." : "";
		ui.lblStatusText->setText(status);
	}
}

void MetaProfileDialog::onMetaContactReceived(const IMetaContact &AContact, const IMetaContact &ABefore)
{
	if (AContact.id == FMetaId)
	{
		ui.lneName->setText(FMetaContacts->metaContactName(AContact));
		if (AContact.items.isEmpty())
		{
			close();
		}
		else if (AContact.items != ABefore.items)
		{
			QSet<Jid> newItems = AContact.items - ABefore.items;
			QMap<int, Jid> orders = FMetaContacts->itemOrders(newItems.toList());
			for (QMap<int, Jid>::const_iterator itemIt=orders.constBegin(); itemIt!=orders.constEnd(); itemIt++)
			{
				IMetaItemDescriptor descriptor = FMetaContacts->metaDescriptorByItem(itemIt.value());
				MetaContainer &container = FMetaContainers[descriptor.metaOrder];
				if (!container.metaWidget)
				{
					container.metaWidget = new QWidget(ui.sawContents);
					container.metaWidget->setObjectName("metaWidget");
					container.metaWidget->setLayout(new QHBoxLayout);
					container.metaWidget->layout()->setMargin(0);
					ui.sawContents->layout()->addWidget(container.metaWidget);

					container.metaLabel = new QLabel(metaLabelText(descriptor)+":",container.metaWidget);
					container.metaLabel->setObjectName("metaLabel");
					container.metaLabel->setMinimumWidth(50);
					container.metaLabel->setAlignment(Qt::AlignLeft|Qt::AlignTop);
					container.metaWidget->layout()->addWidget(container.metaLabel);

					container.itemsWidget = new QWidget(container.metaWidget);
					container.itemsWidget->setObjectName("itemsWidget");
					container.itemsWidget->setLayout(new QVBoxLayout);
					container.itemsWidget->layout()->setMargin(0);
					container.itemsWidget->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Preferred);
					container.metaWidget->layout()->addWidget(container.itemsWidget);
				}

				QWidget *wdtItem = new QWidget(container.itemsWidget);
				wdtItem->setLayout(new QHBoxLayout);
				wdtItem->layout()->setMargin(0);
				wdtItem->installEventFilter(this);
				wdtItem->setObjectName("wdtItem");
				container.itemsWidget->layout()->addWidget(wdtItem);
				container.itemWidgets.insert(itemIt.value(),wdtItem);

				QString itemName = FMetaContacts->itemHint(itemIt.value());
				QLabel *lblItemName = new QLabel(itemName,wdtItem);
				lblItemName->setObjectName("itemName");
				wdtItem->layout()->addWidget(lblItemName);

				CloseButton *cbtDelete = new CloseButton(wdtItem);
				cbtDelete->setObjectName("deleteButton");
				cbtDelete->setVisible(false);
				cbtDelete->setProperty("itemJid",itemIt->bare());
				cbtDelete->setProperty("itemName",itemName);
				connect(cbtDelete,SIGNAL(clicked()),SLOT(onDeleteContactButtonClicked()));
				wdtItem->layout()->addWidget(cbtDelete);
				wdtItem->layout()->setAlignment(cbtDelete,Qt::AlignCenter);
				qobject_cast<QHBoxLayout *>(wdtItem->layout())->addStretch();
			}

			QSet<Jid> oldItems = ABefore.items - AContact.items;
			foreach(Jid itemJid, oldItems)
			{
				IMetaItemDescriptor descriptor = FMetaContacts->metaDescriptorByItem(itemJid);
				MetaContainer &container = FMetaContainers[descriptor.metaOrder];

				QWidget *wdtItem = container.itemWidgets.take(itemJid);
				container.itemsWidget->layout()->removeWidget(wdtItem);
				delete wdtItem;

				if (container.itemWidgets.isEmpty())
				{
					ui.sawContents->layout()->removeWidget(container.metaWidget);
					delete container.metaWidget;
					FMetaContainers.remove(descriptor.metaOrder);
				}
			}
			QTimer::singleShot(0,this,SLOT(onAdjustDialogSize()));
		}
	}
}

