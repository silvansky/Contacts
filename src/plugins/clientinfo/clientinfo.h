#ifndef CLIENTINFO_H
#define CLIENTINFO_H

#include <definitions/version.h>
#include <definitions/namespaces.h>
#include <definitions/dataformtypes.h>
#include <definitions/optionvalues.h>
#include <interfaces/ipluginmanager.h>
#include <interfaces/iclientinfo.h>
#include <interfaces/istanzaprocessor.h>
#include <interfaces/ipresence.h>
#include <interfaces/iservicediscovery.h>
#include <utils/errorhandler.h>
#include <utils/stanza.h>
#include <utils/options.h>
#include <utils/datetime.h>
#include <utils/widgetmanager.h>
#include <utils/systemmanager.h>

struct SoftwareItem {
	SoftwareItem() { 
		status = IClientInfo::SoftwareNotLoaded; 
	}
	QString name;
	QString version;
	QString os;
	int status;
};

struct ActivityItem {
	QDateTime requestTime;
	QDateTime dateTime;
	QString text;
};

struct TimeItem {
	TimeItem() { 
		ping = -1; 
		delta = 0; 
		zone = 0; 
	}
	int ping;
	int delta;
	int zone;
};

class ClientInfo :
		public QObject,
		public IPlugin,
		public IClientInfo,
		public IStanzaHandler,
		public IStanzaRequestOwner,
		public IDataLocalizer
{
	Q_OBJECT;
	Q_INTERFACES(IPlugin IClientInfo IStanzaHandler IStanzaRequestOwner IDataLocalizer);
public:
	ClientInfo();
	~ClientInfo();
	//IPlugin
	virtual QObject *instance() { return this; }
	virtual QUuid pluginUuid() const { return CLIENTINFO_UUID; }
	virtual void pluginInfo(IPluginInfo *APluginInfo);
	virtual bool initConnections(IPluginManager *APluginManager, int &AInitOrder);
	virtual bool initObjects();
	virtual bool initSettings();
	virtual bool startPlugin();
	//IStanzaHandler
	virtual bool stanzaReadWrite(int AHandlerId, const Jid &AStreamJid, Stanza &AStanza, bool &AAccept);
	//IStanzaRequestOwner
	virtual void stanzaRequestResult(const Jid &AStreamJid, const Stanza &AStanza);
	virtual void stanzaRequestTimeout(const Jid &AStreamJid, const QString &AStanzaId);
	//IDataLocalizer
	virtual IDataFormLocale dataFormLocale(const QString &AFormType);
	//IClientInfo
	virtual QString osVersion() const;
	//Software Version
	virtual bool hasSoftwareInfo(const Jid &AContactJid) const;
	virtual bool requestSoftwareInfo( const Jid &AStreamJid, const Jid &AContactJid);
	virtual int softwareStatus(const Jid &AContactJid) const;
	virtual QString softwareName(const Jid &AContactJid) const;
	virtual QString softwareVersion(const Jid &AContactJid) const;
	virtual QString softwareOs(const Jid &AContactJid) const;
	//Last Activity
	virtual bool hasLastActivity(const Jid &AContactJid) const;
	virtual bool requestLastActivity(const Jid &AStreamJid, const Jid &AContactJid);
	virtual QDateTime lastActivityTime(const Jid &AContactJid) const;
	virtual QString lastActivityText(const Jid &AContactJid) const;
	//Entity Time
	virtual bool hasEntityTime(const Jid &AContactJid) const;
	virtual bool requestEntityTime(const Jid &AStreamJid, const Jid &AContactJid);
	virtual QDateTime entityTime(const Jid &AContactJid) const;
	virtual int entityTimeDelta(const Jid &AContactJid) const;
	virtual int entityTimePing(const Jid &AContactJid) const;
signals:
	//IClientInfo
	void softwareInfoChanged(const Jid &AContactJid);
	void lastActivityChanged(const Jid &AContactJid);
	void entityTimeChanged(const Jid &AContactJid);
protected:
	void registerDiscoFeatures();
protected slots:
	void onContactStateChanged(const Jid &AStreamJid, const Jid &AContactJid, bool AStateOnline);
	void onDiscoInfoReceived(const IDiscoInfo &AInfo);
	void onOptionsChanged(const OptionsNode &ANode);
private:
	IPluginManager *FPluginManager;
	IStanzaProcessor *FStanzaProcessor;
	IPresencePlugin *FPresencePlugin;
	IServiceDiscovery *FDiscovery;
	IDataForms *FDataForms;
private:
	int FPingHandle;
	int FTimeHandle;
	int FVersionHandle;
	int FActivityHandler;
	QMap<QString, Jid> FSoftwareId;
	QMap<Jid, SoftwareItem> FSoftwareItems;
	QMap<QString, Jid> FActivityId;
	QMap<Jid, ActivityItem> FActivityItems;
	QMap<QString, Jid> FTimeId;
	QMap<Jid, TimeItem> FTimeItems;
};

#endif // CLIENTINFO_H
