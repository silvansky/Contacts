#ifndef PRESENCE_H
#define PRESENCE_H

#include <QObject>
#include <interfaces/ipresence.h>
#include <interfaces/istanzaprocessor.h>
#include <interfaces/ixmppstreams.h>
#include <utils/errorhandler.h>

class Presence :
	public QObject,
	public IPresence,
	private IStanzaHandler
{
	Q_OBJECT
	Q_INTERFACES(IPresence IStanzaHandler)
public:
	Presence(IXmppStream *AXmppStream, IStanzaProcessor *AStanzaProcessor);
	~Presence();
	virtual QObject *instance() { return this; }
	//IStanzaProcessorHandler
	virtual bool stanzaReadWrite(int AHandlerId, const Jid &AStreamJid, Stanza & AStanza, bool &AAccept);
	//IPresence
	virtual Jid streamJid() const;
	virtual IXmppStream *xmppStream() const;
	virtual bool isOpen() const;
	virtual int show() const;
	virtual bool setShow(int AShow);
	virtual QString status() const;
	virtual bool setStatus(const QString &AStatus);
	virtual int priority() const;
	virtual bool setPriority(int APriority);
	virtual QString errCondition() const;
	virtual bool setPresence(int AShow, const QString &AStatus, int APriority);
	virtual bool sendPresence(const Jid &AContactJid, int AShow, const QString &AStatus, int APriority);
	virtual IPresenceItem presenceItem(const Jid &AItemJid) const;
	virtual QList<IPresenceItem> presenceItems(const Jid &AItemJid = Jid()) const;
signals:
	void opened();
	void changed(int AShow, const QString &AStatus, int APriority);
	void received(const IPresenceItem &AItem, const IPresenceItem &ABefore);
	void sent(const Jid &AContactJid, int AShow, const QString &AStatus, int APriority);
	void aboutToClose(int AShow, const QString &AStatus);
	void closed();
protected:
	void clearItems();
protected slots:
	void onStreamError(const QString &AError);
	void onStreamClosed();
private:
	IXmppStream *FXmppStream;
	IStanzaProcessor *FStanzaProcessor;
private:
	bool FOpened;
	int FSHIPresence;
	int FShow;
	int FPriority;
	QString FStatus;
	QString FErrCondition;
	QHash<Jid, IPresenceItem> FItems;
};

#endif // PRESENCE_H
