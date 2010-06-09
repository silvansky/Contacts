#ifndef ICLIENTINFO_H
#define ICLIENTINFO_H

#include <QDateTime>
#include <utils/jid.h>

#define CLIENTINFO_UUID "{8b8abf23-7bd2-4d18-9769-f34dbbef4a67}"

class IClientInfo
{
public:
	enum InfoType {
		SoftwareVersion             =1,
		LastActivity                =2,
		EntityTime                  =4
	};
	enum SoftwareStatus {
		SoftwareNotLoaded,
		SoftwareLoaded,
		SoftwareLoading,
		SoftwareError
	};
public:
	virtual QObject *instance() =0;
	virtual QString osVersion() const =0;
	virtual void showClientInfo(const Jid &AStreamJid, const Jid &AContactJid, int AInfoTypes) =0;
	//Software Version
	virtual bool hasSoftwareInfo(const Jid &AContactJid) const =0;
	virtual bool requestSoftwareInfo(const Jid &AStreamJid, const Jid &AContactJid) =0;
	virtual int softwareStatus(const Jid &AContactJid) const =0;
	virtual QString softwareName(const Jid &AContactJid) const =0;
	virtual QString softwareVersion(const Jid &AContactJid) const =0;
	virtual QString softwareOs(const Jid &AContactJid) const =0;
	//Last Activity
	virtual bool hasLastActivity(const Jid &AContactJid) const =0;
	virtual bool requestLastActivity(const Jid &AStreamJid, const Jid &AContactJid) =0;
	virtual QDateTime lastActivityTime(const Jid &AContactJid) const =0;
	virtual QString lastActivityText(const Jid &AContactJid) const =0;
	//Entity Time
	virtual bool hasEntityTime(const Jid &AContactJid) const =0;
	virtual bool requestEntityTime(const Jid &AStreamJid, const Jid &AContactJid) =0;
	virtual QDateTime entityTime(const Jid &AContactJid) const =0;
	virtual int entityTimeDelta(const Jid &AContactJid) const =0;
	virtual int entityTimePing(const Jid &AContactJid) const =0;
protected:
	virtual void softwareInfoChanged(const Jid &AContactJid) =0;
	virtual void lastActivityChanged(const Jid &AContactJid) =0;
	virtual void entityTimeChanged(const Jid &AContactJid) =0;
};

Q_DECLARE_INTERFACE(IClientInfo,"Virtus.Plugin.IClientInfo/1.0")

#endif
