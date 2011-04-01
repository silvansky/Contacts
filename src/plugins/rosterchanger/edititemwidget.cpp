#include "edititemwidget.h"

#include <QVBoxLayout>

#define RESOLVE_WAIT_INTERVAL    1500

EditItemWidget::EditItemWidget(IGateways *AGateways, const Jid &AStreamJid, const IGateServiceDescriptor &ADescriptor, QWidget *AParent) : QWidget(AParent)
{
	ui.setupUi(this);
	setFocusProxy(ui.lneContact);

	FGateways = AGateways;

	FContactTextChanged = false;
	FStreamJid = AStreamJid;
	FDescriptor = ADescriptor;

	ui.wdtProfiles->setLayout(new QVBoxLayout);
	ui.wdtProfiles->layout()->setMargin(0);

	FResolveTimer.setSingleShot(true);
	connect(&FResolveTimer,SIGNAL(timeout()),SLOT(resolveContactJid()));

	connect(ui.lneContact,SIGNAL(editingFinished()),SLOT(onContactTextEditingFinished()));
	connect(ui.lneContact,SIGNAL(textEdited(const QString &)),SLOT(onContactTextEdited(const QString &)));

	IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->insertAutoIcon(ui.lblIcon,ADescriptor.iconKey,0,0,"pixmap");
	IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->insertAutoIcon(ui.lblErrorIcon,MNI_RCHANGER_ADDMETACONTACT_ERROR,0,0,"pixmap");

	ui.lneContact->setAttribute(Qt::WA_MacShowFocusRect, false);
	ui.lneContact->setPlaceholderText(tr("Address in %1").arg(ADescriptor.name));
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

	setErrorMessage(QString::null);
	updateProfiles();
}

EditItemWidget::~EditItemWidget()
{

}

Jid EditItemWidget::contactJid() const
{
	return FContactJid;
}

void EditItemWidget::setContactJid(const Jid &AContactJid)
{
	if (FContactJid != AContactJid.bare())
	{
		QString contact = AContactJid.bare();
		Jid serviceJid = AContactJid.domain();
		if (FGateways->streamServices(FStreamJid).contains(serviceJid))
		{
			contact = FGateways->legacyIdFromUserJid(AContactJid);
			if (FProfiles.contains(serviceJid))
				FProfiles[serviceJid]->setChecked(true);
		}
		setContactText(contact);
	}
}

QString EditItemWidget::contactText() const
{
	return ui.lneContact->text();
}

void EditItemWidget::setContactText(const QString &AText)
{
	ui.lneContact->setText(AText);
	startResolve(0);
}

Jid EditItemWidget::gatewayJid() const
{
	return selectedProfile();
}

void EditItemWidget::setGatewayJid(const Jid &AGatewayJid) 
{
	setSelectedProfile(AGatewayJid);
}

IGateServiceDescriptor EditItemWidget::gateDescriptor() const
{
	return FDescriptor;
}

void EditItemWidget::setCorrectSizes(int ANameWidth, int APhotoWidth)
{
	ui.lblIcon->setMinimumWidth(ANameWidth);
	ui.cbtDelete->setMinimumWidth(APhotoWidth);
}

