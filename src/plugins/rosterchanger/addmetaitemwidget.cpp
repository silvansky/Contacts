#include "addmetaitemwidget.h"

#include <QVBoxLayout>

#define RESOLVE_WAIT_INTERVAL    1500

AddMetaItemWidget::AddMetaItemWidget(IOptionsManager *AOptionsManager, IRoster *ARoster, IGateways *AGateways, const IGateServiceDescriptor &ADescriptor, QWidget *AParent) : QWidget(AParent)
{
	ui.setupUi(this);
	setFocusProxy(ui.lneContact);

	FRoster = ARoster;
	FGateways = AGateways;
	FOptionsManager = AOptionsManager;

	FServiceFailed = false;
	FContactTextChanged = false;
	FDescriptor = ADescriptor;

	ui.wdtProfiles->setLayout(new QVBoxLayout);
	ui.wdtProfiles->layout()->setMargin(0);

	FResolveTimer.setSingleShot(true);
	connect(&FResolveTimer,SIGNAL(timeout()),SLOT(resolveContactJid()));

	IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->insertAutoIcon(ui.lblIcon,ADescriptor.iconKey,0,0,"pixmap");
	IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->insertAutoIcon(ui.lblErrorIcon,MNI_RCHANGER_ADDMETACONTACT_ERROR,0,0,"pixmap");

	ui.lneContact->setAttribute(Qt::WA_MacShowFocusRect, false);
	ui.lneContact->setPlaceholderText(placeholderTextForGate());
	connect(ui.lneContact,SIGNAL(editingFinished()),SLOT(onContactTextEditingFinished()));
	connect(ui.lneContact,SIGNAL(textEdited(const QString &)),SLOT(onContactTextEdited(const QString &)));

	connect(ui.cbtDelete,SIGNAL(clicked()),SIGNAL(deleteButtonClicked()));

	connect(FGateways->instance(),SIGNAL(loginReceived(const QString &, const QString &)),
		SLOT(onServiceLoginReceived(const QString &, const QString &)));
	connect(FGateways->instance(),SIGNAL(userJidReceived(const QString &, const Jid &)),
		SLOT(onLegacyContactJidReceived(const QString &, const Jid &)));
	connect(FGateways->instance(),SIGNAL(errorReceived(const QString &, const QString &)),
		SLOT(onGatewayErrorReceived(const QString &, const QString &)));
	connect(FGateways->instance(),SIGNAL(streamServicesChanged(const Jid &)),
		SLOT(onStreamServicesChanged(const Jid &)));
	connect(FGateways->instance(),SIGNAL(serviceEnableChanged(const Jid &, const Jid &, bool)),
		SLOT(onServiceEnableChanged(const Jid &, const Jid &, bool)));
	connect(FGateways->instance(),SIGNAL(servicePresenceChanged(const Jid &, const Jid &, const IPresenceItem &)),
		SLOT(onServicePresenceChanged(const Jid &, const Jid &, const IPresenceItem &)));

	connect(FRoster->instance(),SIGNAL(opened()),SLOT(onRosterOpened()));
	connect(FRoster->instance(),SIGNAL(closed()),SLOT(onRosterClosed()));

	setErrorMessage(QString::null,false);
	updateProfiles();
}

AddMetaItemWidget::~AddMetaItemWidget()
{

}

QString AddMetaItemWidget::gateDescriptorId() const
{
	return FDescriptor.id;
}

Jid AddMetaItemWidget::streamJid() const
{
	return FRoster->streamJid();
}

Jid AddMetaItemWidget::contactJid() const
{
	return FContactJid;
}

void AddMetaItemWidget::setContactJid(const Jid &AContactJid)
{
	if (FContactJid != AContactJid.bare())
	{
		QString contact = AContactJid.bare();
		Jid serviceJid = AContactJid.domain();
		if (FGateways->streamServices(streamJid()).contains(serviceJid))
		{
			contact = FGateways->legacyIdFromUserJid(AContactJid);
			if (FProfiles.contains(serviceJid))
				FProfiles[serviceJid]->setChecked(true);
		}
		setContactText(contact);
	}
}

QString AddMetaItemWidget::contactText() const
{
	return ui.lneContact->text();
}

void AddMetaItemWidget::setContactText(const QString &AText)
{
	ui.lneContact->setText(AText);
	startResolve(0);
}

Jid AddMetaItemWidget::gatewayJid() const
{
	return selectedProfile();
}

void AddMetaItemWidget::setGatewayJid(const Jid &AGatewayJid) 
{
	setSelectedProfile(AGatewayJid);
}

