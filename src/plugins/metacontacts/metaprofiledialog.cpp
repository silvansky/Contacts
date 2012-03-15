#include "metaprofiledialog.h"

#include <QTimer>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QDesktopServices>
#include <utils/graphicseffectsstorage.h>
#include <definitions/graphicseffects.h>
#include <definitions/menuicons.h>
#ifdef Q_WS_MAC
# include <utils/macwidgets.h>
#endif

#define BIRTHDAY_META_ORDER     -10
#define MAX_STATUS_TEXT_SIZE    60

MetaProfileDialog::MetaProfileDialog(IPluginManager *APluginManager, IMetaContacts *AMetaContacts, IMetaRoster *AMetaRoster, const QString &AMetaId, QWidget *AParent) : QDialog(AParent)
{
	ui.setupUi(this);
	ui.lblName->setElideMode(Qt::ElideRight);
	setWindowIconText(tr("Contact Profile"));

	FGateways = NULL;
	FStatusIcons = NULL;
	FStatusChanger = NULL;
	FRosterChanger = NULL;
	FVCardPlugin = NULL;

	FMetaId = AMetaId;
	FMetaRoster = AMetaRoster;
	FMetaContacts = AMetaContacts;

	FBorder = CustomBorderStorage::staticStorage(RSR_STORAGE_CUSTOMBORDER)->addBorder(this, CBS_DIALOG);
	if (FBorder)
	{
		FBorder->setResizable(false);
		FBorder->setMinimizeButtonVisible(false);
		FBorder->setMaximizeButtonVisible(false);
		FBorder->setAttribute(Qt::WA_DeleteOnClose,true);
		FBorder->setWindowTitle(windowIconText());
		connect(this, SIGNAL(accepted()), FBorder, SLOT(closeWidget()));
		connect(this, SIGNAL(rejected()), FBorder, SLOT(closeWidget()));
		connect(FBorder, SIGNAL(closeClicked()), SLOT(reject()));
		setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
	}
	else
		setAttribute(Qt::WA_DeleteOnClose,true);

	ui.sawContents->setLayout(new QVBoxLayout);
	ui.sawContents->layout()->setSpacing(10);
	ui.sawContents->layout()->setContentsMargins(0,10,0,0);

	StyleStorage::staticStorage(RSR_STORAGE_STYLESHEETS)->insertAutoStyle(this,STS_METACONTACTS_METAPROFILEDIALOG);
	GraphicsEffectsStorage::staticStorage(RSR_STORAGE_GRAPHICSEFFECTS)->installGraphicsEffect(this, GFX_LABELS);
	GraphicsEffectsStorage::staticStorage(RSR_STORAGE_GRAPHICSEFFECTS)->installGraphicsEffect(ui.lblStatusIcon, GFX_STATUSICONS);

	connect(FMetaRoster->instance(),SIGNAL(metaContactReceived(const IMetaContact &, const IMetaContact &)),
		SLOT(onMetaContactReceived(const IMetaContact &, const IMetaContact &)));
	connect(FMetaRoster->instance(),SIGNAL(metaAvatarChanged(const QString &)),SLOT(onMetaAvatarChanged(const QString &)));
	connect(FMetaRoster->instance(),SIGNAL(metaPresenceChanged(const QString &)),SLOT(onMetaPresenceChanged(const QString &)));

#ifdef Q_WS_MAC
	ui.buttonsLayout->setSpacing(16);
	//ui.buttonsLayout->addWidget(ui.pbtAddContact);
	setWindowGrowButtonEnabled(this->window(), false);
#endif

	ui.pbtClose->setFocus();
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
		MetaContainer &container = FMetaContainers[BIRTHDAY_META_ORDER];
		container.metaWidget = new QWidget(ui.sawContents);
		container.metaWidget->setLayout(new QHBoxLayout);
		container.metaWidget->layout()->setMargin(0);
		ui.sawContents->layout()->addWidget(container.metaWidget);

		container.metaLabel = new QLabel(tr("Birthday:"),container.metaWidget);
		container.metaWidget->layout()->addWidget(container.metaLabel);

		container.itemsWidget = new QLabel(birthday.toString(Qt::SystemLocaleLongDate), container.metaWidget);
		container.itemsWidget->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Preferred);
		container.metaWidget->layout()->addWidget(container.itemsWidget);
	}
}

void MetaProfileDialog::updateStatusText()
{
	QString status;
	IMetaContact contact = FMetaRoster->metaContact(FMetaId);
	if (!FMetaRoster->roster()->subscriptionRequests().intersect(contact.items).isEmpty())
		status = tr("Requests authorization");
	else if (contact.ask == SUBSCRIPTION_SUBSCRIBE)
		status = tr("Sent an authorization request");
	else if (contact.subscription == SUBSCRIPTION_NONE)
		status = tr("Not authorized");
	else
		status = FMetaRoster->metaPresenceItem(FMetaId).status;

	QString text = status.left(MAX_STATUS_TEXT_SIZE);
	text += text.size() < status.size() ? "..." : "";
	ui.lblStatusText->setText(text);
}

