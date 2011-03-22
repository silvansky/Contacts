#include "addmetacontactdialog.h"

#include <QApplication>
#include <QDesktopWidget>

#define ADR_GATE_DESCRIPTOR_NAME    Action::DR_Parametr1

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
	IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->insertAutoIcon(this,MNI_RCHANGER_ADD_CONTACT,0,0,"windowIcon");
	StyleStorage::staticStorage(RSR_STORAGE_STYLESHEETS)->insertAutoStyle(this,STS_RCHANGER_ADDMETACONTACTDIALOG);

	FMetaRoster = NULL;
	FAvatars = NULL;
	FVcardPlugin = NULL;
	FRosterChanger = ARosterChanger;

	FStreamJid = AStreamJid;

	FItemsLayout = new QVBoxLayout;
	FItemsLayout->setMargin(0);
	FItemsLayout->addStretch();
	ui.wdtItems->setLayout(FItemsLayout);

	initialize(APluginManager);
	createGatewaysMenu();

	ui.dbbButtons->button(QDialogButtonBox::Ok)->setText(tr("Add Contact"));
	connect(ui.dbbButtons,SIGNAL(accepted()),SLOT(onDialogAccepted()));
	connect(ui.dbbButtons,SIGNAL(rejected()),SLOT(reject()));
}

AddMetaContactDialog::~AddMetaContactDialog()
{
	emit dialogDestroyed();
}

Jid AddMetaContactDialog::streamJid() const
{
	return FStreamJid;
}

QString AddMetaContactDialog::nickName() const
{
	return ui.lneNick->text().trimmed();
}

void AddMetaContactDialog::setNickName(const QString &ANick)
{
	ui.lneNick->setText(ANick);
}

void AddMetaContactDialog::initialize(IPluginManager *APluginManager)
{
	IPlugin *plugin = APluginManager->pluginInterface("IMetaContact").value(0,NULL);
	if (plugin)
	{
		IMetaContacts *metaContacts = qobject_cast<IMetaContacts *>(plugin->instance());
		FMetaRoster = metaContacts!=NULL ? metaContacts->findMetaRoster(FStreamJid) : NULL;
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
		//if (FVcardPlugin)
		//{
		//	connect(FVcardPlugin->instance(), SIGNAL(vcardReceived(const Jid &)),SLOT(onVCardReceived(const Jid &)));
		//	connect(FVcardPlugin->instance(), SIGNAL(vcardError(const Jid &, const QString &)),SLOT(onVCardError(const Jid &, const QString &)));
		//}
	}

	plugin = APluginManager->pluginInterface("IGateways").value(0,NULL);
	if (plugin)
	{
		FGateways = qobject_cast<IGateways *>(plugin->instance());
	}
}

void AddMetaContactDialog::createGatewaysMenu()
{
	if (FGateways)
	{
		Menu *menu = new Menu(ui.pbtAddItem);
		foreach(QString descriptorName, FGateways->availDescriptors())
		{
			IGateServiceDescriptor descriptor = FGateways->descriptorByName(descriptorName);
			if (descriptorStatus(descriptor) != DS_UNAVAILABLE)
			{
				Action *action = new Action(menu);
				action->setText(descriptor.name);
				action->setIcon(RSR_STORAGE_MENUICONS,descriptor.iconKey);
				action->setData(ADR_GATE_DESCRIPTOR_NAME,descriptor.name);
				connect(action,SIGNAL(triggered(bool)),SLOT(onAddItemActionTriggered(bool)));
				menu->addAction(action,AG_DEFAULT,true);
				FAvailDescriptors.append(descriptor.name);
			}
		}
		ui.pbtAddItem->setMenu(menu);
	}
}

void AddMetaContactDialog::addContactItem(const IGateServiceDescriptor &ADescriptor)
{
	if (FGateways)
	{
		switch(descriptorStatus(ADescriptor))
		{
		case DS_ENABLED:
			{
				EditItemWidget *widget = new EditItemWidget(FGateways,FStreamJid,ADescriptor,ui.wdtItems);
				FItemsLayout->insertWidget(FItemsLayout->count()-1,widget);
				connect(widget,SIGNAL(adjustSizeRequired()),SLOT(onItemWidgetAdjustSizeRequested()));
				connect(widget,SIGNAL(deleteButtonClicked()),SLOT(onItemWidgetDeleteButtonClicked()));
				connect(widget,SIGNAL(contactJidChanged(const Jid &)),SLOT(onItemWidgetContactJidChanged(const Jid &)));
				FItemWidgets.append(widget);
				QTimer::singleShot(1,this,SLOT(onAdjustDialogSize()));
			}
			break;
		case DS_DISABLED:
			{

			}
			break;
		case DS_UNREGISTERED:
			{
				IDiscoIdentity identity;
				identity.category = "gateway";
				identity.type = ADescriptor.type;
				QList<Jid> availGates = FGateways->availServices(FStreamJid,identity);
				if (!availGates.isEmpty())
					FGateways->showAddLegacyAccountDialog(FStreamJid,availGates.at(0),this);
			}
			break;
		default:
			break;
		}
	}
}

int AddMetaContactDialog::descriptorStatus(const IGateServiceDescriptor &ADescriptor) const
{
	if (FGateways && ADescriptor.isValid)
	{
		if (ADescriptor.needGate && ADescriptor.needLogin)
		{
			IDiscoIdentity identity;
			identity.category = "gateway";
			identity.type = ADescriptor.type;
			if (!FGateways->availServices(FStreamJid,identity).isEmpty())
			{
				foreach(Jid gateJid, FGateways->streamServices(FStreamJid,identity))
				{
					if (FGateways->isServiceEnabled(FStreamJid,gateJid))
						return DS_ENABLED;
				}
				return DS_UNREGISTERED;
			}
			return DS_UNAVAILABLE;
		}
		return DS_ENABLED;
	}
	return DS_UNAVAILABLE;
}

void AddMetaContactDialog::onDialogAccepted()
{
	accept();
}

void AddMetaContactDialog::onAdjustDialogSize()
{
	if (!FItemWidgets.isEmpty())
	{
		ui.scaItems->setVisible(true);
		ui.scaItems->setMinimumHeight(qMin(2*qApp->desktop()->availableGeometry(this).height()/3,ui.wdtItems->sizeHint().height()));
	}
	else
	{
		ui.scaItems->setVisible(false);
	}
	adjustSize();
}

void AddMetaContactDialog::onAddItemActionTriggered(bool)
{
	Action *action = qobject_cast<Action *>(sender());
	if (action)
	{
		addContactItem(FGateways->descriptorByName(action->data(ADR_GATE_DESCRIPTOR_NAME).toString()));
	}
}

void AddMetaContactDialog::onItemWidgetAdjustSizeRequested()
{
	QTimer::singleShot(1,this,SLOT(onAdjustDialogSize()));
}

void AddMetaContactDialog::onItemWidgetDeleteButtonClicked()
{
	EditItemWidget *widget = qobject_cast<EditItemWidget *>(sender());
	if (FItemWidgets.contains(widget))
	{
		FItemWidgets.removeAll(widget);
		ui.wdtItems->layout()->removeWidget(widget);
		delete widget;
		QTimer::singleShot(1,this,SLOT(onAdjustDialogSize()));
	}
}

void AddMetaContactDialog::onItemWidgetContactJidChanged(const Jid &AContactJid)
{

}
