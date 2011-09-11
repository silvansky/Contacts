#ifndef CHATSTATES_H
#define CHATSTATES_H

#include <QMap>
#include <QTimer>
#include <definitions/menuicons.h>
#include <definitions/resources.h>
#include <definitions/namespaces.h>
#include <definitions/stanzahandlerorders.h>
#include <definitions/toolbargroups.h>
#include <definitions/optionvalues.h>
#include <definitions/optionnodes.h>
#include <definitions/optionwidgetorders.h>
#include <definitions/rosternotifyorders.h>
#include <definitions/notificationtypes.h>
#include <definitions/notificationdataroles.h>
#include <definitions/tabpagenotifypriorities.h>
#include <interfaces/ipluginmanager.h>
#include <interfaces/ichatstates.h>
#include <interfaces/ipresence.h>
#include <interfaces/istanzaprocessor.h>
#include <interfaces/imessagewidgets.h>
#include <interfaces/iservicediscovery.h>
#include <interfaces/irostersview.h>
#include <interfaces/inotifications.h>
#include <utils/options.h>

struct ChatParams
{
	ChatParams() {
		userState = IChatStates::StateUnknown;
		selfState = IChatStates::StateUnknown;
		selfLastActive = 0;
		canSendStates = false;
		notifyId = 0;
	}
	int userState;
	int selfState;
	uint selfLastActive;
	bool canSendStates;
	int notifyId;
};

class ChatStates :
	public QObject,
	public IPlugin,
	public IChatStates,
	public IStanzaHandler
{
	Q_OBJECT;
	Q_INTERFACES(IPlugin IChatStates IStanzaHandler);
public:
	ChatStates();
	~ChatStates();
	//IPlugin
	virtual QObject *instance() { return this; }
	virtual QUuid pluginUuid() const { return CHATSTATES_UUID; }
	virtual void pluginInfo(IPluginInfo *APluginInfo);
	virtual bool initConnections(IPluginManager *APluginManager, int &AInitOrder);
	virtual bool initObjects();
	virtual bool initSettings();
	virtual bool startPlugin();
	//IStanzaHandler
	virtual bool stanzaReadWrite(int AHandlerId, const Jid &AStreamJid, Stanza &AStanza, bool &AAccept);
	//IChatStates
	virtual int permitStatus(const Jid &AContactJid) const;
	virtual void setPermitStatus(const Jid AContactJid, int AStatus);
	virtual bool isEnabled(const Jid &AStreamJid, const Jid &AContactJid) const;
	virtual bool isSupported(const Jid &AStreamJid, const Jid &AContactJid) const;
	virtual int userChatState(const Jid &AStreamJid, const Jid &AContactJid) const;
	virtual int selfChatState(const Jid &AStreamJid, const Jid &AContactJid) const;
signals:
	void permitStatusChanged(const Jid &AContactJid, int AStatus) const;
	void supportStatusChanged(const Jid &AStreamJid, const Jid &AContactJid, bool ASupported) const;
	void userChatStateChanged(const Jid &AStreamJid, const Jid &AContactJid, int AState) const;
	void selfChatStateChanged(const Jid &AStreamJid, const Jid &AContactJid, int AState) const;
protected:
	bool isSendingPossible(const Jid &AStreamJid, const Jid &AContactJid) const;
	void sendStateMessage(const Jid &AStreamJid, const Jid &AContactJid, int AState) const;
	void resetSupported(const Jid &AContactJid = Jid());
	void setSupported(const Jid &AStreamJid, const Jid &AContactJid, bool ASupported);
	void setUserState(const Jid &AStreamJid, const Jid &AContactJid, int AState);
	void setSelfState(const Jid &AStreamJid, const Jid &AContactJid, int AState, bool ASend = true);
	void notifyUserState(const Jid &AStreamJid, const Jid &AContactJid);
	void registerDiscoFeatures();
protected slots:
	void onPresenceOpened(IPresence *APresence);
	void onPresenceItemReceived(IPresence *APresence, const IPresenceItem &AItem, const IPresenceItem &ABefore);
	void onPresenceClosed(IPresence *APresence);
	void onChatWindowCreated(IChatWindow *AWindow);
	void onChatWindowActivated();
	void onChatWindowTextChanged();
	void onChatWindowClosed();
	void onChatWindowDestroyed(IChatWindow *AWindow);
	void onUpdateSelfStates();
	void onOptionsOpened();
	void onOptionsClosed();
	void onOptionsChanged(const OptionsNode &ANode);
private:
	IPresencePlugin *FPresencePlugin;
	IMessageWidgets *FMessageWidgets;
	IStanzaProcessor *FStanzaProcessor;
	IServiceDiscovery *FDiscovery;
	INotifications *FNotifications;
private:
	QMap<Jid,int> FSHIMessagesIn;
	QMap<Jid,int> FSHIMessagesOut;
private:
	QTimer FUpdateTimer;
	QMap<Jid, int> FPermitStatus;
	QMap<Jid, QList<Jid> > FNotSupported;
	QMap<Jid, QMap<Jid, ChatParams> > FChatParams;
	QMap<QTextEdit *, IChatWindow *> FChatByEditor;
};

#endif // CHATSTATES_H