QString AddMetaItemWidget::errorMessage() const
{
	return ui.lblError->isVisible() ? ui.lblError->text() : QString::null;
}

void AddMetaItemWidget::setErrorMessage(const QString &AMessage, bool AInvalidInput)
{
	if (ui.lblError->text() != AMessage)
	{
		ui.lblError->setText(AMessage);
		ui.lblError->setVisible(!AMessage.isEmpty());
		ui.lblErrorIcon->setVisible(!AMessage.isEmpty());
		ui.lneContact->setProperty("error", !AMessage.isEmpty() && AInvalidInput  ? true : false);
		setStyleSheet(styleSheet());
		emit adjustSizeRequested();
	}
}

bool AddMetaItemWidget::isServiceIconVisible() const
{
	return ui.lblIcon->isVisible();
}

void AddMetaItemWidget::setServiceIconVisible(bool AVisible)
{
	ui.lblIcon->setVisible(AVisible);
}

bool AddMetaItemWidget::isCloseButtonVisible() const
{
	return ui.cbtDelete->isVisible();
}

void AddMetaItemWidget::setCloseButtonVisible(bool AVisible)
{
	ui.cbtDelete->setVisible(AVisible);
}

void AddMetaItemWidget::setCorrectSizes(int ANameWidth, int APhotoWidth)
{
	ui.lblIcon->setMinimumWidth(ANameWidth);
	ui.cbtDelete->setMinimumWidth(APhotoWidth);
}

void AddMetaItemWidget::updateProfiles()
{
	IDiscoIdentity identity;
	identity.category = "gateway";
	identity.type = FDescriptor.type;

	QList<Jid> gates = FDescriptor.needLogin ? FGateways->streamServices(streamJid(),identity) : FGateways->availServices(streamJid(),identity);
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
	for (QMap<Jid,QRadioButton *>::const_iterator it=FProfiles.constBegin(); it!=FProfiles.constEnd(); it++)
	{
		Jid serviceJid = it.key();

		QRadioButton *button = FProfiles.value(serviceJid);
		QString login = FProfileLogins.value(serviceJid);
		button->setText(!login.isEmpty() ? login : serviceJid.pBare());

		if (streamJid() != serviceJid)
		{
			if (!FProfileLogins.contains(serviceJid) && !FLoginRequests.values().contains(serviceJid))
			{
				QString requestId = FGateways->sendLoginRequest(streamJid(),serviceJid);
				if (!requestId.isEmpty())
					FLoginRequests.insert(requestId,serviceJid);
			}

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

	if (!FRoster->isOpen())
	{
		FServiceFailed = true;
		setErrorMessage(tr("You are not connected to server."),false);
		ui.lneContact->setEnabled(false);
	}
	else if (FProfiles.isEmpty())
	{
		FServiceFailed = true;
		setErrorMessage(tr("Service '%1' is not available now.").arg(FDescriptor.name),false);
		ui.lneContact->setEnabled(false);
	}
	else if (FServiceFailed)
	{
		FServiceFailed = false;
		ui.lneContact->setEnabled(true);
		startResolve(0);
	}
	
	if (!FServiceFailed && selectedProfile().isEmpty())
	{
		if (!FDescriptor.needGate)
			setSelectedProfile(streamJid());
		else
			setSelectedProfile(enabledProfiles.value(0));
	}

	ui.wdtSelectProfile->setVisible(hasDisabledProfiles || FProfiles.count()>1);

	emit adjustSizeRequested();
}

Jid AddMetaItemWidget::selectedProfile() const
{
	for (QMap<Jid, QRadioButton *>::const_iterator it = FProfiles.constBegin(); it!=FProfiles.constEnd(); it++)
		if (it.value()->isChecked())
			return it.key();
	return Jid::null;
}

void AddMetaItemWidget::setSelectedProfile(const Jid &AServiceJid)
{
	if (FProfiles.contains(AServiceJid))
	{
		QRadioButton *button = FProfiles.value(AServiceJid);
		if (button->isEnabled() && !button->isChecked())
		{
			FProfiles.value(AServiceJid)->setChecked(true);
			startResolve(0);
		}
	}
}

QString AddMetaItemWidget::placeholderTextForGate() const
{
	QString text;
	if (FDescriptor.id == GSID_SMS)
		text = tr("Phone number (+7)");
	else if (FDescriptor.id == GSID_MAIL)
		text = tr("E-mail address");
	else
		text = tr("Address in %1").arg(FDescriptor.name);
	return text;
}

void AddMetaItemWidget::startResolve(int ATimeout)
{
	setRealContactJid(Jid::null);
	setErrorMessage(QString::null,false);
	FResolveTimer.start(ATimeout);
}

void AddMetaItemWidget::setRealContactJid(const Jid &AContactJid)
{
	if (FContactJid != AContactJid.bare())
	{
		FContactJid = AContactJid.bare();
		emit contactJidChanged(AContactJid);
	}
}

void AddMetaItemWidget::resolveContactJid()
{
	QString contact = FGateways->normalizeContactLogin(FDescriptor.id,contactText(),true);
	if (contactText() != contact)
		ui.lneContact->setText(contact);
	FContactTextChanged = false;

	QString errMessage;
	if (!contact.isEmpty())
	{
		errMessage = FGateways->checkNormalizedContactLogin(FDescriptor.id,contact);
		if (errMessage.isEmpty())
		{
			Jid serviceJid = selectedProfile();
			if (serviceJid.isEmpty())
			{
				errMessage = tr("Select your return address.");
			}
			else if (streamJid() != serviceJid)
			{
				FContactJidRequest = FGateways->sendUserJidRequest(streamJid(),serviceJid,contact);
				if (FContactJidRequest.isEmpty())
					errMessage = tr("Failed to request contact JID from transport.");
			}
			else if (FRoster->rosterItem(contact).isValid)
			{
				errMessage = tr("This contact is already present in your roster.");
			}
			else
			{
				setRealContactJid(contact);
			}
		}
	}

	setErrorMessage(errMessage,true);
}

void AddMetaItemWidget::onRosterOpened()
{
	updateProfiles();
}

void AddMetaItemWidget::onRosterClosed()
{
	updateProfiles();
}

void AddMetaItemWidget::onContactTextEditingFinished()
{
	if (FContactTextChanged)
		startResolve(0);
}

void AddMetaItemWidget::onContactTextEdited(const QString &AText)
{
	Q_UNUSED(AText);
	FContactTextChanged = true;
	startResolve(RESOLVE_WAIT_INTERVAL);
}

void AddMetaItemWidget::onProfileButtonToggled(bool)
{
	QRadioButton *button = qobject_cast<QRadioButton *>(sender());
	if (button && button->isChecked())
	{
		setSelectedProfile(FProfiles.key(button));
	}
}

void AddMetaItemWidget::onProfileLabelLinkActivated(const QString &ALink)
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
				FOptionsManager->showOptionsDialog(OPN_GATEWAYS_ACCOUNTS);
		}
	}
}

