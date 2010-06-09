#ifndef ICHATSTATES_H
#define ICHATSTATES_H

#include <utils/jid.h>

#define CHATSTATES_UUID "{8f09f9e9-0c85-4bc9-9c06-61f15112db3b}"

class IChatStates
{
public:
	enum ChatState {
		StateUnknown,
		StateActive,
		StateComposing,
		StatePaused,
		StateInactive,
		StateGone
	};
	enum PermitStatus {
		StatusDefault,
		StatusEnable,
		StatusDisable
	};
public:
	virtual QObject *instance() =0;
	virtual int permitStatus(const Jid &AContactJid) const =0;
	virtual void setPermitStatus(const Jid AContactJid, int AStatus) =0;
	virtual bool isEnabled(const Jid &AStreamJid, const Jid &AContactJid) const =0;
	virtual bool isSupported(const Jid &AStreamJid, const Jid &AContactJid) const =0;
	virtual int userChatState(const Jid &AStreamJid, const Jid &AContactJid) const =0;
	virtual int selfChatState(const Jid &AStreamJid, const Jid &AContactJid) const =0;
protected:
	virtual void permitStatusChanged(const Jid &AContactJid, int AStatus) const =0;
	virtual void supportStatusChanged(const Jid &AStreamJid, const Jid &AContactJid, bool ASupported) const =0;
	virtual void userChatStateChanged(const Jid &AStreamJid, const Jid &AContactJid, int AState) const =0;
	virtual void selfChatStateChanged(const Jid &AStreamJid, const Jid &AContactJid, int AState) const =0;
};

Q_DECLARE_INTERFACE(IChatStates,"Virtus.Plugin.IChatStates/1.0")

#endif
