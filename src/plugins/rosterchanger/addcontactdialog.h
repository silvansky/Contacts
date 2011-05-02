#ifndef ADDCONTACTDIALOG_H
#define ADDCONTACTDIALOG_H

#include <QUrl>
#include <QTimer>
#include <QDialog>
#include <definitions/vcardvaluenames.h>
#include <definitions/resources.h>
#include <definitions/menuicons.h>
#include <definitions/optionvalues.h>
#include <definitions/stylesheets.h>
#include <interfaces/ipluginmanager.h>
#include <interfaces/irosterchanger.h>
#include <interfaces/iroster.h>
#include <interfaces/igateways.h>
#include <interfaces/iavatars.h>
#include <interfaces/ivcard.h>
#include <utils/options.h>
#include <utils/iconstorage.h>
#include <utils/stylestorage.h>
#include <utils/customlistview.h>
#include "ui_addcontactdialog.h"

class AddContactDialog :
	public QDialog,
	public IAddContactDialog
{
	Q_OBJECT;
	Q_INTERFACES(IAddContactDialog);
public:
	AddContactDialog(IRoster *ARoster, IRosterChanger *ARosterChanger, IPluginManager *APluginManager, QWidget *AParent = NULL);
	~AddContactDialog();
	//IAddContactDialog
	virtual QDialog *instance() { return this; }
	virtual Jid streamJid() const;
	virtual Jid contactJid() const;
	virtual void setContactJid(const Jid &AContactJid);
	virtual QString contactText() const;
	virtual void setContactText(const QString &AText);
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
protected:
	QString normalContactText(const QString &AText) const;
	QString defaultContactNick(const Jid &AContactJid) const;
	QList<Jid> suitableServices(const IGateServiceDescriptor &ADescriptor) const;
	QList<Jid> suitableServices(const QList<IGateServiceDescriptor> &ADescriptors) const;
protected:
	void setDialogState(int AState);
	void startResolve(int ATimeout);
	void setErrorMessage(const QString &AMessage);
	void setGatewaysEnabled(bool AEnabled);
	void setRealContactJid(const Jid &AContactJid);
	void setResolveNickState(bool AResole);
protected slots:
	void resolveDescriptor();
	void resolveServiceJid();
	void resolveContactJid();
	void resolveContactName();
protected:
	virtual void showEvent(QShowEvent *AEvent);
protected slots:
	void onDialogAccepted();
	void onAdjustDialogSize();
	void onContactTextEdited(const QString &AText);
	void onContactNickEdited(const QString &AText);
	void onGroupCurrentIndexChanged(int AIndex);
	void onProfileCurrentIndexChanged(int AIndex);
	void onVCardReceived(const Jid &AContactJid);
	void onVCardError(const Jid &AContactJid, const QString &AError);
	void onServiceLoginReceived(const QString &AId, const QString &ALogin);
	void onLegacyContactJidReceived(const QString &AId, const Jid &AUserJid);
	void onServiceEnableChanged(const Jid &AStreamJid, const Jid &AServiceJid, bool AEnabled);
	void onGatewayErrorReceived(const QString &AId, const QString &AError);
private:
	Ui::AddContactDialogClass ui;
private:
	IRoster *FRoster;
	IAvatars *FAvatars;
	IGateways *FGateways;
	IVCardPlugin *FVcardPlugin;
	IRosterChanger *FRosterChanger;
private:
	IGateServiceDescriptor FDescriptor;
	QList<IGateServiceDescriptor> FConfirmDescriptors;
private:
	QLabel *FServiceIcon;
private:
	Jid FContactJid;
	Jid FPreferGateJid;
private:
	bool FShown;
	int FDialogState;
	bool FResolveNick;
	QTimer FResolveTimer;
	QString FContactJidRequest;
	QList<Jid> FEnabledGateways;
	QList<Jid> FDisabledGateways;
	QMap<QString, Jid> FServices;
	QMap<QString, Jid> FLoginRequests;
};

#endif // ADDCONTACTDIALOG_H
