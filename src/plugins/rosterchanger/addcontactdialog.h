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
#include "ui_addcontactdialog.h"

class AddContactDialog :
			public QDialog,
			public IAddContactDialog
{
	Q_OBJECT
	Q_INTERFACES(IAddContactDialog)
public:
	AddContactDialog(IRosterChanger *ARosterChanger, IPluginManager *APluginManager, const Jid &AStreamJid, QWidget *AParent = NULL);
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
	void updateGateways();
	void updateServices(const Jid &AServiceJid = Jid::null);
protected:
	QString normalContactText(const QString &AText) const;
	QString defaultContactNick(const Jid &AContactJid) const;
	QList<Jid> suitableServices(const IGateServiceDescriptor &ADescriptor) const;
	QList<Jid> suitableServices(const QList<IGateServiceDescriptor> &ADescriptors) const;
protected:
	void startResolve(int ATimeout);
	void setInfoMessage(const QString &AMessage);
	void setErrorMessage(const QString &AMessage);
	void setActionLink(const QString &AMessage, const QUrl &AUrl);
	void setGatewaysEnabled(bool AEnabled);
	void setContactAcceptable(bool AAcceptable);
	void setRealContactJid(const Jid &AContactJid);
	void setResolveNickState(bool AResole);
protected slots:
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
	void onActionLinkActivated(const QString &ALink);
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
	QLabel *FServiceIcon;
private:
	Jid FStreamJid;
	Jid FContactJid;
	Jid FPreferGateJid;
private:
	bool FShown;
	bool FResolveNick;
	QTimer FResolveTimer;
	QString FContactJidRequest;
	QList<Jid> FEnabledGateways;
	QList<Jid> FDisabledGateways;
	QMap<QString, Jid> FServices;
	QMap<QString, Jid> FLoginRequests;
};

#endif // ADDCONTACTDIALOG_H