void AddMetaItemWidget::onServiceLoginReceived(const QString &AId, const QString &ALogin)
{
	if (FLoginRequests.contains(AId))
	{
		Jid serviceJid = FLoginRequests.take(AId);
		FProfileLogins.insert(serviceJid,ALogin);
		updateProfiles();
	}
}

void AddMetaItemWidget::onLegacyContactJidReceived(const QString &AId, const Jid &AUserJid)
{
	if (FContactJidRequest == AId)
	{
		if (!FRoster->rosterItem(AUserJid).isValid)
			setRealContactJid(AUserJid);
		else
			setErrorMessage(tr("This contact is already present in your roster."),true);
	}
}

void AddMetaItemWidget::onGatewayErrorReceived(const QString &AId, const QString &AError)
{
	Q_UNUSED(AError);
	if (FLoginRequests.contains(AId))
	{
		Jid serviceJid = FLoginRequests.take(AId);
		FProfileLogins.insert(serviceJid,QString::null);
		updateProfiles();
	}
	else if (FContactJidRequest == AId)
	{
		setRealContactJid(Jid::null);
		setErrorMessage(tr("Failed to request contact JID from transport."),false);
	}
}

void AddMetaItemWidget::onStreamServicesChanged(const Jid &AStreamJid)
{
	if (streamJid() == AStreamJid)
	{
		updateProfiles();
	}
}

void AddMetaItemWidget::onServiceEnableChanged(const Jid &AStreamJid, const Jid &AServiceJid, bool AEnabled)
{
	Q_UNUSED(AServiceJid); 
	Q_UNUSED(AEnabled);
	if (streamJid() == AStreamJid)
	{
		FProfileLogins.remove(AServiceJid);
		updateProfiles();
	}
}

void AddMetaItemWidget::onServicePresenceChanged(const Jid &AStreamJid, const Jid &AServiceJid, const IPresenceItem &AItem)
{
	Q_UNUSED(AItem);
	if (streamJid()==AStreamJid && FProfiles.contains(AServiceJid))
	{
		updateProfiles();
	}
}
