#include "addmetaitempage.h"

#include <QPainter>
#include <QPaintEvent>
#include <QVBoxLayout>

AddMetaItemPage::AddMetaItemPage(IPluginManager *APluginManager, IMetaTabWindow *AMetaTabWindow, const IMetaItemDescriptor &ADescriptor, QWidget *AParent) : QWidget(AParent)
{
	ui.setupUi(this);
	StyleStorage::staticStorage(RSR_STORAGE_STYLESHEETS)->insertAutoStyle(this,STS_METACONTACTS_ADDMETAITEMPAGE);

	FMetaTabWindow = AMetaTabWindow;
	FDescriptor = ADescriptor;
	
	FRosterChanger = NULL;
	FMessageProcessor = NULL;
	initialize(APluginManager);

	ui.lblInfo->setText(infoMessageForGate());

	FAddWidget = FRosterChanger->newAddMetaItemWidget(FMetaTabWindow->metaRoster()->streamJid(),ADescriptor.gateId,ui.wdtAddMetaItem);
	if (FAddWidget)
	{
		FAddWidget->setErrorClickable(true);
		FAddWidget->setServiceIconVisible(false);
		FAddWidget->setCloseButtonVisible(false);
		connect(FAddWidget->instance(),SIGNAL(errorMessageClicked()),SLOT(onItemWidgetErrorMessageClicked()));
		connect(FAddWidget->instance(),SIGNAL(contactJidChanged()),SLOT(onItemWidgetContactJidChanged()));

		ui.wdtAddMetaItem->setLayout(new QVBoxLayout);
		ui.wdtAddMetaItem->layout()->setMargin(0);
		ui.wdtAddMetaItem->layout()->addWidget(FAddWidget->instance());
	}

	ui.pbtAppend->setEnabled(false);
	ui.pbtAppend->addTextFlag(TF_LIGHTSHADOW);

	connect(ui.pbtAppend,SIGNAL(clicked()),SLOT(onAppendContactButtonClicked()));

	connect(FMetaTabWindow->metaRoster()->instance(),SIGNAL(metaContactReceived(const IMetaContact &, const IMetaContact &)),
		SLOT(onMetaContactReceived(const IMetaContact &, const IMetaContact &)));
	connect(FMetaTabWindow->metaRoster()->instance(),SIGNAL(metaActionResult(const QString &, const QString &, const QString &)),
		SLOT(onMetaActionResult(const QString &, const QString &, const QString &)));
}

AddMetaItemPage::~AddMetaItemPage()
{
	emit tabPageDestroyed();
}

void AddMetaItemPage::assignTabPage()
{
	emit tabPageAssign();	
}

void AddMetaItemPage::showTabPage()
{
	emit tabPageShow();
}

void AddMetaItemPage::showMinimizedTabPage()
{
	emit tabPageShowMinimized();
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
	return "AddMetaTabPage|"+FMetaTabWindow->metaRoster()->streamJid().pBare()+"|"+FMetaTabWindow->metaId();
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

void AddMetaItemPage::initialize(IPluginManager *APluginManager)
{
	IPlugin *plugin = APluginManager->pluginInterface("IRosterChanger").value(0,NULL);
	if (plugin)
		FRosterChanger = qobject_cast<IRosterChanger *>(plugin->instance());

	plugin = APluginManager->pluginInterface("IMessageProcessor").value(0,NULL);
	if (plugin)
		FMessageProcessor = qobject_cast<IMessageProcessor *>(plugin->instance());
}

QString AddMetaItemPage::infoMessageForGate()
{
	QString info;
	if (FDescriptor.gateId == GSID_SMS)
		info = tr("Enter the phone number of the interlocutor, to send the SMS:");
	else if (FDescriptor.gateId == GSID_MAIL)
		info = tr("Enter e-mail address of contact:");
	else
		info = tr("Enter contact %1 address:").arg(FDescriptor.name);
	return info;
}

void AddMetaItemPage::setErrorMessage(const QString &AMessage)
{
	if (FAddWidget)
	{
		ui.wdtAddMetaItem->setEnabled(true);
		FAddWidget->setErrorMessage(AMessage,false);
		ui.pbtAppend->setEnabled(FAddWidget->isContactJidReady());
	}
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
	if (isActive())
		emit tabPageActivated();
}

void AddMetaItemPage::closeEvent(QCloseEvent *AEvent)
{
	QWidget::closeEvent(AEvent);
	emit tabPageClosed();
}

void AddMetaItemPage::paintEvent(QPaintEvent *AEvent)
{
	QStyleOption opt;
	opt.init(this);
	QPainter p(this);
	p.setClipRect(AEvent->rect());
	style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
}

void AddMetaItemPage::onAppendContactButtonClicked()
{
	if (FAddWidget && FAddWidget->isContactJidReady())
	{
		IMetaContact contact;
		contact.items += FAddWidget->contactJid();
		FCreateRequestId = FMetaTabWindow->metaRoster()->createContact(contact);
		if (!FCreateRequestId.isEmpty())
		{
			ui.wdtAddMetaItem->setEnabled(false);
			ui.pbtAppend->setEnabled(false);
		}
		else
		{
			setErrorMessage(Qt::escape(tr("Failed to create new contact.")));
		}
	}
}

void AddMetaItemPage::onItemWidgetErrorMessageClicked()
{
	QString metaId = FMetaTabWindow->metaRoster()->itemMetaContact(FAddWidget->contactJid());
	if (FMessageProcessor && !metaId.isEmpty())
	{
		FMessageProcessor->createWindow(FMetaTabWindow->metaRoster()->streamJid(),FAddWidget->contactJid(),Message::Chat,IMessageHandler::SM_SHOW);
		FAddWidget->setContactText(QString::null);
	}
}

void AddMetaItemPage::onItemWidgetContactJidChanged()
{
	setErrorMessage(QString::null);
}

void AddMetaItemPage::onMetaContactReceived(const IMetaContact &AContact, const IMetaContact &ABefore)
{
	Q_UNUSED(ABefore);
	if (FAddWidget && AContact.id!=FMetaTabWindow->metaId() && AContact.items.contains(FAddWidget->contactJid()))
	{
		if (FRosterChanger)
			FRosterChanger->subscribeContact(FMetaTabWindow->metaRoster()->streamJid(),FAddWidget->contactJid());
		FMergeRequestId = FMetaTabWindow->metaRoster()->mergeContacts(FMetaTabWindow->metaId(), QList<QString>() << AContact.id);
		if (FMergeRequestId.isEmpty())
			setErrorMessage(Qt::escape(tr("Failed to merge contacts.")));
	}
	else if (FAddWidget && AContact.id==FMetaTabWindow->metaId() && AContact.items.contains(FAddWidget->contactJid()))
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
			setErrorMessage(Qt::escape(tr("Failed to create new contact.")));
	}
	else if (AActionId == FMergeRequestId)
	{
		if (!AErrCond.isEmpty())
			setErrorMessage(Qt::escape(tr("Failed to merge contacts.")));
	}
}
