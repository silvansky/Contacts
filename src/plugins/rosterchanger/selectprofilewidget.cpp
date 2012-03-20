#include "selectprofilewidget.h"

#include <QVBoxLayout>
SelectProfileWidget::SelectProfileWidget(IRoster *ARoster, IGateways *AGateways, IOptionsManager *AOptionsManager, const IGateServiceDescriptor &ADescriptor, QWidget *AParent) : QWidget(AParent)
{
	ui.setupUi(this);

	FVisible = true;
	FAutoSelect = true;
	FRoster = ARoster;
	FGateways = AGateways;
	FOptionsManager = AOptionsManager;
	FDescriptor = ADescriptor;

	ui.wdtProfiles->setLayout(new QVBoxLayout);
	ui.wdtProfiles->layout()->setMargin(0);

	connect(FRoster->instance(),SIGNAL(opened()),SLOT(onRosterOpened()));
	connect(FRoster->instance(),SIGNAL(closed()),SLOT(onRosterClosed()));

	connect(FGateways->instance(),SIGNAL(loginReceived(const QString &, const QString &)),
		SLOT(onServiceLoginReceived(const QString &, const QString &)));
	connect(FGateways->instance(),SIGNAL(errorReceived(const QString &, const QString &)),
		SLOT(onGatewayErrorReceived(const QString &, const QString &)));
	connect(FGateways->instance(),SIGNAL(streamServicesChanged(const Jid &)),
		SLOT(onStreamServicesChanged(const Jid &)));
	connect(FGateways->instance(),SIGNAL(serviceEnableChanged(const Jid &, const Jid &, bool)),
		SLOT(onServiceEnableChanged(const Jid &, const Jid &, bool)));
	connect(FGateways->instance(),SIGNAL(servicePresenceChanged(const Jid &, const Jid &, const IPresenceItem &)),
		SLOT(onServicePresenceChanged(const Jid &, const Jid &, const IPresenceItem &)));

	updateProfiles();
}

SelectProfileWidget::~SelectProfileWidget()
{

}

Jid SelectProfileWidget::streamJid() const
{
	return FRoster->streamJid();
}

QList<Jid> SelectProfileWidget::profiles() const
{
	return FProfiles.keys();
}

Jid SelectProfileWidget::selectedProfile() const
{
	for (QMap<Jid, QRadioButton *>::const_iterator it = FProfiles.constBegin(); it!=FProfiles.constEnd(); it++)
		if (it.value()->isChecked() && it.value()->isEnabled())
			return it.key();
	return Jid::null;
}

void SelectProfileWidget::setSelectedProfile(const Jid &AServiceJid)
{
	QRadioButton *button = FProfiles.value(AServiceJid);
	if (button && button->isEnabled())
	{
		button->blockSignals(true);
		button->setChecked(true);
		button->blockSignals(false);
		emit selectedProfileChanged();
	}
}

bool SelectProfileWidget::autoSelectProfile() const
{
	return FAutoSelect;
}

void SelectProfileWidget::setAutoSelectProfile(bool AAuto)
{
	if (FAutoSelect != AAuto)
	{
		FAutoSelect = AAuto;
		updateProfiles();
	}
}

