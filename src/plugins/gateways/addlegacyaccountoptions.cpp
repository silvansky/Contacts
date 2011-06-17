#include "addlegacyaccountoptions.h"

#include <QToolButton>

#define ADR_GATEJID				Action::DR_Parametr1

AddLegacyAccountOptions::AddLegacyAccountOptions(IGateways *AGateways, const Jid &AStreamJid, QWidget *AParent) : QWidget(AParent)
{
	ui.setupUi(this);

	FGateways = AGateways;
	FStreamJid = AStreamJid;

	connect(FGateways->instance(),SIGNAL(availServicesChanged(const Jid &)),SLOT(onServicesChanged(const Jid &)));
	connect(FGateways->instance(),SIGNAL(streamServicesChanged(const Jid &)),SLOT(onServicesChanged(const Jid &)));

	FLayout = new QHBoxLayout(ui.wdtGateways);
	FLayout->setContentsMargins(20, 6, 20, 6);
	FLayout->addStretch();

	onServicesChanged(FStreamJid);
}

AddLegacyAccountOptions::~AddLegacyAccountOptions()
{

}

void AddLegacyAccountOptions::apply()
{
	emit childApply();
}

void AddLegacyAccountOptions::reset()
{
	emit childReset();
}

void AddLegacyAccountOptions::appendServiceButton(const Jid &AServiceJid)
{
	IGateServiceDescriptor descriptor = FGateways->serviceDescriptor(FStreamJid,AServiceJid);
	if (!FWidgets.contains(AServiceJid) && !descriptor.id.isEmpty() && descriptor.needLogin)
	{
		QWidget *widget = new QWidget(ui.wdtGateways);
		widget->setObjectName("serviceContainer");
		//widget->setMinimumWidth(widget->fontMetrics().boundingRect("XXXXXXXXXXX").width());

		QVBoxLayout *wlayout = new QVBoxLayout;
		wlayout->setMargin(0);
		widget->setLayout(wlayout);

		QToolButton *button = new QToolButton(widget);
		button->setObjectName("serviceButton");
		button->setToolButtonStyle(Qt::ToolButtonIconOnly);
		button->setIconSize(QSize(32,32));

		QLabel *label = new QLabel(descriptor.name,widget);
		label->setObjectName("serviceName");
		label->setAlignment(Qt::AlignCenter);

		Action *action = new Action(button);
		action->setIcon(RSR_STORAGE_MENUICONS,descriptor.iconKey,0);
		action->setText(descriptor.name);
		action->setData(ADR_GATEJID,AServiceJid.full());
		connect(action,SIGNAL(triggered(bool)),SLOT(onGateActionTriggeted(bool)));
		button->setDefaultAction(action);

		wlayout->addWidget(button,0,Qt::AlignCenter);
		wlayout->addWidget(label,0,Qt::AlignCenter);
		FLayout->insertWidget(FLayout->count()-1, widget);

		FWidgets.insert(AServiceJid,widget);
	}
}

void AddLegacyAccountOptions::removeServiceButton(const Jid &AServiceJid)
{
	if (FWidgets.contains(AServiceJid))
	{
		QWidget *widget = FWidgets.take(AServiceJid);
		FLayout->removeWidget(widget);
		widget->deleteLater();
	}
}

void AddLegacyAccountOptions::onGateActionTriggeted(bool)
{
	Action *action = qobject_cast<Action *>(sender());
	if (action)
	{
		Jid gateJid = action->data(ADR_GATEJID).toString();
		FGateways->showAddLegacyAccountDialog(FStreamJid,gateJid);
	}
}

void AddLegacyAccountOptions::onServicesChanged(const Jid &AStreamJid)
{
	if (FStreamJid == AStreamJid)
	{
		IDiscoIdentity identity;
		identity.category = "gateway";

		QList<Jid> usedGates = FGateways->streamServices(FStreamJid,identity);
		QList<Jid> availGates = FGateways->availServices(FStreamJid,identity);

		foreach(Jid serviceJid, availGates)
		{
			if (!usedGates.contains(serviceJid))
				appendServiceButton(serviceJid);
			else
				removeServiceButton(serviceJid);
		}

		foreach(Jid serviceJid, FWidgets.keys().toSet() - availGates.toSet())
			removeServiceButton(serviceJid);

		if (!FWidgets.isEmpty())
		{
			ui.lblInfo->setText(tr("You can link multiple accounts and communicate with your friends on other services"));
		}
		else
		{
			ui.lblInfo->setText(tr("All available accounts are already linked"));
		}
	}
}
