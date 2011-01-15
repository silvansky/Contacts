#ifndef SIPPHONEWIDGET_H
#define SIPPHONEWIDGET_H

#include <QWidget>
//#include <QDialog>
#include "ui_sipphonewidget.h"


class SipUri;
class SipUser;
class SipCall;
class SipCallMember;
class SipTransaction;
class CallAudio;
class IncomingCall;
class SipMessage;
class KSipAuthentication;
class SipPhoneProxy;



class SipPhoneWidget : public QWidget
{
	Q_OBJECT

public:
	SipPhoneWidget(QWidget *parent = 0);
	SipPhoneWidget(KSipAuthentication *auth, CallAudio *callaudio, SipCall *initcall, SipPhoneProxy *parent, const char *name = 0);
	~SipPhoneWidget();


public:
	SipCall *getCall();
	void switchCall( SipCall *newcall );
	void setRemote( QString newremote );
	void clickDial( void );
	void clickHangup( void );
	//
public slots:
	void pleaseDial( const SipUri &dialuri );
	void callDeletedSlot( bool );
		//
signals:
		void callDeleted( bool );
		void redirectCall( const SipUri &calluri, const QString &subject );


private slots:
	void hangupCall( void );
	void dialClicked( void );
	void audioOutputDead( void );
	void acceptCall( void );
	void callMemberStatusUpdated( void );
	void updateAudioStatus( void );
	void handleRedirect( void );
	//void getUri( void );
	// void ringTimeout( void );
	void acceptCallTimeout( void );
	void remotePictureShow(const QImage&);

private:
	// Sip Stuff
	SipCall *_pSipCall;
	SipCallMember *_pSipCallMember;
	KSipAuthentication *_pSipAuthentication;

	// Audio Stuff
	CallAudio *_pAudioContoller;

	int _ringCount;
	QTimer *_pRingTimer;
	bool _isRingingTone;
	QTimer *_pAcceptCallTimer;
	QString _subject; // Клиент кому звоним

	//// DTMF Stuff
	//QTimer *dtmfsenderTimer;

	//// GUI Stuff
	QLabel *_pCurrentStatus;
	QLabel *_pCurrentAudioStatus;
	//QPushButton *_pDialButton;
	QPushButton *_pHangupButton;

	// State stuff
	enum CallState
	{
		PreDial,
		Calling,
		Connected,
		Called
	};
	CallState _currentState;

	//  // Private functions
	void forceDisconnect( void );
	bool _isHangupInitiator;

private:
	Ui::SipPhoneWidget ui;
};

#endif // SIPPHONEWIDGET_H
