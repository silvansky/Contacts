#ifndef METAROSTER_H
#define METAROSTER_H

#include <definitions/namespaces.h>
#include <definitions/stanzahandlerorders.h>
#include <interfaces/imetacontacts.h>
#include <interfaces/istanzaprocessor.h>
#include <utils/stanza.h>

class MetaRoster : 
	public QObject,
	public IMetaRoster,
	public IStanzaHandler,
	public IStanzaRequestOwner
{
	Q_OBJECT;
	Q_INTERFACES(IMetaRoster IStanzaHandler IStanzaRequestOwner);
public:
	MetaRoster(IRoster *ARoster, IStanzaProcessor *AStanzaProcessor);
	~MetaRoster();
	virtual QObject *instance() { return this; }
	//IStanzaHandler
	virtual bool stanzaReadWrite(int AHandlerId, const Jid &AStreamJid, Stanza &AStanza, bool &AAccept);
	//IStanzaRequestOwner
	virtual void stanzaRequestResult(const Jid &AStreamJid, const Stanza &AStanza);
	virtual void stanzaRequestTimeout(const Jid &AStreamJid, const QString &AStanzaId);
	//IMetaRoster
	virtual Jid streamJid() const;
	virtual IRoster *roster() const;
	virtual bool isOpen() const;
	virtual QList<Jid> metaContacts() const;
	virtual IMetaContact metaContact(const Jid &AMetaId) const;
	virtual IMetaContact itemMetaContact(const Jid &AItemJid) const;
	virtual QSet<QString> groups() const;
	virtual QList<IMetaContact> groupContacts(const QString &AGroup) const;
signals:
	void metaRosterOpened();
	void metaContactReceived(const IMetaContact &AContact, const IMetaContact &ABefore);
	void metaRosterClosed();
protected:
	void processMetasElement(QDomElement AMetasElement, bool ACompleteRoster);
	Stanza convertMetaElemToRosterStanza(QDomElement AMetaElem) const;
	Stanza convertRosterElemToMetaStanza(QDomElement ARosterElem) const;
	void processRosterStanza(const Jid &AStreamJid, Stanza AStanza);
	void insertStanzaHandlers();
	void removeStanzaHandlers();
protected slots:
	void onStreamClosed();
private:
	IRoster *FRoster;
	IStanzaProcessor *FStanzaProcessor;
private:
	int FSHIMetaContacts;
	int FSHIRosterResult;
	int FSHIRosterRequest;
	QString FOpenRequestId;
	Stanza FRosterRequest;
	QList<QString> FBlockResults;
private:
	bool FOpened;
	QHash<Jid, Jid> FItemMetaId;
	QHash<Jid, IMetaContact> FMetaContacts;
};

#endif // METAROSTER_H
