#ifndef ISIPPHONE_H
#define ISIPPHONE_H

#include <QUuid>
#include <utils/jid.h>

#define SIPPHONE_UUID "{28686B71-6E29-4065-8D2E-6116F2491394}"

struct ISipStream 
{
	enum Kind {
		SK_INITIATOR,
		SK_RESPONDER
	};
	enum State {
		SS_OPEN,
		SS_OPENED,
		SS_CLOSE,
		SS_CLOSED,
	};
	ISipStream() : noAnswer(false) {
		kind = SK_INITIATOR;
		state = SS_CLOSED;
	};
	int kind;
	int state;
	QString sid;
	Jid streamJid;
	Jid contactJid;
	bool noAnswer;
	//QString metaId;
};

class ISipPhone
{
	virtual QObject *instance() =0;
	virtual bool isSupported(const Jid &AStreamJid, const Jid &AContactJid) const =0;
	virtual QList<QString> streams() const =0;
	virtual ISipStream streamById(const QString &AStreamId) const =0;
	virtual QString findStream(const Jid &AStreamJid, const Jid &AContactJid) const =0;
	virtual QString openStream(const Jid &AStreamJid, const Jid &AContactJid) =0;
	virtual bool acceptStream(const QString &AStreamId) =0;
	virtual void closeStream(const QString &AStreamId) =0;
protected:
	virtual void streamCreated(const QString &AStreamId) =0;
	virtual void streamStateChanged(const QString &AStreamId, int AState) =0;
	virtual void streamRemoved(const QString &AStreamId) =0;
};

Q_DECLARE_INTERFACE(ISipPhone,"Virtus.Plugin.ISipPhone/1.0")

#endif //ISIPPHONE_H
