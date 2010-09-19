#ifndef ADDCONTACTDIALOG_H
#define ADDCONTACTDIALOG_H

#include <QDialog>
#include <definations/vcardvaluenames.h>
#include <definations/resources.h>
#include <definations/menuicons.h>
#include <definations/optionvalues.h>
#include <definations/stylesheets.h>
#include <interfaces/ipluginmanager.h>
#include <interfaces/irosterchanger.h>
#include <interfaces/iroster.h>
#include <interfaces/igateways.h>
#include <interfaces/ivcard.h>
#include <utils/options.h>
#include <utils/stylestorage.h>
#include "ui_addcontactdialog.h"

class AddContactDialog :
			public QDialog,
			public IAddContactDialog
{
	Q_OBJECT;
	Q_INTERFACES(IAddContactDialog);
public:
	AddContactDialog(IRosterChanger *ARosterChanger, IPluginManager *APluginManager, const Jid &AStreamJid, QWidget *AParent = NULL);
	~AddContactDialog();
	//IAddContactDialog
	virtual QDialog *instance() { return this; }
	virtual Jid streamJid() const;
	virtual Jid contactJid() const;
	virtual void setContactJid(const Jid &AContactJid);
	virtual QString nickName() const;
	virtual void setNickName(const QString &ANick);
	virtual QString group() const;
	virtual void setGroup(const QString &AGroup);
	virtual Jid gatewayJid() const;
	virtual void setGatewayJid(const Jid &AGatewayJid);
signals:
	void dialogDestroyed();
protected:
	void initialize(IPluginManager *APluginManager);
	void initGroups();
	void initGateways();
protected slots:
	void onDialogAccepted();
	void onGroupCurrentIndexChanged(int AIndex);
	void onVCardReceived(const Jid &AContactJid);
	void onServiceLoginReceived(const QString &AId, const QString &ALogin);
	void onServiceErrorReceived(const QString &AId, const QString &AError);
private:
	Ui::AddContactDialogClass ui;
private:
	IRoster *FRoster;
	IGateways *FGateways;
	IVCardPlugin *FVcardPlugin;
	IRosterChanger *FRosterChanger;
private:
	Jid FStreamJid;
	QMap<QString, Jid> FLoginRequests;
};

#endif // ADDCONTACTDIALOG_H
