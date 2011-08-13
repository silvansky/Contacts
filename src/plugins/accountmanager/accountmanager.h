#ifndef ACCOUNTMANAGER_H
#define ACCOUNTMANAGER_H

#include <definitions/optionvalues.h>
#include <interfaces/ipluginmanager.h>
#include <interfaces/iaccountmanager.h>
#include <interfaces/ioptionsmanager.h>
#include <interfaces/ixmppstreams.h>
#include <utils/action.h>
#include <utils/log.h>
#include "account.h"

class AccountManager :
	public QObject,
	public IPlugin,
	public IAccountManager
{
	Q_OBJECT;
	Q_INTERFACES(IPlugin IAccountManager);
public:
	AccountManager();
	~AccountManager();
	virtual QObject *instance() { return this; }
	//IPlugin
	virtual QUuid pluginUuid() const { return ACCOUNTMANAGER_UUID; }
	virtual void pluginInfo(IPluginInfo *APluginInfo);
	virtual bool initConnections(IPluginManager *APluginManager, int &AInitOrder);
	virtual bool initObjects() { return true; }
	virtual bool initSettings();
	virtual bool startPlugin() { return true; }
	//IAccountManager
	virtual QList<IAccount *> accounts() const;
	virtual IAccount *accountById(const QUuid &AAcoountId) const;
	virtual IAccount *accountByStream(const Jid &AStreamJid) const;
	virtual IAccount *appendAccount(const QUuid &AAccountId);
	virtual void showAccount(const QUuid &AAccountId);
	virtual void hideAccount(const QUuid &AAccountId);
	virtual void removeAccount(const QUuid &AAccountId);
	virtual void destroyAccount(const QUuid &AAccountId);
signals:
	void appended(IAccount *AAccount);
	void shown(IAccount *AAccount);
	void hidden(IAccount *AAccount);
	void removed(IAccount *AAccount);
	void changed(IAccount *AAcount, const OptionsNode &ANode);
	void destroyed(const QUuid &AAccountId);
protected slots:
	void onProfileOpened(const QString &AProfile);
	void onProfileClosed(const QString &AProfile);
	void onOptionsOpened();
	void onOptionsClosed();
	void onAccountActiveChanged(bool AActive);
	void onAccountOptionsChanged(const OptionsNode &ANode);
private:
	IXmppStreams *FXmppStreams;
	IOptionsManager *FOptionsManager;
private:
	QMap<QUuid, IAccount *> FAccounts;
};

#endif // ACCOUNTMANAGER_H
