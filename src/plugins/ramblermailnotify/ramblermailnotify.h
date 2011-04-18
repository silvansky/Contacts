#ifndef RAMBLERMAILNOTIFY_H
#define RAMBLERMAILNOTIFY_H

#define RAMBLERMAILNOTIFY_UUID "{7EDE7B07-D284-4cd9-AE63-46EFBD4DE683}"

#include <QObject>
#include <definitions/rosterindextyperole.h>
#include <definitions/rosterindextypeorders.h>
#include <definitions/rosterfootertextorders.h>
#include <interfaces/ipluginmanager.h>
#include <interfaces/igateways.h>
#include <interfaces/iroster.h>
#include <interfaces/ipresence.h>
#include <interfaces/irostersview.h>
#include <interfaces/irostersmodel.h>
#include <interfaces/iservicediscovery.h>
#include <interfaces/istatusicons.h>

class RamblerMailNotify : 
	public QObject,
	public IPlugin
{
	Q_OBJECT;
	Q_INTERFACES(IPlugin);
public:
	RamblerMailNotify();
	~RamblerMailNotify();
	virtual QObject *instance() { return this; }
	//IPlugin
	virtual QUuid pluginUuid() const { return RAMBLERMAILNOTIFY_UUID; }
	virtual void pluginInfo(IPluginInfo *APluginInfo);
	virtual bool initConnections(IPluginManager *APluginManager, int &AInitOrder);
	virtual bool initObjects() { return true; }
	virtual bool initSettings() { return true; }
	virtual bool startPlugin() { return true; }
protected:
	void updateMailIndex(const Jid &AStreamJid);
	IRosterIndex *findMailIndex(const Jid &AStreamJid) const;
protected slots:
	void onStreamAdded(const Jid &AStreamJid);
	void onStreamRemoved(const Jid &AStreamJid);
	void onRosterStateChanged(IRoster *ARoster);
private:
	IGateways *FGateways;
	IRosterPlugin *FRosterPlugin;
	IRostersView *FRostersView;
	IRostersModel *FRostersModel;
	IServiceDiscovery *FDiscovery;
	IStatusIcons *FStatusIcons;
private:
	QList<IRosterIndex *> FMailIndexes;
};

#endif // RAMBLERMAILNOTIFY_H
