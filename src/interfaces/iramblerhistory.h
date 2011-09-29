#ifndef IRAMBLERHISTORY_H
#define IRAMBLERHISTORY_H

#include <QDateTime>
#include <utils/jid.h>
#include <utils/message.h>

#define RAMBLERHISTORY_UUID "{4460E68D-CC30-41df-A63D-F4F11A894CC7}"

#define HISTORY_SAVE_TRUE      "true"
#define HISTORY_SAVE_FALSE     "false"

struct IHistoryItemPrefs
{
	QString save;
	bool operator==(const IHistoryItemPrefs &AOther) const {
		return save==AOther.save;
	}
	bool operator!=(const IHistoryItemPrefs &AOther) const {
		return !operator==(AOther);
	}
};

struct IHistoryStreamPrefs
{
	QString autoSave;
	IHistoryItemPrefs defaultPrefs;
	QHash<Jid,IHistoryItemPrefs> itemPrefs;
};

struct IHistoryRetrieve 
{
	Jid with;
	int count;
	QString beforeId;
	QDateTime beforeTime;
};

struct IHistoryMessages 
{
	Jid with;
	QString beforeId;
	QDateTime beforeTime;
	QList<Message> messages;
};

class IRamblerHistory
{
public:
	virtual QObject *instance() = 0;
	virtual bool isReady(const Jid &AStreamJid) const =0;
	virtual bool isSupported(const Jid &AStreamJid) const =0;
	virtual IHistoryStreamPrefs historyPrefs(const Jid &AStreamJid) const =0;
	virtual IHistoryItemPrefs historyItemPrefs(const Jid &AStreamJid, const Jid &AItemJid) const =0;
	virtual QString setHistoryPrefs(const Jid &AStreamJid, const IHistoryStreamPrefs &APrefs) =0;
	virtual QString removeHistoryItemPrefs(const Jid &AStreamJid, const Jid &AItemJid) =0;
	virtual QString loadServerMessages(const Jid &AStreamJid, const IHistoryRetrieve &ARetrieve) =0;
	virtual QWidget *showViewHistoryWindow(const Jid &AStreamJid, const Jid &AContactJid) =0;
protected:
	virtual void requestCompleted(const QString &AId) =0;
	virtual void requestFailed(const QString &AId, const QString &AError) =0;
	//History preferences
	virtual void historyPrefsChanged(const Jid &AStreamJid, const IHistoryStreamPrefs &APrefs) =0;
	virtual void historyItemPrefsChanged(const Jid &AStreamJid, const Jid &AItemJid, const IHistoryItemPrefs &APrefs) =0;
	//Server History
	virtual void serverMessagesLoaded(const QString &AId, const IHistoryMessages &AMessages) =0;
};

Q_DECLARE_INTERFACE(IRamblerHistory,"Virtus.Plugin.IRamblerHistory/1.0")

#endif // IRAMBLERHISTORY_H
