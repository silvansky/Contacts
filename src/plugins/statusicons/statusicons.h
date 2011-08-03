#ifndef STATUSICONS_H
#define STATUSICONS_H

#include <QRegExp>
#include <definitions/resources.h>
#include <definitions/statusicons.h>
#include <definitions/optionvalues.h>
#include <definitions/optionnodes.h>
#include <definitions/optionnodeorders.h>
#include <definitions/optionwidgetorders.h>
#include <definitions/actiongroups.h>
#include <definitions/menuicons.h>
#include <definitions/rosterindextyperole.h>
#include <definitions/rosterdataholderorders.h>
#include <interfaces/ipluginmanager.h>
#include <interfaces/istatusicons.h>
#include <interfaces/iroster.h>
#include <interfaces/ipresence.h>
#include <interfaces/irostersmodel.h>
#include <interfaces/irostersview.h>
#include <interfaces/ioptionsmanager.h>
#include <utils/options.h>

class StatusIcons :
	public QObject,
	public IPlugin,
	public IStatusIcons,
	public IRosterDataHolder
{
	Q_OBJECT;
	Q_INTERFACES(IPlugin IStatusIcons IRosterDataHolder);
public:
	StatusIcons();
	~StatusIcons();
	//IPlugin
	virtual QObject *instance() { return this; }
	virtual QUuid pluginUuid() const { return STATUSICONS_UUID; }
	virtual void pluginInfo(IPluginInfo *APluginInfo);
	virtual bool initConnections(IPluginManager *APluginManager, int &AInitOrder);
	virtual bool initObjects();
	virtual bool initSettings();
	virtual bool startPlugin() { return true; }
	//IRosterDataHolder
	virtual int rosterDataOrder() const;
	virtual QList<int> rosterDataRoles() const;
	virtual QList<int> rosterDataTypes() const;
	virtual QVariant rosterData(const IRosterIndex *AIndex, int ARole) const;
	virtual bool setRosterData(IRosterIndex *AIndex, int ARole, const QVariant &AValue);
	//IStatusIcons
	virtual QList<QString> rules(RuleType ARuleType) const;
	virtual QString ruleIconset(const QString &APattern, RuleType ARuleType) const;
	virtual void insertRule(const QString &APattern, const QString &ASubStorage, RuleType ARuleType);
	virtual void removeRule(const QString &APattern, RuleType ARuleType);
	virtual QIcon iconByJid(const Jid &AStreamJid, const Jid &AContactJid) const;
	virtual QIcon iconByStatus(int AShow, const QString &ASubscription, bool AAsk) const;
	virtual QIcon iconByJidStatus(const Jid &AContactJid, int AShow, const QString &ASubscription, bool AAsk) const;
	virtual QString iconsetByJid(const Jid &AContactJid) const;
	virtual QString iconKeyByJid(const Jid &AStreamJid, const Jid &AContactJid) const;
	virtual QString iconKeyByStatus(int AShow, const QString &ASubscription, bool AAsk) const;
	virtual QString iconFileName(const QString &ASubStorage, const QString &AIconKey) const;
signals:
	void defaultIconsetChanged(const QString &ASubStorage);
	void ruleInserted(const QString &APattern, const QString &ASubStorage, RuleType ARuleType);
	void ruleRemoved(const QString &APattern, RuleType ARuleType);
	void defaultIconsChanged();
	void statusIconsChanged();
	//IRosterDataHolder
	void rosterDataChanged(IRosterIndex *AIndex = NULL, int ARole = RDR_ANY_ROLE);
protected:
	void loadStorages();
	void clearStorages();
	void startStatusIconsChanged();
protected slots:
	void onStatusIconsChangedTimer();
	void onOptionsOpened();
	void onOptionsClosed();
	void onOptionsChanged(const OptionsNode &ANode);
	void onDefaultIconsetChanged();
private:
	IRosterPlugin *FRosterPlugin;
	IPresencePlugin *FPresencePlugin;
	IRostersModel *FRostersModel;
private:
	bool FStatusIconsChangedStarted;
	IconStorage *FDefaultStorage;
	QSet<QString> FStatusRules;
	QMap<QString, QString> FUserRules;
	QMap<QString, QString> FDefaultRules;
	QMap<QString, IconStorage *> FStorages;
	mutable QHash<Jid, QString> FJid2Storage;
};

#endif // STATUSICONS_H
