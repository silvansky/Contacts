#ifndef METAROSTER_H
#define METAROSTER_H

#include <definitions/namespaces.h>
#include <definitions/stanzahandlerorders.h>
#include <interfaces/ipluginmanager.h>
#include <interfaces/imetacontacts.h>
#include <interfaces/istanzaprocessor.h>
#include <interfaces/iavatars.h>
#include <utils/stanza.h>
#include <utils/errorhandler.h>

class MetaRoster : 
	public QObject,
	public IMetaRoster,
	public IStanzaHandler,
	public IStanzaRequestOwner
{
	Q_OBJECT;
	Q_INTERFACES(IMetaRoster IStanzaHandler IStanzaRequestOwner);
public:
	MetaRoster(IRoster *ARoster, IPluginManager *APluginManager);
	~MetaRoster();
	virtual QObject *instance() { return this; }
	//IStanzaHandler
	virtual bool stanzaReadWrite(int AHandlerId, const Jid &AStreamJid, Stanza &AStanza, bool &AAccept);
	//IStanzaRequestOwner
	virtual void stanzaRequestResult(const Jid &AStreamJid, const Stanza &AStanza);
	virtual void stanzaRequestTimeout(const Jid &AStreamJid, const QString &AStanzaId);
	//IMetaRoster
	virtual bool isEnabled() const;
	virtual Jid streamJid() const;
	virtual IRoster *roster() const;
	virtual bool isOpen() const;
	virtual QList<Jid> metaContacts() const;
	virtual Jid itemMetaContact(const Jid &AItemJid) const;
	virtual IMetaContact metaContact(const Jid &AMetaId) const;
	virtual IPresenceItem metaPresence(const Jid &AMetaId) const;
	virtual QString metaAvatarHash(const Jid &AMetaId) const;
	virtual QImage metaAvatarImage(const Jid &AMetaId, bool ANullImage = true) const;
	virtual QSet<QString> groups() const;
	virtual QList<IMetaContact> groupContacts(const QString &AGroup) const;
	virtual QSet<QString> contactGroups(const Jid &AMetaId) const;
	virtual QString setContactGroups(const Jid &AMetaId, const QSet<QString> &AGroups);
	virtual QString releaseContactItem(const Jid &AMetaId, const Jid &AItemJid);
	virtual QString deleteContactItem(const Jid &AMetaId, const Jid &AItemJid);
	virtual QString renameContact(const Jid &AMetaId, const QString &ANewName);
	virtual QString deleteContact(const Jid &AMetaId);
	virtual QString mergeContacts(const Jid &AParentId, const QList<Jid> &AChildsId);
	virtual void saveMetaContacts(const QString &AFileName) const;
	virtual void loadMetaContacts(const QString &AFileName);
signals:
	void metaRosterOpened();
	void metaAvatarChanged(const Jid &AMetaId);
	void metaPresenceChanged(const Jid &AMetaId);
	void metaContactReceived(const IMetaContact &AContact, const IMetaContact &ABefore);
	void metaActionResult(const QString &AActionId, const QString &AErrCond, const QString &AErrMessage);
	void metaRosterClosed();
	void metaRosterEnabled(bool AEnabled);
	void metaRosterStreamJidAboutToBeChanged(const Jid &AAfter);
	void metaRosterStreamJidChanged(const Jid &ABefore);
protected:
	void initialize(IPluginManager *APluginManager);
	void setEnabled(bool AEnabled);
	void clearMetaContacts();
	void removeMetaContact(const Jid &AMetaId);
	void processMetasElement(QDomElement AMetasElement, bool ACompleteRoster);
	Stanza convertMetaElemToRosterStanza(QDomElement AMetaElem) const;
	Stanza convertRosterElemToMetaStanza(QDomElement ARosterElem) const;
	void processRosterStanza(const Jid &AStreamJid, Stanza AStanza);
	void insertStanzaHandlers();
	void removeStanzaHandlers();
protected slots:
	void onStreamClosed();
	void onStreamJidAboutToBeChanged(const Jid &AAfter);
	void onStreamJidChanged(const Jid &ABefore);
	void onPresenceAdded(IPresence *APresence);
	void onPresenceReceived(const IPresenceItem &AItem, const IPresenceItem &ABefore);
	void onPresenceRemoved(IPresence *APresence);
	void onAvatarChanged(const Jid &AContactJid);
private:
	IRoster *FRoster;
	IAvatars *FAvatars;
	IPresence *FPresence;
	IStanzaProcessor *FStanzaProcessor;
private:
	int FSHIMetaContacts;
	int FSHIRosterResult;
	int FSHIRosterRequest;
	QString FOpenRequestId;
	Stanza FRosterRequest;
	QList<QString> FBlockResults;
	QList<QString> FActionRequests;
private:
	bool FOpened;
	bool FEnabled;
	QHash<Jid, Jid> FItemMetaId;
	QHash<Jid, IMetaContact> FMetaContacts;
};

#endif // METAROSTER_H
