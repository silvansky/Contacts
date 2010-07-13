#include "addlegacyaccountdialog.h"

#include <QTimer>
#include <QPushButton>
#include <QMessageBox>

AddLegacyAccountDialog::AddLegacyAccountDialog(IPluginManager *APluginManager,const Jid &AStreamJid, const Jid &AServiceJid, QWidget *AParent)	: QDialog(AParent)
{
	ui.setupUi(this);
	setAttribute(Qt::WA_DeleteOnClose,true);

	FDiscovery = NULL;
	FDataForms = NULL;
	FRegistration = NULL;
	initialize(APluginManager);

	FStreamJid = AStreamJid;
	FServiceJid = AServiceJid;

	ui.lblError->setVisible(false);
	ui.chbShowPassword->setVisible(false);
	ui.btbButtons->button(QDialogButtonBox::Ok)->setText(tr("Append"));
	ui.btbButtons->button(QDialogButtonBox::Ok)->setEnabled(false);

	if (!FDiscovery || !FDataForms || !FRegistration)
		abort(tr("Required plugins not found"));
	else if (!FDiscovery->requestDiscoInfo(FStreamJid,FServiceJid))
		abort(tr("Gateway information request failed"));
}

AddLegacyAccountDialog::~AddLegacyAccountDialog()
{

}

void AddLegacyAccountDialog::abort(const QString &AMessage)
{
	QMessageBox::critical(this,tr("Error connecting account"),tr("Failed to connect account due to error:\n%1").arg(AMessage));
	QTimer::singleShot(0,this,SLOT(reject()));
}

void AddLegacyAccountDialog::initialize(IPluginManager *APluginManager)
{
	IPlugin *plugin = APluginManager->pluginInterface("IServiceDiscovery").value(0,NULL);
	if (plugin)
	{
		FDiscovery = qobject_cast<IServiceDiscovery *>(plugin->instance());
		if (FDiscovery)
		{
			connect(FDiscovery->instance(),SIGNAL(discoInfoReceived(const IDiscoInfo &)),SLOT(onDiscoInfoReceived(const IDiscoInfo &)));
		}
	}

	plugin = APluginManager->pluginInterface("IDataForms").value(0,NULL);
	if (plugin)
		FDataForms = qobject_cast<IDataForms *>(plugin->instance());

	plugin = APluginManager->pluginInterface("IRegistration").value(0,NULL);
	if (plugin)
		FRegistration = qobject_cast<IRegistration *>(plugin->instance());
}

void AddLegacyAccountDialog::onDiscoInfoReceived(const IDiscoInfo &AInfo)
{

}
