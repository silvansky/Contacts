#include "edititemwidget.h"

#include <QVBoxLayout>

EditItemWidget::EditItemWidget(IGateways *AGateways, const Jid &AStreamJid, const IGateServiceDescriptor &ADescriptor, QWidget *AParent) : QWidget(AParent)
{
	ui.setupUi(this);

	FGateways = AGateways;
	FStreamJid = AStreamJid;
	FDescriptor = ADescriptor;

	ui.wdtProfiles->setLayout(new QVBoxLayout);
	ui.wdtProfiles->layout()->setMargin(0);

	IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->insertAutoIcon(ui.lblIcon,ADescriptor.iconKey,0,0,"pixmap");
	ui.lneContact->setPlaceholderText(tr("Address in %1").arg(ADescriptor.name));
	connect(ui.cbtDelete,SIGNAL(clicked()),SIGNAL(deleteButtonClicked()));

	connect(FGateways->instance(),SIGNAL(loginReceived(const QString &, const QString &)),SLOT(onServiceLoginReceived(const QString &, const QString &)));
	connect(FGateways->instance(),SIGNAL(userJidReceived(const QString &, const Jid &)),SLOT(onLegacyContactJidReceived(const QString &, const Jid &)));
	connect(FGateways->instance(),SIGNAL(serviceEnableChanged(const Jid &, const Jid &, bool)),SLOT(onServiceEnableChanged(const Jid &, const Jid &, bool)));
	connect(FGateways->instance(),SIGNAL(errorReceived(const QString &, const QString &)),SLOT(onGatewayErrorReceived(const QString &, const QString &)));

	updateProfiles();
}

EditItemWidget::~EditItemWidget()
{

}

void EditItemWidget::updateProfiles()
{
	IDiscoIdentity identity;
	identity.category = "gateway";
	identity.type = FDescriptor.type;
	QList<Jid> gates = FDescriptor.needLogin ? FGateways->streamServices(FStreamJid,identity) : FGateways->availServices(FStreamJid,identity);

	QSet<Jid> newProfiles = gates.toSet() - FProfiles.keys().toSet();
	QSet<Jid> oldProfiles = FProfiles.keys().toSet() - gates.toSet();

	if (!FDescriptor.needGate && !FProfiles.contains(FStreamJid))
		newProfiles += FStreamJid;
	else if (FDescriptor.needGate && FProfiles.contains(FStreamJid))
		oldProfiles += FStreamJid;

	foreach(Jid serviceJid, newProfiles)
	{
		QRadioButton *button = new QRadioButton(ui.wdtProfiles);
		button->setText(serviceJid.pBare());
		button->setAutoExclusive(true);
		FProfiles.insert(serviceJid,button);
		ui.wdtProfiles->layout()->addWidget(button);
	}

	foreach(Jid serviceJid, oldProfiles)
	{
		QRadioButton *button = FProfiles.take(serviceJid);
		ui.wdtProfiles->layout()->removeWidget(button);
		delete button;
	}

	if (selectedProfile().isEmpty() && !FProfiles.isEmpty())
	{
		if (!FDescriptor.needGate)
			setSelectedProfile(FStreamJid);
		else
			setSelectedProfile(FProfiles.keys().value(0));
	}

	ui.wdtSelectProfile->setVisible(FProfiles.count() > 1);
}

Jid EditItemWidget::selectedProfile() const
{
	for (QMap<Jid, QRadioButton *>::const_iterator it = FProfiles.constBegin(); it!=FProfiles.constEnd(); it++)
		if (it.value()->isChecked())
			return it.key();
	return Jid::null;
}

void EditItemWidget::setSelectedProfile(const Jid &AServiceJid)
{
	if (FProfiles.contains(AServiceJid))
	{
		FProfiles.value(AServiceJid)->setChecked(true);
	}
}

void EditItemWidget::onServiceLoginReceived(const QString &AId, const QString &ALogin)
{
	Q_UNUSED(AId); Q_UNUSED(ALogin);
}

void EditItemWidget::onLegacyContactJidReceived(const QString &AId, const Jid &AUserJid)
{
	Q_UNUSED(AId); Q_UNUSED(AUserJid);
}

void EditItemWidget::onGatewayErrorReceived(const QString &AId, const QString &AError)
{
	Q_UNUSED(AId); Q_UNUSED(AError);
}

void EditItemWidget::onServiceEnableChanged(const Jid &AStreamJid, const Jid &AServiceJid, bool AEnabled)
{
	Q_UNUSED(AServiceJid); Q_UNUSED(AEnabled);
	if (AStreamJid == FStreamJid)
	{
		updateProfiles();
	}
}