void MetaProfileDialog::updateLeftLabelsSizes()
{
	int maxWidth = 0;
	for (QMap<int, MetaContainer>::const_iterator it=FMetaContainers.constBegin(); it!=FMetaContainers.constEnd(); it++)
		maxWidth = qMax(it->metaLabel->sizeHint().width(),maxWidth);

	maxWidth += 10;
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

QString MetaProfileDialog::metaItemLink(const Jid &AItemJid, const IMetaItemDescriptor &ADescriptor) const
{
	if (FGateways)
	{
		if (ADescriptor.metaOrder == MIO_VKONTAKTE)
		{
			QString userId = Jid(FGateways->legacyIdFromUserJid(streamJid(),AItemJid)).node();
			return QString("http://vk.com/%1").arg(userId);
		}
		else if (ADescriptor.metaOrder == MIO_FACEBOOK)
		{
			QString userId = Jid(FGateways->legacyIdFromUserJid(streamJid(),AItemJid)).node();
			return QString("http://www.facebook.com/profile.php?id=%1").arg(userId.right(userId.size()-1));
		}
	}
	return QString::null;
}

bool MetaProfileDialog::eventFilter(QObject *AObject, QEvent *AEvent)
{
	if (AObject->objectName()=="wdtItem" && (AEvent->type()==QEvent::Enter || AEvent->type() == QEvent::Leave))
	{
		CloseButton *cbtDelete = AObject->findChild<CloseButton *>();
		if (cbtDelete)
			cbtDelete->setVisible(AEvent->type()==QEvent::Enter && cbtDelete->property("canDelete").toBool());
	}
	return QDialog::eventFilter(AObject,AEvent);
}

void MetaProfileDialog::onAdjustDialogSize()
{
	updateLeftLabelsSizes();
	ui.scaContacts->setFixedHeight(qMin(ui.sawContents->sizeHint().height(),350));
	QTimer::singleShot(0,this,SLOT(onAdjustBorderSize()));
}

void MetaProfileDialog::onAdjustBorderSize()
{
	adjustSize();
	if (FBorder)
		FBorder->adjustSize();
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
				dialog->setNickName(ui.lblName->text());
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
		FMetaContacts->deleteContactWithNotify(FMetaRoster->streamJid(),FMetaId,dialog->property("itemJid").toString());
	}
}

void MetaProfileDialog::onItemNameLinkActivated(const QString &AUrl)
{
	QDesktopServices::openUrl(AUrl);
}

void MetaProfileDialog::onMetaAvatarChanged(const QString &AMetaId)
{
	if (AMetaId == FMetaId)
	{
		QImage avatar = ImageManager::roundSquared(FMetaRoster->metaAvatarImage(FMetaId, true, false), 48, 3);
		if (avatar.isNull())
			avatar = IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getImage(MNI_AVATAR_EMPTY_FEMALE, 1);
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
		updateStatusText();
	}
}

void MetaProfileDialog::onMetaContactReceived(const IMetaContact &AContact, const IMetaContact &ABefore)
{
	if (AContact.id == FMetaId)
	{
		ui.lblName->setText(FMetaContacts->metaContactName(AContact));
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
					container.metaLabel->setObjectName("lblMetaLabel");
					container.metaLabel->setAlignment(Qt::AlignLeft|Qt::AlignTop);
					container.metaWidget->layout()->addWidget(container.metaLabel);

					container.itemsWidget = new QWidget(container.metaWidget);
					container.itemsWidget->setObjectName("wdtItemsWidget");
					container.itemsWidget->setLayout(new QVBoxLayout);
					container.itemsWidget->layout()->setMargin(0);
					container.itemsWidget->layout()->setSpacing(2);
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

				QString itemName = FMetaContacts->itemFormattedLogin(itemIt.value());
				QString itemLink = metaItemLink(itemIt.value(),descriptor);
				QLabel *lblItemName = new QLabel(wdtItem);
				lblItemName->setTextFormat(Qt::RichText);
				lblItemName->setText(itemLink.isEmpty() ? Qt::escape(itemName) : QString("<a href='%1'>%2</a>").arg(itemLink).arg(itemName));
				lblItemName->setObjectName("lblItemName");
				connect(lblItemName,SIGNAL(linkActivated(const QString &)),SLOT(onItemNameLinkActivated(const QString &)));
				wdtItem->layout()->addWidget(lblItemName);

				CloseButton *cbtDelete = new CloseButton(wdtItem);
				cbtDelete->setObjectName("cbtDelete");
				cbtDelete->setVisible(false);
				cbtDelete->setProperty("itemJid",itemIt->bare());
				cbtDelete->setProperty("itemName",itemName);
				cbtDelete->setProperty("canDelete",FMetaContacts->canDeleteMetaContact(FMetaRoster->streamJid(),FMetaId,itemIt->bare()));

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
			updateStatusText();
			QTimer::singleShot(0,this,SLOT(onAdjustDialogSize()));
		}
	}
}

