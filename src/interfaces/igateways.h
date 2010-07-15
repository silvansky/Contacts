#ifndef IGATEWAYS_H
#define IGATEWAYS_H

#include <QDialog>
#include <utils/jid.h>
#include <interfaces/iregistraton.h>
#include <interfaces/iservicediscovery.h>

#define GATEWAYS_UUID "{2a3ce0cd-bf67-4f15-8907-b7d0706be4b4}"

struct IGateRegisterLabel
{
	IGateRegisterLabel() { valid = false; }
	bool valid;
	QIcon icon;
	QString name;
	QString loginLabel;
	QList<QString> domains;
};

struct IGateRegisterLogin
{
	IGateRegisterLogin() { valid = false; }
	bool valid;
	QString login;
	QString domain;
	QString password;
	IRegisterFields fields;
};

class IGateways
{
public:
	virtual QObject *instance() =0;
	virtual void resolveNickName(const Jid &AStreamJid, const Jid &AContactJid) =0;
	virtual void sendLogPresence(const Jid &AStreamJid, const Jid &AServiceJid, bool ALogIn) =0;
	virtual QList<Jid> keepConnections(const Jid &AStreamJid) const =0;
	virtual void setKeepConnection(const Jid &AStreamJid, const Jid &AServiceJid, bool AEnabled) =0;
	virtual QList<Jid> availServices(const Jid &AStreamJid, const IDiscoIdentity &AIdentity = IDiscoIdentity()) const =0;
	virtual QList<Jid> streamServices(const Jid &AStreamJid, const IDiscoIdentity &AIdentity = IDiscoIdentity()) const =0;
	virtual QList<Jid> serviceContacts(const Jid &AStreamJid, const Jid &AServiceJid) const =0;
	virtual IGateRegisterLabel registerLabel(const Jid &AStreamJid, const Jid &AServiceJid) const =0;
	virtual IGateRegisterLogin registerLogin(const Jid &AStreamJid, const Jid &AServiceJid, const IRegisterFields &AFields) const =0;
	virtual IRegisterSubmit registerSubmit(const Jid &AStreamJid, const Jid &AServiceJid, const IGateRegisterLogin &ALogin) const =0;
	virtual bool changeService(const Jid &AStreamJid, const Jid &AServiceFrom, const Jid &AServiceTo, bool ARemove, bool ASubscribe) =0;
	virtual QString sendPromptRequest(const Jid &AStreamJid, const Jid &AServiceJid) =0;
	virtual QString sendUserJidRequest(const Jid &AStreamJid, const Jid &AServiceJid, const QString &AContactID) =0;
	virtual QDialog *showAddLegacyAccountDialog(const Jid &AStreamJid, const Jid &AServiceJid, QWidget *AParent = NULL) =0;
	virtual QDialog *showAddLegacyContactDialog(const Jid &AStreamJid, const Jid &AServiceJid, QWidget *AParent = NULL) =0;
protected:
	virtual void promptReceived(const QString &AId, const QString &ADesc, const QString &APrompt) =0;
	virtual void userJidReceived(const QString &AId, const Jid &AUserJid) =0;
	virtual void errorReceived(const QString &AId, const QString &AError) =0;
};

Q_DECLARE_INTERFACE(IGateways,"Virtus.Plugin.IGateways/1.0")

#endif
