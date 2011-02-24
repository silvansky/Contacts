#ifndef SIPPHONE_H
#define SIPPHONE_H

#include <definitions/namespaces.h>
#include <definitions/menuicons.h>
#include <definitions/resources.h>
#include <definitions/stylesheets.h>
#include <definitions/soundfiles.h>
#include <definitions/rosternotifyorders.h>
#include <definitions/tabpagenotifypriorities.h>
#include <definitions/actiongroups.h>
#include <definitions/toolbargroups.h>
#include <definitions/rosterlabelorders.h>
#include <definitions/rosterindextyperole.h>
#include <definitions/notificators.h>
#include <definitions/notificationdataroles.h>
#include <definitions/optionwidgetorders.h>
#include <interfaces/ipluginmanager.h>
#include <interfaces/isipphone.h>
#include <interfaces/istanzaprocessor.h>
#include <interfaces/imessageprocessor.h>
#include <interfaces/iservicediscovery.h>
#include <interfaces/irostersview.h>
#include <interfaces/inotifications.h>
#include <interfaces/ixmppstreams.h>
#include <interfaces/iconnectionmanager.h>
#include <interfaces/idefaultconnection.h>
#include <interfaces/imetacontacts.h>
#include <utils/errorhandler.h>
#include <utils/action.h>

#include "rcallcontrol.h"
//#include "sipphonewidget.h"
#include "sipphoneproxy.h"
#include "voipmediainit.h"



class SipPhone : 
	public QObject,
	public IPlugin,
	public ISipPhone,
	public IStanzaHandler,
	public IStanzaRequestOwner
{
	Q_OBJECT;
	Q_INTERFACES(IPlugin ISipPhone IStanzaHandler IStanzaRequestOwner);

public:
	SipPhone();
	~SipPhone();

	virtual QObject *instance() { return this; }
	//IPlugin
	virtual QUuid pluginUuid() const { return SIPPHONE_UUID; }
	virtual void pluginInfo(IPluginInfo *APluginInfo);
	virtual bool initConnections(IPluginManager *APluginManager, int &AInitOrder);
	virtual bool initObjects();
	virtual bool initSettings() { return true; }
	virtual bool startPlugin() { return true; }
	//IStanzaHandler
	virtual bool stanzaReadWrite(int AHandleId, const Jid &AStreamJid, Stanza &AStanza, bool &AAccept);
	//IStanzaRequestOwner
	virtual void stanzaRequestResult(const Jid &AStreamJid, const Stanza &AStanza);
	virtual void stanzaRequestTimeout(const Jid &AStreamJid, const QString &AStanzaId);
	//ISipPhone
	virtual bool isSupported(const Jid &AStreamJid, const Jid &AContactJid) const;
	virtual QList<QString> streams() const;
	virtual ISipStream streamById(const QString &AStreamId) const;
	virtual QString findStream(const Jid &AStreamJid, const Jid &AContactJid) const;
	virtual QString openStream(const Jid &AStreamJid, const Jid &AContactJid);
	virtual bool acceptStream(const QString &AStreamId);
	virtual void closeStream(const QString &AStreamId);

signals:
	void streamCreated(const QString &AStreamId);
	void streamStateChanged(const QString &AStreamId, int AState);
	void streamRemoved(const QString &AStreamId);

	// Сигналы относящиеся к взаимодействию с SIP протоколом
signals:
	void sipSendRegisterAsInitiator(const Jid &AStreamJid, const Jid &AContactJid);
	void sipSendRegisterAsResponder(const QString& AStreamId);

	void sipSendInvite(const QString &AClientSIP);
	void sipSendBye(const QString &AClientSIP);
	void sipSendUnRegister();

protected slots:
	// Действие после получения ответа на регистрацию. Регистрация инициатора.
	void sipActionAfterRegistrationAsInitiator(bool ARegistrationResult, const Jid& AStreamJid, const Jid& AContactJid);
	// Действие после получения ответа на регистрацию. Регистрация на принимающей стороне
	void sipActionAfterRegistrationAsResponder(bool ARegistrationResult, const QString &AStreamId);
	// Действие в случае ответа пользователя на INVITE
	void sipActionAfterInviteAnswer(bool AInviteStatus, const QString &AClientSIP);
	void finalActionAfterHangup();

	// Слот обработки завершения звонка
	void sipCallDeletedSlot(bool);

	void sipClearRegistration(const QString&);

	void onMetaTabWindowCreated(IMetaTabWindow*);
	void onToolBarActionTriggered(bool);
//
//	// ЗАГЛУШКИ
//	void sipRegisterInitiatorSlot(, const Jid& AStreamJid, const Jid& AContactJid);
//	void sipRegisterResponderSlot(const QString &AStreamId);
//	//void sipSendInviteSlot(const QString &AClientSIP);



protected:
	virtual void insertNotify(const ISipStream &AStream);
	virtual void removeNotify(const QString &AStreamId);
	virtual void removeStream(const QString &AStreamId);

protected slots:
	void onOpenStreamByAction(bool);
	void onAcceptStreamByAction(bool);
	void onCloseStreamByAction(bool);
	void onNotificationActivated(int ANotifyId);
	void onNotificationRemoved(int ANotifyId);
	void onRosterIndexContextMenu(IRosterIndex *AIndex, QList<IRosterIndex *> ASelected, Menu *AMenu);
	void onRosterLabelToolTips(IRosterIndex *AIndex, int ALabelId, QMultiMap<int,QString> &AToolTips, ToolBarChanger *AToolBarChanger);

	void onTabActionHangup();

	void onStreamOpened(IXmppStream *);
	void onStreamClosed(IXmppStream *);

	void onStreamCreated(const QString&);

private:
	IServiceDiscovery *FDiscovery;
	IStanzaProcessor *FStanzaProcessor;
	INotifications *FNotifications;
	//IRostersViewPlugin *FRostersViewPlugin;
	IRostersView *FRostersView;
	IMetaContacts *FMetaContacts;
	IPresencePlugin *FPresencePlugin;

	IMessageProcessor *FMessageProcessor;
private:
	int FSHISipRequest;
	QMap<QString, QString> FOpenRequests;
	QMap<QString, QString> FCloseRequests;
	QMap<QString, QString> FPendingRequests;
private:
	QMap<QString, ISipStream> FStreams;
	QMap<int, QString> FNotifies;
	QMap<QString, RCallControl*> FCallControls;

	SipPhoneProxy* FSipPhoneProxy;

	Jid userJid;
	QString sipUri;
	QString username;
	QString pass;
	QString streamId;
};

#endif // SIPPHONE_H