void SelectProfileWidget::updateProfiles()
{
	IDiscoIdentity identity;
	identity.category = "gateway";
	identity.type = FDescriptor.type;

	QList<Jid> nativeGates;
	if (FDescriptor.id == GSID_RAMBLER)
		nativeGates.append(streamJid());

	QList<Jid> gates = FGateways->gateDescriptorServices(streamJid(),FDescriptor,FDescriptor.needLogin);
	for (QList<Jid>::iterator it=gates.begin(); it!=gates.end(); )
	{
		IGateServiceDescriptor descriptor = FGateways->serviceDescriptor(streamJid(),*it);
		if (!descriptor.readOnly)
		{
			if (FDescriptor.id == descriptor.id)
				nativeGates.append(*it);
			it++;
		}
		else
		{
			it = gates.erase(it);
		}
	}

	Jid oldSelected = selectedProfile();
	QList<Jid> newProfiles = (gates.toSet() - FProfiles.keys().toSet()).toList();
	QList<Jid> oldProfiles = (FProfiles.keys().toSet() - gates.toSet()).toList();

	qSort(newProfiles);
	if (!FDescriptor.needGate && !FProfiles.contains(streamJid()))
		newProfiles.prepend(streamJid());
	else if (FDescriptor.needGate && FProfiles.contains(streamJid()))
		oldProfiles.prepend(streamJid());
	else
		oldProfiles.removeAll(streamJid());

	foreach(Jid serviceJid, newProfiles)
	{
		QRadioButton *button = new QRadioButton(ui.wdtProfiles);
		button->setAutoExclusive(true);
		connect(button,SIGNAL(toggled(bool)),SLOT(onProfileButtonToggled(bool)));
		FProfiles.insert(serviceJid,button);

		QLabel *label = new QLabel(ui.wdtProfiles);
		connect(label,SIGNAL(linkActivated(const QString &)),SLOT(onProfileLabelLinkActivated(const QString &)));
		FProfileLabels.insert(serviceJid,label);

		QHBoxLayout *layout = new QHBoxLayout();
		layout->setMargin(0);
		layout->setSpacing(0);
		layout->addWidget(button);
		layout->addWidget(label);
		layout->addStretch();
		qobject_cast<QVBoxLayout *>(ui.wdtProfiles->layout())->addLayout(layout);
	}

	foreach(Jid serviceJid, oldProfiles)
	{
		FProfileLogins.remove(serviceJid);
		delete FProfiles.take(serviceJid);
		delete FProfileLabels.take(serviceJid);
	}

	QList<Jid> enabledProfiles;
	bool hasDisabledProfiles = false;
	bool hasChangedProfiles = false;
	for (QMap<Jid,QRadioButton *>::const_iterator it=FProfiles.constBegin(); it!=FProfiles.constEnd(); it++)
	{
		Jid serviceJid = it.key();

		QRadioButton *button = FProfiles.value(serviceJid);
		QString login = FProfileLogins.value(serviceJid);
		button->setText(!login.isEmpty() ? login : serviceJid.pBare());

		if (streamJid() != serviceJid)
		{
			if (!FLoginRequests.values().contains(serviceJid) && (FUpdateLogins.contains(serviceJid) || !FProfileLogins.contains(serviceJid)))
			{
				QString requestId = FGateways->sendLoginRequest(streamJid(),serviceJid);
				if (!requestId.isEmpty())
					FLoginRequests.insert(requestId,serviceJid);
			}
			FUpdateLogins.removeAll(serviceJid);

			QString labelText;
			QLabel *label = FProfileLabels.value(serviceJid);
			if (FDescriptor.needLogin)
			{
				IPresenceItem pitem = FGateways->servicePresence(streamJid(),serviceJid);
				if (streamJid()!=serviceJid && !FGateways->isServiceEnabled(streamJid(),serviceJid))
					labelText = " - " + tr("disconnected. %1").arg("<a href='connect:%1'>%2</a>").arg(serviceJid.pFull()).arg(tr("Connect"));
				else if (pitem.show == IPresence::Error)
					labelText = " - " + tr("failed to connect. %1").arg("<a href='options:%1'>%2</a>").arg(serviceJid.pFull()).arg(tr("Options"));
				else if (pitem.show == IPresence::Offline)
					labelText = " - " + tr("connecting...");
			}

			if (labelText != label->text())
				hasChangedProfiles = true;

			if (!labelText.isEmpty())
			{
				label->setText(labelText);
				button->setEnabled(false);
				button->setChecked(false);
				hasDisabledProfiles = true;
			}
			else
			{
				label->setText(QString::null);
				button->setEnabled(true);
				enabledProfiles.append(serviceJid);
			}
			label->setEnabled(true);
		}
	}

	bool autoSelected = false;
	if (FAutoSelect && nativeGates.count()==1)
	{
		setSelectedProfile(nativeGates.first());
		autoSelected = selectedProfile()==nativeGates.first();
	}

	Jid newSelected = selectedProfile();
	if (newSelected.isEmpty())
	{
		if (!FDescriptor.needGate)
			setSelectedProfile(streamJid());
		else if (!enabledProfiles.isEmpty())
			setSelectedProfile(enabledProfiles.value(0));
		else if (oldSelected != newSelected)
			emit selectedProfileChanged();
	}

	bool adjustSize = hasChangedProfiles;
	bool newVisible = !autoSelected && (hasDisabledProfiles || FProfiles.count()>1);
	if (FVisible != newVisible)
	{
		adjustSize = true;
		FVisible = newVisible;
		setVisible(FVisible);
	}

	if (!newProfiles.isEmpty() || !oldProfiles.isEmpty())
	{
		adjustSize = true;
		emit profilesChanged();
	}
	
	if (adjustSize)
	{
		emit adjustSizeRequested();
	}
}

void SelectProfileWidget::onRosterOpened()
{
	updateProfiles();
}

void SelectProfileWidget::onRosterClosed()
{
	updateProfiles();
}

void SelectProfileWidget::onProfileButtonToggled(bool AChecked)
{
	if (AChecked)
		setSelectedProfile(FProfiles.key(qobject_cast<QRadioButton *>(sender())));
}

void SelectProfileWidget::onProfileLabelLinkActivated(const QString &ALink)
{
	QUrl url(ALink);
	Jid serviceJid = url.path();
	QLabel *label = FProfileLabels.value(serviceJid);
	if (label)
	{
		if (url.scheme() == "connect")
		{
			if (FGateways->setServiceEnabled(streamJid(),serviceJid,true))
				label->setEnabled(false);
		}
		else if (url.scheme() == "options")
		{
			if (FOptionsManager)
				FOptionsManager->showOptionsDialog(OPN_GATEWAYS);
		}
	}
}

void SelectProfileWidget::onServiceLoginReceived(const QString &AId, const QString &ALogin)
{
	if (FLoginRequests.contains(AId))
	{
		Jid serviceJid = FLoginRequests.take(AId);
		FProfileLogins.insert(serviceJid,ALogin);
		updateProfiles();
	}
}

void SelectProfileWidget::onGatewayErrorReceived(const QString &AId, const QString &AError)
{
	Q_UNUSED(AError);
	if (FLoginRequests.contains(AId))
	{
		Jid serviceJid = FLoginRequests.take(AId);
		FProfileLogins.insert(serviceJid,QString::null);
		updateProfiles();
	}
}

void SelectProfileWidget::onStreamServicesChanged(const Jid &AStreamJid)
{
	if (streamJid() == AStreamJid)
	{
		updateProfiles();
	}
}

void SelectProfileWidget::onServiceEnableChanged(const Jid &AStreamJid, const Jid &AServiceJid, bool AEnabled)
{
	Q_UNUSED(AEnabled);
	Q_UNUSED(AServiceJid); 
	if (streamJid() == AStreamJid)
	{
		FUpdateLogins.append(AServiceJid);
		updateProfiles();
	}
}

void SelectProfileWidget::onServicePresenceChanged(const Jid &AStreamJid, const Jid &AServiceJid, const IPresenceItem &AItem)
{
	Q_UNUSED(AItem);
	if (streamJid()==AStreamJid && FProfiles.contains(AServiceJid))
	{
		updateProfiles();
	}
}
