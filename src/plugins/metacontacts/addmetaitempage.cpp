#include "addmetaitempage.h"

#include <QVBoxLayout>

AddMetaItemPage::AddMetaItemPage(IRosterChanger *ARosterChanger, IMetaTabWindow *AMetaTabWindow, IMetaRoster *AMetaRoster, const QString &AMetaId, const IMetaItemDescriptor &ADescriptor, QWidget *AParent) : QWidget(AParent)
{
	ui.setupUi(this);

	FMetaRoster = AMetaRoster;
	FMetaTabWindow = AMetaTabWindow;
	FRosterChanger = ARosterChanger;

	FMetaId = AMetaId;
	FDescriptor = ADescriptor;

	ui.lblInfo->setText(infoMessageByGate(ADescriptor.gateId));
	IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->insertAutoIcon(ui.lblErrorIcon,MNI_RCHANGER_ADDMETACONTACT_ERROR,0,0,"pixmap");

	FAddWidget = FRosterChanger->newAddMetaItemWidget(FMetaRoster->streamJid(),ADescriptor.gateId,ui.wdtAddMetaItem);
	FAddWidget->setCloseButtonVisible(false);
	connect(FAddWidget->instance(),SIGNAL(showOptionsRequested()),SLOT(onItemWidgetShowOptionsRequested()));
	connect(FAddWidget->instance(),SIGNAL(contactJidChanged(const Jid &)),SLOT(onItemWidgetContactJidChanged(const Jid &)));

	ui.wdtAddMetaItem->setLayout(new QVBoxLayout);
	ui.wdtAddMetaItem->layout()->addWidget(FAddWidget->instance());

	connect(ui.pbtAppend,SIGNAL(clicked()),SLOT(onAppendContactButtonClicked()));

	connect(FMetaRoster->instance(),SIGNAL(metaContactReceived(const IMetaContact &, const IMetaContact &)),
		SLOT(onMetaContactReceived(const IMetaContact &, const IMetaContact &)));
	connect(FMetaRoster->instance(),SIGNAL(metaActionResult(const QString &, const QString &, const QString &)),
		SLOT(onMetaActionResult(const QString &, const QString &, const QString &)));

	setErrorMessage(QString::null);
}

AddMetaItemPage::~AddMetaItemPage()
{
	emit tabPageDestroyed();
}

void AddMetaItemPage::showTabPage()
{
	emit tabPageShow();
}

void AddMetaItemPage::closeTabPage()
{
	emit tabPageClose();
}

bool AddMetaItemPage::isActive() const
{
	const QWidget *widget = this;
	while (widget->parentWidget())
		widget = widget->parentWidget();
	return isVisible() && widget->isActiveWindow() && !widget->isMinimized() && widget->isVisible();
}

QString AddMetaItemPage::tabPageId() const
{
	return "AddMetaTabPage|"+FMetaRoster->streamJid().pBare()+"|"+FMetaId;
}

QIcon AddMetaItemPage::tabPageIcon() const
{
	return windowIcon();
}

QString AddMetaItemPage::tabPageCaption() const
{
	return windowIconText();
}

QString AddMetaItemPage::tabPageToolTip() const
{
	return QString::null;
}

ITabPageNotifier *AddMetaItemPage::tabPageNotifier() const
{
	return NULL;
}

void AddMetaItemPage::setTabPageNotifier(ITabPageNotifier *ANotifier)
{
	Q_UNUSED(ANotifier);
}

QString AddMetaItemPage::infoMessageByGate(const QString &AGateId)
{
	QString info;
	if (AGateId == GSID_SMS)
		info = tr("Enter the phone number of the interlocutor, to send the SMS");
	else
		info = tr("Enter contact %1 address").arg(FDescriptor.name);
	return info;
}

void AddMetaItemPage::setErrorMessage(const QString &AMessage)
{
	if (ui.lblError->text() != AMessage)
	{
		ui.lblError->setText(AMessage);
		ui.lblError->setVisible(!AMessage.isEmpty());
		ui.lblErrorIcon->setVisible(!AMessage.isEmpty());
	}
	ui.wdtAddMetaItem->setEnabled(true);
	ui.pbtAppend->setEnabled(FAddWidget->contactJid().isValid());
}

bool AddMetaItemPage::event(QEvent *AEvent)
{
	if (AEvent->type() == QEvent::WindowActivate)
	{
		emit tabPageActivated();
	}
	else if (AEvent->type() == QEvent::WindowDeactivate)
	{
		emit tabPageDeactivated();
	}
	return QWidget::event(AEvent);
}

void AddMetaItemPage::showEvent(QShowEvent *AEvent)
{
	QWidget::showEvent(AEvent);
	emit tabPageActivated();
}

void AddMetaItemPage::closeEvent(QCloseEvent *AEvent)
{
	QWidget::closeEvent(AEvent);
	emit tabPageClosed();
}

void AddMetaItemPage::onAppendContactButtonClicked()
{
	if (FAddWidget->contactJid().isValid())
	{
		IMetaContact contact;
		contact.items += FAddWidget->contactJid();
		FCreateRequestId = FMetaRoster->createContact(contact);
		if (!FCreateRequestId.isEmpty())
		{
			ui.wdtAddMetaItem->setEnabled(false);
			ui.pbtAppend->setEnabled(false);
		}
		else
		{
			setErrorMessage(tr("Failed to create new contact"));
		}
	}
}

void AddMetaItemPage::onItemWidgetShowOptionsRequested()
{

}

void AddMetaItemPage::onItemWidgetContactJidChanged(const Jid &AContactJid)
{
	Q_UNUSED(AContactJid);
	setErrorMessage(QString::null);
}

void AddMetaItemPage::onMetaContactReceived(const IMetaContact &AContact, const IMetaContact &ABefore)
{
	Q_UNUSED(ABefore);
	if (AContact.id!=FMetaId && AContact.items.contains(FAddWidget->contactJid()))
	{
		FRosterChanger->subscribeContact(FMetaRoster->streamJid(),FAddWidget->contactJid());
		FMergeRequestId = FMetaRoster->mergeContacts(FMetaId, QList<QString>() << AContact.id);
		if (FMergeRequestId.isEmpty())
			setErrorMessage(tr("Failed to merge contacts"));
	}
	else if (AContact.id==FMetaId && AContact.items.contains(FAddWidget->contactJid()))
	{
		FMetaTabWindow->setCurrentItem(FAddWidget->contactJid());
	}
}

void AddMetaItemPage::onMetaActionResult(const QString &AActionId, const QString &AErrCond, const QString &AErrMessage)
{
	Q_UNUSED(AErrMessage);
	if (AActionId == FCreateRequestId)
	{
		if (!AErrCond.isEmpty())
			setErrorMessage(tr("Failed to create new contact"));
	}
	else if (AActionId == FMergeRequestId)
	{
		if (!AErrCond.isEmpty())
			setErrorMessage(tr("Failed to merge contacts"));
	}
}