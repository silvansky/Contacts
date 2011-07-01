#ifndef SIPPHONEPROXY_H
#define SIPPHONEPROXY_H

#include <QObject>
#include "sipphonewidget.h"

#include "ridentityform.h"
//#include "rsipform.h"
//#include "raudioform.h"
#include <sipcall.h>

#include "utils/jid.h"
#include <utils/customborderstorage.h>
#include <definitions/resources.h>
#include <definitions/customborder.h>

class SipClient;
class SipUser;
class CallAudio;
class SipCall;
class KSipAuthentication;
class SipRegister;
class SipPhoneWidget;



//QString const KPhoneVersion = "4.2";
int const gListenPort = 5060;
int const quitTime = 5000;
int const ringTime_1 = 200;
int const ringTime_2 = 2000;
int const acceptTime = 500;
int const acceptSubscribeSendToSubscribe_time = 5000;
int const constRegistrationExpiresTime = 900;
int const constSubscribeExpiresTime = 600;
int const constStunRequestPeriod = 60;

int const constListenPort = 5060;
int const constMinPort = 0;
int const constMaxPort = 0;
int const kphoneMinimunWidht = 220;
//QString const constStunServer = "stun.wirlab.net:3478";
QString const constStunServer = "stun.tario.ru:3478";


class SipPhoneProxy : public QObject
{
	Q_OBJECT

public:
	SipPhoneProxy(QString localAddress, const QString& sipURI, const QString& userName, const QString& password, QObject *parent);
	~SipPhoneProxy();

	bool initRegistrationData( void );
	virtual bool eventFilter(QObject *, QEvent *);

	enum State { ONLINE, OFFLINE };

	void updateIdentity( SipUser *newUser, SipRegister *newReg = 0 );


protected:
	SipPhoneWidget* DoCall( QString num, SipCall::CallType ctype );
	void setStunSrv( QString stunuri );

signals:
	void stateChanged( void );
	void callDeletedProxy( bool );
	void incomingThreadTimeChange(qint64);

public slots:
	void showIdentities( void );
	void localStatusUpdate( void );
	void stateUpdated( int id );
	void registrationStatus( bool status );
	//void trueRegistrationStatusSlot(bool state);

public slots:
	void makeNewCall( const QString& uri );
	void makeVideoCall( const QString& uri );


public slots:
	void makeRegisterProxySlot(const Jid& AStreamJid, const Jid& AContactJid);
	void makeRegisterResponderProxySlot(const QString& AStreamId);
	void makeClearRegisterProxySlot();

	void makeInviteProxySlot(const Jid &AClientSIP);
	void makeByeProxySlot(const Jid &AClientSIP);
	void hangupCall();
	void onHangupCall();

signals:
	void registrationStatusIs(bool status, const Jid& AStreamJid, const Jid& AContactJid);
	void registrationStatusIs(bool status, const QString& AStreamId);
	void proxyStartCamera();
	void proxyStopCamera();
	void proxySuspendStateChange(bool);
	void proxyCamResolutionChange(bool);
	
	void camPresentChanged(bool);
	void micPresentChanged(bool);
	void volumePresentChanged(bool);

	// ѕараметры временно передаваемые из SipPhone
private:
	bool FRegAsInitiator;
	Jid FStreamJid;
	Jid FContactJid;
	QString FStreamId;



private slots: // ”правл€ющие слоты
	//void incomingInstantMessage( SipMessage *message );
	void incomingSubscribe( SipCallMember *member, bool sendSubscribe );
	void incomingCall( SipCall *call, QString body );
	void contactCall( void );
	void hideCallWidget( SipCall *call );

	void timerTick( void );
	void stun_timerTick( void );
	void onFullScreenState(bool);


protected:
	//virtual void closeEvent(QCloseEvent *);
	void kphoneQuit( void );

private:
	QTimer *quitTimer;

private:
	SipPhoneWidget* _pWorkWidget;//FPhoneWidget;
	CustomBorderContainer* _pWorkWidgetContainer;

	SipClient* _pSipClient;
	SipUser* _pSipUser;

	//CallAudio* _audioForSettings;
	
	// ќтдельно CallAudio в данном объекте не используетс€ и чтобы не вводить в заблуждение
	// и не создавать серьезных ошибок исключаем его отсюда
	//CallAudio *_pCallAudio;

	KSipAuthentication *_pSipAuthentication;
	SipRegister *_pSipRegister;
	RSipRegistrations *registrations;

	//int atomId;
	QTimer *_pSubscribeTimer;
	int _subscribeExpiresTime;

	bool _isOnline;
	bool _buttonSetOffline;
	bool _setSubscribeOffline;
	bool _useStunProxy;
	QString _stunProxyUri;

	// ѕуста€ ф-и€
	void sendNotify( int id, SipCallMember *member = 0 );
};

#endif // SIPPHONEPROXY_H
