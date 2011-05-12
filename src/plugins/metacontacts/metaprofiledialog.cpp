#include "metaprofiledialog.h"

#define MAX_STATUS_TEXT_SIZE    100

MetaProfileDialog::MetaProfileDialog(IPluginManager *APluginManager, IMetaRoster *AMetaRoster, const QString &AMetaId, QWidget *AParent) : QDialog(AParent)
{
	ui.setupUi(this);

	FGateways = NULL;
	FStatusIcons = NULL;
	FStatusChanger = NULL;
	FRosterChanger = NULL;

	FMetaId = AMetaId;
	FMetaRoster = AMetaRoster;

	ui.lblCaption->setText(tr("Contact Profile"));

	FBorder = CustomBorderStorage::staticStorage(RSR_STORAGE_CUSTOMBORDER)->addBorder(this, CBS_DIALOG);
	if (FBorder)
	{
		FBorder->setResizable(false);
		FBorder->setMinimizeButtonVisible(false);
		FBorder->setMaximizeButtonVisible(false);
		FBorder->setAttribute(Qt::WA_DeleteOnClose,true);
		FBorder->setWindowTitle(ui.lblCaption->text());
		connect(this, SIGNAL(accepted()), FBorder, SLOT(close()));
		connect(this, SIGNAL(rejected()), FBorder, SLOT(close()));
		connect(FBorder, SIGNAL(closeClicked()), SIGNAL(rejected()));
		setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
	}
	else
		setAttribute(Qt::WA_DeleteOnClose,true);

	StyleStorage::staticStorage(RSR_STORAGE_STYLESHEETS)->insertAutoStyle(this,STS_METACONTACTS_METAPROFILEDIALOG);
	GraphicsEffectsStorage::staticStorage(RSR_STORAGE_GRAPHICSEFFECTS)->installGraphicsEffect(this, GFX_LABELS);

	connect(FMetaRoster->instance(),SIGNAL(metaContactReceived(const IMetaContact &, const IMetaContact &)),
		SLOT(onMetaContactReceived(const IMetaContact &, const IMetaContact &)));
	connect(FMetaRoster->instance(),SIGNAL(metaAvatarChanged(const QString &)),SLOT(onMetaAvatarChanged(const QString &)));
	connect(FMetaRoster->instance(),SIGNAL(metaPresenceChanged(const QString &)),SLOT(onMetaPresenceChanged(const QString &)));

	initialize(APluginManager);

	onMetaAvatarChanged(FMetaId);
	onMetaPresenceChanged(FMetaId);
	onMetaContactReceived(FMetaRoster->metaContact(FMetaId),IMetaContact());
}

MetaProfileDialog::~MetaProfileDialog()
{
	if (FBorder)
		FBorder->deleteLater();
}

Jid MetaProfileDialog::streamJid() const
{
	return FMetaRoster->streamJid();
}

QString MetaProfileDialog::metaContactId() const
{
	return FMetaId;
}

void MetaProfileDialog::initialize(IPluginManager *APluginManager)
{
	IPlugin *plugin = APluginManager->pluginInterface("IRosterChanger").value(0,NULL);
	if (plugin)
		FRosterChanger = qobject_cast<IRosterChanger *>(plugin->instance());

	plugin = APluginManager->pluginInterface("IGateways").value(0,NULL);
	if (plugin)
		FGateways = qobject_cast<IGateways *>(plugin->instance());

	plugin = APluginManager->pluginInterface("IStatusIcons").value(0,NULL);
	if (plugin)
		FStatusIcons = qobject_cast<IStatusIcons *>(plugin->instance());

	plugin = APluginManager->pluginInterface("IStatusChanger").value(0,NULL);
	if (plugin)
		FStatusChanger = qobject_cast<IStatusChanger *>(plugin->instance());

	ui.pbtAddContact->setEnabled(FRosterChanger!=NULL);
}

void MetaProfileDialog::onMetaAvatarChanged(const QString &AMetaId)
{
	if (AMetaId == FMetaId)
	{
		QImage avatar = ImageManager::roundSquared(FMetaRoster->metaAvatarImage(FMetaId,false,false),50,2);
		ui.lblAvatar->setPixmap(QPixmap::fromImage(avatar));
	}
}

void MetaProfileDialog::onMetaPresenceChanged(const QString &AMetaId)
{
	if (AMetaId == FMetaId)
	{
		IPresenceItem pitem = FMetaRoster->metaPresenceItem(FMetaId);
		QIcon icon = FStatusIcons!=NULL ? FStatusIcons->iconByStatus(pitem.show,SUBSCRIPTION_BOTH,false) : QIcon();
		ui.lblStatusIcon->setPixmap(icon.pixmap(icon.availableSizes().value(0)));
		ui.lblStatusName->setText(FStatusChanger!=NULL ? FStatusChanger->nameByShow(pitem.show) : QString::null);

		QString status = pitem.status.left(MAX_STATUS_TEXT_SIZE);
		status += status.size() < pitem.status.size() ? "..." : "";
		ui.lblStatusText->setText(status);
	}
}

void MetaProfileDialog::onMetaContactReceived(const IMetaContact &AContact, const IMetaContact &ABefore)
{
	if (AContact.id == FMetaId)
	{
		ui.lneName->setText(AContact.name);
	}
}
