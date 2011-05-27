#include "addmetaitemwidget.h"

#include <QVBoxLayout>
#include <utils/stylestorage.h>

#define RESOLVE_WAIT_INTERVAL    1500

AddMetaItemWidget::AddMetaItemWidget(IOptionsManager *AOptionsManager, IRoster *ARoster, IGateways *AGateways, const IGateServiceDescriptor &ADescriptor, QWidget *AParent) : QWidget(AParent)
{
	ui.setupUi(this);
	setFocusProxy(ui.lneContact);

	FRoster = ARoster;
	FGateways = AGateways;

	FServiceFailed = false;
	FContactTextChanged = false;
	FDescriptor = ADescriptor;

	FSelectProfileWidget = new SelectProfileWidget(FRoster,FGateways,AOptionsManager,FDescriptor,ui.wdtSelectProfile);
	connect(FSelectProfileWidget,SIGNAL(profilesChanged()),SLOT(onProfilesChanged()));
	connect(FSelectProfileWidget,SIGNAL(selectedProfileChanged()),SLOT(onSelectedProfileChanched()));
	connect(FSelectProfileWidget,SIGNAL(adjustSizeRequested()),SIGNAL(adjustSizeRequested()));

	ui.wdtSelectProfile->setLayout(new QVBoxLayout);
	ui.wdtSelectProfile->layout()->setMargin(0);
	ui.wdtSelectProfile->layout()->addWidget(FSelectProfileWidget);

	FResolveTimer.setSingleShot(true);
	connect(&FResolveTimer,SIGNAL(timeout()),SLOT(resolveContactJid()));

	IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->insertAutoIcon(ui.lblIcon,ADescriptor.iconKey,0,0,"pixmap");
	IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->insertAutoIcon(ui.lblErrorIcon,MNI_RCHANGER_ADDMETACONTACT_ERROR,0,0,"pixmap");

	ui.lneContact->setAttribute(Qt::WA_MacShowFocusRect, false);
	ui.lneContact->setPlaceholderText(placeholderTextForGate());
	connect(ui.lneContact,SIGNAL(editingFinished()),SLOT(onContactTextEditingFinished()));
	connect(ui.lneContact,SIGNAL(textEdited(const QString &)),SLOT(onContactTextEdited(const QString &)));

	connect(ui.cbtDelete,SIGNAL(clicked()),SIGNAL(deleteButtonClicked()));

	connect(FGateways->instance(),SIGNAL(userJidReceived(const QString &, const Jid &)),
		SLOT(onLegacyContactJidReceived(const QString &, const Jid &)));
	connect(FGateways->instance(),SIGNAL(errorReceived(const QString &, const QString &)),
		SLOT(onGatewayErrorReceived(const QString &, const QString &)));

	setErrorMessage(QString::null,false);
	onProfilesChanged();
	onSelectedProfileChanched();
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
		if (FGateways->availServices(streamJid()).contains(serviceJid))
		{
			contact = FGateways->legacyIdFromUserJid(AContactJid);
			FSelectProfileWidget->setSelectedProfile(serviceJid);
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
	return FSelectProfileWidget->selectedProfile();
}

void AddMetaItemWidget::setGatewayJid(const Jid &AGatewayJid)
{
	FSelectProfileWidget->setSelectedProfile(AGatewayJid);
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
		StyleStorage::updateStyle(this);
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
	QString errMessage;

	QString contact = FGateways->normalizedContactLogin(FDescriptor,contactText(),true);
	if (contactText() != contact)
		ui.lneContact->setText(contact);
	FContactTextChanged = false;

	if (!contact.isEmpty())
	{
		errMessage = FGateways->checkNormalizedContactLogin(FDescriptor,contact);
		if (errMessage.isEmpty())
		{
			Jid serviceJid = FSelectProfileWidget->selectedProfile();
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
				errMessage = tr("This contact is already present in your contact-list.");
			}
			else
			{
				setRealContactJid(contact);
			}
		}
	}

	setErrorMessage(errMessage,true);
}

void AddMetaItemWidget::onProfilesChanged()
{
	if (!FRoster->isOpen())
	{
		FServiceFailed = true;
		setErrorMessage(tr("You are not connected to server."),false);
		ui.lneContact->setEnabled(false);
	}
	else if (FSelectProfileWidget->profiles().isEmpty())
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
}

void AddMetaItemWidget::onSelectedProfileChanched()
{
	if (!FSelectProfileWidget->selectedProfile().isEmpty())
		startResolve(0);
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

void AddMetaItemWidget::onLegacyContactJidReceived(const QString &AId, const Jid &AUserJid)
{
	if (FContactJidRequest == AId)
	{
		if (!FRoster->rosterItem(AUserJid).isValid)
			setRealContactJid(AUserJid);
		else
			setErrorMessage(tr("This contact is already present in your contact-list."),true);
	}
}

void AddMetaItemWidget::onGatewayErrorReceived(const QString &AId, const QString &AError)
{
	Q_UNUSED(AError);
	if (FContactJidRequest == AId)
	{
		setRealContactJid(Jid::null);
		setErrorMessage(tr("Failed to request contact JID from transport."),false);
	}
}
