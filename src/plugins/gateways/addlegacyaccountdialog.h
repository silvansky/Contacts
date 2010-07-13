#ifndef ADDLEGACYACCOUNTDIALOG_H
#define ADDLEGACYACCOUNTDIALOG_H

#include <QDialog>
#include <interfaces/ipluginmanager.h>
#include <interfaces/idataforms.h>
#include <interfaces/iregistraton.h>
#include <interfaces/iservicediscovery.h>
#include "ui_addlegacyaccountdialog.h"

class AddLegacyAccountDialog : 
	public QDialog
{
	Q_OBJECT;
public:
	AddLegacyAccountDialog(IPluginManager *APluginManager, const Jid &AStreamJid, const Jid &AServiceJid, QWidget *AParent=NULL);
	~AddLegacyAccountDialog();
protected:
	void abort(const QString &AMessage);
	void initialize(IPluginManager *APluginManager);
protected slots:
	void onDiscoInfoReceived(const IDiscoInfo &AInfo);
private:
	Ui::AddLegacyAccountDialogClass ui;
private:
	IDataForms *FDataForms;
	IRegistration *FRegistration;
	IServiceDiscovery *FDiscovery;
private:
	Jid FStreamJid;
	Jid FServiceJid;
};

#endif // ADDLEGACYACCOUNTDIALOG_H
