#ifndef ISMSMESSAGEHANDLER_H
#define ISMSMESSAGEHANDLER_H

#include <utils/jid.h>

#define SMSMESSAGEHANDLER_UUID "{7A7DBF1A-4C1C-4ba5-9A82-ACD7A204A438}"

class ISmsMessageHandler
{
public:
	virtual QObject *instance() = 0;
	virtual bool isSmsContact(const Jid &AStreamJid, const Jid &AContactJid) const =0;
	virtual int smsBalance(const Jid &AStreamJid, const Jid &AServiceJid) const =0;
	virtual bool requestSmsBalance(const Jid &AstreamJid, const Jid &AServiceJid) =0;
	virtual QString requestSmsSupplement(const Jid &AStreamJid, const Jid &AServiceJid) =0;
protected:
	virtual void smsBalanceChanged(const Jid &AStreamJid, const Jid &AServiceJid, int ABalance) =0;
	virtual void smsSupplementReceived(const QString &AId, const QString &ANumber, const QString &ACode, int ACount) =0;
	virtual void smsSupplementError(const QString &AId, const QString &ACondition, const QString &AMessage) =0;
};

Q_DECLARE_INTERFACE(ISmsMessageHandler,"Virtus.Plugin.ISmsMessageHandler/1.0")

#endif // ISMSMESSAGEHANDLER_H
