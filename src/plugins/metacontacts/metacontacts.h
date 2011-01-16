#ifndef METACONTACTS_H
#define METACONTACTS_H

#include <QMultiMap>
#include <QObjectCleanupHandler>
#include <definitions/rosterproxyorders.h>
#include <definitions/rosterclickhookerorders.h>
#include <interfaces/ipluginmanager.h>
#include <interfaces/imetacontacts.h>
#include <interfaces/istanzaprocessor.h>
#include <interfaces/irostersview.h>
#include <interfaces/imessagewidgets.h>
#include <interfaces/imessageprocessor.h>
#include "metaroster.h"
#include "metaproxymodel.h"
#include "metatabwindow.h"

class MetaContacts : 
	public QObject,
	public IPlugin,
	public IMetaContacts,
	public IRostersClickHooker
{
	Q_OBJECT;
	Q_INTERFACES(IPlugin IMetaContacts IRostersClickHooker);
public:
	MetaContacts();
	~MetaContacts();
	virtual QObject* instance() { return this; }
	//IPlugin
	virtual QUuid pluginUuid() const { return METACONTACTS_UUID; }
	virtual void pluginInfo(IPluginInfo *APluginInfo);
	virtual bool initConnections(IPluginManager *APluginManager, int &AInitOrder);
	virtual bool initObjects();
	virtual bool initSettings() { return true; }
	virtual bool startPlugin() { return true; }
	//IRostersClickHooker
	virtual bool rosterIndexClicked(IRosterIndex *AIndex, int AOrder);
	//IMetaContacts
	virtual IMetaRoster *newMetaRoster(IRoster *ARoster);
	virtual IMetaRoster *findMetaRoster(const Jid &AStreamJid) const;
	virtual void removeMetaRoster(IRoster *ARoster);
	virtual QString metaRosterFileName(const Jid &AStreamJid) const;
	virtual QList<IMetaTabWindow *> metaTabWindows() const;
	virtual IMetaTabWindow *newMetaTabWindow(const Jid &AStreamJid, const Jid &AMetaId);
	virtual IMetaTabWindow *findMetaTabWindow(const Jid &AStreamJid, const Jid &AMetaId) const;
signals:
	void metaRosterAdded(IMetaRoster *AMetaRoster);
	void metaRosterOpened(IMetaRoster *AMetaRoster);
	void metaContactReceived(IMetaRoster *AMetaRoster, const IMetaContact &AContact, const IMetaContact &ABefore);
	void metaActionResult(IMetaRoster *AMetaRoster, const QString &AActionId, const QString &AErrCond, const QString &AErrMessage);
	void metaRosterClosed(IMetaRoster *AMetaRoster);
	void metaRosterEnabled(IMetaRoster *AMetaRoster, bool AEnabled);
	void metaRosterStreamJidAboutToBeChanged(IMetaRoster *AMetaRoster, const Jid &AAfter);
	void metaRosterStreamJidChanged(IMetaRoster *AMetaRoster, const Jid &ABefore);
	void metaRosterRemoved(IMetaRoster *AMetaRoster);
	void metaTabWindowCreated(IMetaTabWindow *AWindow);
	void metaTabWindowDestroyed(IMetaTabWindow *AWindow);
protected:
	void deleteMetaRosterWindows(IMetaRoster *AMetaRoster);
protected slots:
	void onMetaRosterOpened();
	void onMetaContactReceived(const IMetaContact &AContact, const IMetaContact &ABefore);
	void onMetaActionResult(const QString &AActionId, const QString &AErrCond, const QString &AErrMessage);
	void onMetaRosterClosed();
	void onMetaRosterEnabled(bool AEnabled);
	void onMetaRosterStreamJidAboutToBeChanged(const Jid &AAfter);
	void onMetaRosterStreamJidChanged(const Jid &ABefour);
	void onMetaRosterDestroyed(QObject *AObject);
	void onRosterAdded(IRoster *ARoster);
	void onRosterRemoved(IRoster *ARoster);
protected slots:
	void onMetaTabWindowItemPageRequested(const Jid &AItemJid);
	void onMetaTabWindowDestroyed();
protected slots:
	void onLoadMetaRosters();
	void onChatWindowCreated(IChatWindow *AWindow);
private:
	IPluginManager *FPluginManager;
	IRosterPlugin *FRosterPlugin;
	IRostersViewPlugin *FRostersViewPlugin;
	IStanzaProcessor *FStanzaProcessor;
	IMessageWidgets *FMessageWidgets;
	IMessageProcessor *FMessageProcessor;
private:
	QList<IMetaRoster *> FLoadQueue;
	QList<IMetaRoster *> FMetaRosters;
	QObjectCleanupHandler FCleanupHandler;
private:
	QList<IMetaTabWindow *> FMetaTabWindows;
};

#endif // METACONTACTS_H
