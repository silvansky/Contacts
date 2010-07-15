#include "addlegacyaccountoptions.h"

#include <QHBoxLayout>
#include <QToolButton>

#define ADR_GATEJID				Action::DR_Parametr1

AddLegacyAccountOptions::AddLegacyAccountOptions(IGateways *AGateways, IServiceDiscovery *ADiscovery, const Jid &AStreamJid, QWidget *AParent) : QWidget(AParent)
{
	ui.setupUi(this);

	FGateways = AGateways;
	FDiscovery = ADiscovery;
	FStreamJid = AStreamJid;

	createButtons();
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

void AddLegacyAccountOptions::createButtons()
{
	QHBoxLayout *hblayout = new QHBoxLayout(ui.wdtGateways);
	hblayout->addStretch();

	IDiscoIdentity identity;
	identity.category = "gateway";

	QList<Jid> availGates = FGateways->availServices(FStreamJid,identity);
	qSort(availGates);

	foreach(Jid gateJid, availGates)
	{
		IGateRegisterLabel grlabel = FGateways->registerLabel(FStreamJid,gateJid);
		if (grlabel.valid)
		{
			QWidget *widget = new QWidget(ui.wdtGateways);
			widget->setLayout(new QVBoxLayout);
			widget->layout()->setMargin(0);

			QToolButton *button = new QToolButton(widget);
			button->setToolButtonStyle(Qt::ToolButtonIconOnly);
			button->setIconSize(QSize(32,32));
			button->setFixedSize(32,32);

			QLabel *label = new QLabel(grlabel.name,widget);
			label->setAlignment(Qt::AlignCenter);

			Action *action = new Action(button);
			action->setIcon(grlabel.icon);
			action->setText(grlabel.name);
			action->setData(ADR_GATEJID,gateJid.full());
			connect(action,SIGNAL(triggered(bool)),SLOT(onGateActionTriggeted(bool)));
			button->setDefaultAction(action);
			FGateActions.append(action);

			widget->layout()->addWidget(button);
			widget->layout()->addWidget(label);
			hblayout->addWidget(widget);
			hblayout->addStretch();
		}
	}

	updateButtons();
}

void AddLegacyAccountOptions::updateButtons()
{
	QList<Jid> usedGates = FGateways->streamServices(FStreamJid);
	foreach(Action *action, FGateActions)
	{
		Jid gateJid = action->data(ADR_GATEJID).toString();
		action->setEnabled(!usedGates.contains(gateJid));
	}
}

void AddLegacyAccountOptions::onGateActionTriggeted(bool)
{
	Action *action = qobject_cast<Action *>(sender());
	if (action)
	{
		Jid gateJid = action->data(ADR_GATEJID).toString();
		FGateways->showAddLegacyAccountDialog(FStreamJid,gateJid,this);
	}
}