void EditItemWidget::updateProfiles()
{
	IDiscoIdentity identity;
	identity.category = "gateway";
	identity.type = FDescriptor.type;

	QList<Jid> gates = FDescriptor.needLogin ? FGateways->streamServices(FStreamJid,identity) : FGateways->availServices(FStreamJid,identity);
	QList<Jid> newProfiles = (gates.toSet() - FProfiles.keys().toSet()).toList();
	QList<Jid> oldProfiles = (FProfiles.keys().toSet() - gates.toSet()).toList();

	qSort(newProfiles);
	if (!FDescriptor.needGate && !FProfiles.contains(FStreamJid))
		newProfiles.prepend(FStreamJid);
	else if (FDescriptor.needGate && FProfiles.contains(FStreamJid))
		oldProfiles.prepend(FStreamJid);
	else
		oldProfiles.removeAll(FStreamJid);

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
		QString login = FProfileLogins.value(serviceJid,serviceJid.pBare());
		button->setText(login);

		if (FStreamJid != serviceJid)
		{
			if (!FProfileLogins.contains(serviceJid) && !FLoginRequests.values().contains(serviceJid))
			{
				QString requestId = FGateways->sendLoginRequest(FStreamJid,serviceJid);
				if (!requestId.isEmpty())
					FLoginRequests.insert(requestId,serviceJid);
			}

			QString labelText;
			QLabel *label = FProfileLabels.value(serviceJid);
			if (FDescriptor.needLogin)
			{
				IPresenceItem pitem = FGateways->servicePresence(FStreamJid,serviceJid);
				if (FStreamJid!=serviceJid && !FGateways->isServiceEnabled(FStreamJid,serviceJid))
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

	if (selectedProfile().isEmpty() && !FProfiles.isEmpty())
	{
		if (!FDescriptor.needGate)
			setSelectedProfile(FStreamJid);
		else
			setSelectedProfile(enabledProfiles.value(0));
	}

	ui.wdtSelectProfile->setVisible(hasDisabledProfiles || FProfiles.count()>1);

	emit adjustSizeRequested();
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
		QRadioButton *button = FProfiles.value(AServiceJid);
		if (button->isEnabled() && !button->isChecked())
		{
			FProfiles.value(AServiceJid)->setChecked(true);
			startResolve(0);
		}
	}
}

void EditItemWidget::startResolve(int ATimeout)
{
	setRealContactJid(Jid::null);
	setErrorMessage(QString::null);
	FResolveTimer.start(ATimeout);
}

void EditItemWidget::setRealContactJid(const Jid &AContactJid)
{
	if (FContactJid != AContactJid.bare())
	{
		FContactJid = AContactJid.bare();
		emit contactJidChanged(AContactJid);
	}
}

void EditItemWidget::setErrorMessage(const QString &AMessage)
{
	if (ui.lblError->text() != AMessage)
	{
		ui.lblError->setText(AMessage);
		ui.lblError->setVisible(!AMessage.isEmpty());
		ui.lblErrorIcon->setVisible(!AMessage.isEmpty());
		ui.lneContact->setProperty("error", AMessage.isEmpty() ? false : true);
		setStyleSheet(styleSheet());
		emit adjustSizeRequested();
	}
}

void EditItemWidget::resolveContactJid()
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
				errMessage = tr("Select your return address");
			}
			else if (serviceJid != FStreamJid)
			{
				FContactJidRequest = FGateways->sendUserJidRequest(FStreamJid,serviceJid,contact);
				if (FContactJidRequest.isEmpty())
					errMessage = tr("Failed to request contact JID from transport");
			}
			else
			{
				setRealContactJid(contact);
			}
		}
	}

	setErrorMessage(errMessage);
}

void EditItemWidget::onContactTextEditingFinished()
{
	if (FContactTextChanged)
		startResolve(0);
}

void EditItemWidget::onContactTextEdited(const QString &AText)
{
	Q_UNUSED(AText);
	FContactTextChanged = true;
	startResolve(RESOLVE_WAIT_INTERVAL);
}

void EditItemWidget::onProfileButtonToggled(bool)
{
	QRadioButton *button = qobject_cast<QRadioButton *>(sender());
	if (button && button->isChecked())
	{
		setSelectedProfile(FProfiles.key(button));
	}
}

void EditItemWidget::onProfileLabelLinkActivated(const QString &ALink)
{
	QUrl url(ALink);
	Jid serviceJid = url.path();
	QLabel *label = FProfileLabels.value(serviceJid);
	if (label)
	{
		if (url.scheme() == "connect")
		{
			if (FGateways->setServiceEnabled(FStreamJid,serviceJid,true))
				label->setEnabled(false);
		}
		else if (url.scheme() == "options")
		{
			emit showOptionsRequested();
		}
	}
}

void EditItemWidget::onServiceLoginReceived(const QString &AId, const QString &ALogin)
{
	if (FLoginRequests.contains(AId))
	{
		Jid serviceJid = FLoginRequests.take(AId);
		FProfileLogins.insert(serviceJid,ALogin);
		updateProfiles();
	}
}

void EditItemWidget::onLegacyContactJidReceived(const QString &AId, const Jid &AUserJid)
{
	if (FContactJidRequest == AId)
	{
		setRealContactJid(AUserJid);
	}
}

void EditItemWidget::onGatewayErrorReceived(const QString &AId, const QString &AError)
{
	Q_UNUSED(AError);
	if (FLoginRequests.contains(AId))
	{
		Jid serviceJid = FLoginRequests.take(AId);
		FProfileLogins.remove(serviceJid);
		updateProfiles();
	}
	else if (FContactJidRequest == AId)
	{
		setRealContactJid(Jid::null);
		setErrorMessage(tr("Failed to request contact JID from transport"));
	}
}

void EditItemWidget::onStreamServicesChanged(const Jid &AStreamJid)
{
	if (FStreamJid == AStreamJid)
	{
		updateProfiles();
	}
}

void EditItemWidget::onServiceEnableChanged(const Jid &AStreamJid, const Jid &AServiceJid, bool AEnabled)
{
	Q_UNUSED(AServiceJid); 
	Q_UNUSED(AEnabled);
	if (AStreamJid == FStreamJid)
	{
		FProfileLogins.remove(AServiceJid);
		updateProfiles();
	}
}

void EditItemWidget::onServicePresenceChanged(const Jid &AStreamJid, const Jid &AServiceJid, const IPresenceItem &AItem)
{
	Q_UNUSED(AItem);
	if (FStreamJid == AStreamJid && FProfiles.contains(AServiceJid))
	{
		updateProfiles();
	}
}
