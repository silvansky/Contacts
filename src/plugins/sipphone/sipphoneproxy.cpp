#include "sipphoneproxy.h"


//#include <QSettings>
#include <QTimer>
#include <QCloseEvent>
#include <QDateTime>
#include <QMessageBox>

#include <sipclient.h>
#include <sipuser.h>
#include <sipcall.h>
#include <sipregister.h>

#include "RSipAuthentication.h"
#include "callaudio.h"

#include <utils/customborderstorage.h>
#include <definitions/resources.h>
#include <definitions/customborder.h>

//#include "RCallWidget.h"

#include "CrossDefine.h"




//SipPhoneProxy::SipPhoneProxy(QObject *parent) : QObject(parent)
SipPhoneProxy::SipPhoneProxy(QString localAddress, const QString& sipURI, const QString& userName, const QString& password, QObject *parent) : QObject(parent)
{
	//_formIdentity = NULL;
	//_formSip = NULL;
	//_formAudio = NULL;
	registrations = NULL;


	int listenPort = 5060;
	bool looseRoute = false;
	bool strictRoute = false;

	_pSipClient = NULL;
	_pSipUser = NULL;
	_audioForSettings = NULL;
	_pSipAuthentication = NULL;
	_pCallAudio = NULL;
	_pWorkWidget = NULL;
	_pSipRegister = NULL;
	//atomId = 0;
	_pSubscribeTimer = NULL;
	_subscribeExpiresTime = 0;

	registrations = NULL;
	_isOnline = false;
	_buttonSetOffline = false;
	_setSubscribeOffline = false;
	_useStunProxy = false;
	_stunProxyUri = "";
	//quitTimer = NULL;
	FRegAsInitiator = true;


	//QSettings settings;
	//Sip::setLocalAddress( settings.value( "/settings/dissipate_addr", Sip::getLocalAddress() ).toString() );
	Sip::setLocalAddress( (localAddress == "") ? Sip::getLocalAddress() : localAddress );
	//QString socketTypeStr = settings.value( "/settings/General/SocketMode", "UDP" ).toString();
	//listenPort = settings.value( "/settings/General/ListenPort", 5060 ).toInt();
	QString socketTypeStr = "UDP";
	listenPort = gListenPort;

	
	// Инициализируем SIP клиент (Терминал пользователя)
	_pSipClient = new SipClient( 0, 0, listenPort, looseRoute, strictRoute, socketTypeStr );

	//QString p = "/settings/Registration/";
	QString uriStr = sipURI;// settings.value(p + "SipUri" ).toString();

	//if( settings.value( p + "UseProxyDial", "Yes" ).toString() == "Yes" )
	//{
	//	_pSipClient->setUseProxyDial( true );
	//}
	//else
	//{
	//	_pSipClient->setUseProxyDial( false );
	//}
	_pSipClient->setUseProxyDial( true );

	//if( settings.value( "/settings/Symmetric/Signalling", "Yes" ).toString() == "Yes" )
	//{
	//	_pSipClient->setSymmetricMode( true );
	//}
	_pSipClient->setSymmetricMode( true );

	//QString hvstr = settings.value( "/settings/Symmetric/hideviamode", "NoHide" ).toString();
	//if( hvstr == "NoHide" )
	//{
	//	_pSipClient->setHideViaMode( SipClient::DontHideVia );
	//}
	//else if( hvstr == "HideHop" )
	//{
	//	_pSipClient->setHideViaMode( SipClient::HideHop );
	//}
	//else if( hvstr == "HideRoute" )
	//{
	//	_pSipClient->setHideViaMode( SipClient::HideRoute );
	//}
	_pSipClient->setHideViaMode( SipClient::DontHideVia );

	// Создаем пользователя для SIP-клиента (_pSipClient).
	// Пользователь идентифицируется по URI из настроек (uriStr)
	SipUri userURI = SipUri( uriStr );
	userURI.setProxyUsername(userName);
	userURI.setPassword(password);

	//_pSipUser = new SipUser( _pSipClient, SipUri( uriStr ) );
	_pSipUser = new SipUser( _pSipClient, userURI );

	//connect( _pSipClient, SIGNAL( updateSubscribes() ), this, SLOT( updateSubscribes() ) );
	connect( _pSipClient, SIGNAL( incomingSubscribe( SipCallMember *, bool ) ), this, SLOT( incomingSubscribe( SipCallMember *, bool ) ) );
	connect( _pSipClient, SIGNAL( incomingCall( SipCall *, QString ) ), this, SLOT( incomingCall( SipCall *, QString ) ) );
	connect( _pSipClient, SIGNAL( hideCallWidget( SipCall * ) ), this, SLOT( hideCallWidget( SipCall * ) ) );
	//connect( _pSipClient, SIGNAL( incomingInstantMessage( SipMessage * ) ), this, SLOT( incomingInstantMessage( SipMessage * ) ) );


	// То, что болтается KPhoneView
	_pSipRegister = 0;
	_buttonSetOffline = false;
	_setSubscribeOffline = false;

	// Объект аутентификации. Используется для запроса пароля в случае если его нет или он не верен.
	// Для этого в объекте есть слот: authRequest
	// Здесь только инициализация. Использование в других ф-иях
	_pSipAuthentication = new KSipAuthentication();


	//atomId = 1000;
	//p = "/settings/local/";
	//_subscribeExpiresTime = settings.value( p + "SubscribeExpiresTime", constSubscribeExpiresTime ).toInt();
	_subscribeExpiresTime = constSubscribeExpiresTime;
	if( _subscribeExpiresTime == 0 )
	{
		_subscribeExpiresTime = -1;
	}
	_pSubscribeTimer = new QTimer( this );
	connect( _pSubscribeTimer, SIGNAL( timeout() ), this, SLOT( localStatusUpdate() ) );
	//isOnline = false;
	//p = "/settings/presence/";

	//int count = 0;
	//label.setNum( count );
	//label = p + "/Rejected" + label;
	//QString s = settings.readEntry( label, "" );
	//while( !s.isEmpty() )
	//{
	//  rejectedContactList.append( s );
	//  label.setNum( ++count );
	//  label = p + "/Rejected" + label;
	//  s = settings.readEntry( label, "" );
	//}

	// Объект CallAudio создается для изменения настроек Audio (что-то тут не чисто... не правильно как-то)
	_audioForSettings = new CallAudio();

	if( uriStr != QString::null )
	{
		//QString p = "/settings/Registration/";
		//QString userdefaultproxy = settings.value( p + "SipServer", "" ).toString();
		QString userdefaultproxy = "";
		if( userdefaultproxy.toLower() == "sip:" )
		{
			userdefaultproxy = "";
		}
		updateIdentity(_pSipUser);
		//view->updateIdentity( user );
		_pSipClient->updateIdentity( _pSipUser, userdefaultproxy );
	}

	//registrations = new RSipRegistrations(_pSipClient, this, this);

	////_formIdentity = new RIdentityForm( _client, view, this, getUserPrefix() );
	////view->identities( registrations );
	//identities( registrations );


	//if( registrations->getUseStun() )
	//{
	//	//view->setStunSrv( registrations->getStunSrv() );
	//	setStunSrv( registrations->getStunSrv() );
	//	_pSipClient->sendStunRequest( registrations->getStunSrv() );
	//	QTimer *stun_timer = new QTimer( this );
	//	connect( stun_timer, SIGNAL( timeout() ), this, SLOT( stun_timerTick() ) );
	//	p = "/settings/Registration/";
	//	int timeout = settings.value( p + "RequestPeriod", constStunRequestPeriod ).toInt();
	//	if( timeout > 0 )
	//	{
	//		stun_timer->start( timeout * 1000 );
	//	}
	//}

	//QTimer *timer = new QTimer( this );
	//connect( timer, SIGNAL( timeout() ), this, SLOT( timerTick() ) );

	//timer->start( 20 );

}

SipPhoneProxy::~SipPhoneProxy()
{

}



bool SipPhoneProxy::initRegistrationData( void )
{
	//QSettings settings;


	SipRegistrationData regData;
	//{
	//  // Адрес SIP сервера для регистрации
	//  // Остается пустым если задан sipUserUri
	//  QString sipServerUri;
	//  // Порт SIP сервера
	//  int sipServerPort;
	//  // Время действия регистрации
	//  int registrationExpiresTime;
	//  // URI текущего пользователя
	//  QString sipUserUri;
	//  // Логин и пароль для регистрации
	//  QString userName;
	//  QString password;
	//  // Флаг использования STUN сервера
	//  bool useStun;
	//  // Адрес STUN сервера включая порт в формате stunserver.domen:3478
	//  QString stunServerWithPort;
	//  // Значение qValue
	//  QString qValue;
	//  // Флаг авторегистрации
	//  bool autoRegister;
	//};

	//QString pp = "/settings/local/";

	regData.registrationExpiresTime = constRegistrationExpiresTime;// settings.value( pp + "/RegistrationExpiresTime", constRegistrationExpiresTime ).toInt();
	if( regData.registrationExpiresTime == 0 )
	{
		regData.registrationExpiresTime = -1;
	}

	//pp = "/settings/Registration/";
	//regData.sipUserUri = _pSipUser->getUri().getRegisterUri();//  settings.value( pp + "/SipUri" ).toString();
	regData.sipUserUri = _pSipUser->getUri().nameAddr_noTag();

	//SipUri userURI = _pSipUser->getUri();
	//QString res;
	//res = "userURI: " + _pSipUser->getUri().nameAddr_noTag() + " pass: " + userURI.getPassword();
	//QMessageBox::information(NULL, "debug", res);

	regData.useStun = false;
	//if( settings.value( "/settings/STUN/UseStun", "No" ).toString() == "Yes" )
	//{
	//	regData.useStun = true;
	//}

	if(regData.useStun)
	{
		regData.stunServerWithPort = "talkpad.ru:5065";// settings.value( "/settings/STUN/StunServer", constStunServer ).toString();
		if( regData.stunServerWithPort.isEmpty() )
		{
			QString dname = _pSipUser->getMyUri()->getHostname();
			regData.stunServerWithPort = dname;
			QString srv = _pSipClient->getSRV( QString( "_stun._udp." ) + dname );
			if( !srv.isEmpty() )
			{
				regData.stunServerWithPort = srv;
			}
			regData.stunServerWithPort += ":3478";
		}
		else
		{
			if( !regData.stunServerWithPort.contains( ':' ) )
			{
				regData.stunServerWithPort += ":3478";
			}
		}
	}

	//if( settings.value( pp + "/SipServer", "" ).toString() != "" )
	//{
	//	regData.sipServerUri = settings.value( pp + "/SipServer" ).toString();
	//}
	//regData.sipServerUri = "81.19.69.224";
	regData.sipServerUri = "81.19.70.66";
	regData.qValue = "";// settings.value( pp + "/qValue", "" ).toString();
	//regData.userName = settings.value( pp + "/UserName" ).toString();
	//regData.password = settings.value( pp + "/Password" ).toString();
	//QString str = settings.value( pp + "/AutoRegister" ).toString();
	//regData.autoRegister = ( str == "Yes" ) ? true : false;
	regData.autoRegister = false;


	//connect(this, SIGNAL(proxyTrueRegistrationStatus(bool)), this, SLOT(registrationStatus(bool)));

	registrations = new RSipRegistrations(regData, _pSipClient, this);
	
	if(registrations == NULL)
		return false;
	
	connect(registrations, SIGNAL(proxyTrueRegistrationStatus(bool)), this, SLOT(registrationStatus(bool)));
	//connect(registrations, SIGNAL(proxyTrueRegistrationStatus(bool)), this, SLOT(trueRegistrationStatusSlot(bool)));

	// Кнопки регистрации
	//connect(ui.buttonRegister, SIGNAL(clicked()), registrations, SLOT(makeRegister()));
	//connect(ui.buttonUnRegister, SIGNAL(clicked()), registrations, SLOT(clearRegister()));

	//identities( registrations );


	if( registrations->getUseStun() )
	{
		//view->setStunSrv( registrations->getStunSrv() );
		setStunSrv( registrations->getStunSrv() );
		_pSipClient->sendStunRequest( registrations->getStunSrv() );
		QTimer *stun_timer = new QTimer( this );
		connect( stun_timer, SIGNAL( timeout() ), this, SLOT( stun_timerTick() ) );
		//QString p = "/settings/Registration/";
		//int timeout = settings.value( p + "RequestPeriod", constStunRequestPeriod ).toInt();
		int timeout = constStunRequestPeriod;
		if( timeout > 0 )
		{
			stun_timer->start( timeout * 1000 );
		}
	}

	QTimer *timer = new QTimer( this );
	connect( timer, SIGNAL( timeout() ), this, SLOT( timerTick() ) );

	timer->start( 20 );

	return true;
}

//void SipPhoneProxy::trueRegistrationStatusSlot(bool state)
//{
//	//emit proxyTrueRegistrationStatus(state);
//	//QMessageBox::information(NULL, "debug", "SipPhoneProxy::trueRegistrationStatusSlot");
//}


void SipPhoneProxy::registrationStatus( bool status )
{
	if(FRegAsInitiator)
		emit registrationStatusIs(status, FStreamJid, FContactJid);
	else
		emit registrationStatusIs(status, FStreamId);
}

void SipPhoneProxy::timerTick( void )
{
	_pSipClient->doSelect(false);
}

void SipPhoneProxy::setStunSrv( QString stunuri )
{
	if( !stunuri.isEmpty() )
	{
		_useStunProxy = true;
		_stunProxyUri = stunuri;
	}
}

void SipPhoneProxy::stun_timerTick( void )
{
	_pSipClient->sendStunRequest();
}



void SipPhoneProxy::incomingSubscribe( SipCallMember *member, bool sendSubscribe )
{
	//clist->auditList();
	if( member == 0 )
	{
		return;
	}
	dbgPrintf( "KPhoneView: Incoming Subscribe\n" );
	bool remove_subscribe = false;
	SipUri uri = member->getUri();
	QString uristr = member->getUri().reqUri();
	//for ( QStringList::Iterator it = rejectedContactList.begin(); it != rejectedContactList.end(); ++it )
	//{
	//  if( uristr == QString(*it) )
	//  {
	//    dbgPrintf( "KPhoneView: Incoming Subscribe Rejected\n" );
	//    remove_subscribe = true;
	//  }
	//}
	SipCallIterator it( _pSipClient->getCallList() );
	SipCall* current = NULL;
	bool find = false;
	if( !remove_subscribe )
	{
		it.toFront();
		while(it.hasNext())
		{
			current = it.next();
			if(current->getCallType() == SipCall::outSubscribeCall)
			{
				if(current->getMember(uri) != NULL)
				{
					find = true;
				}
			}
		}

	}
	if( !find && !remove_subscribe)
	{
		QString uristr = member->getUri().reqUri();
		QMessageBox mb( tr("Virtus"),
			tr("Subscribe message from uri:") + "\n" + uristr + "\n\n" +
			tr("Do you want to accept and create a contact ?"),
			QMessageBox::Information,
			QMessageBox::Yes | QMessageBox::Default,
			QMessageBox::No,
			QMessageBox::Cancel | QMessageBox::Escape );
		mb.setButtonText( QMessageBox::Yes, tr("Accept") );
		mb.setButtonText( QMessageBox::No, tr("Reject permanently") );
		mb.setButtonText( QMessageBox::Cancel, tr("Reject this time") );
		switch( mb.exec() )
		{
		case QMessageBox::Yes:
			//addContactToPhoneBook( member );
			sendSubscribe = false;
			break;
		case QMessageBox::No:
			//rejectedContactList.append( member->getUri().reqUri() );
			//saveRejectContactList();
			remove_subscribe = true;
			break;
		case QMessageBox::Cancel:
			remove_subscribe = true;
			break;
		}
	}
	if( remove_subscribe )
	{
		current = NULL;
		it.toFront();
		while(it.hasNext())
		{
			current = it.next();
			if(current->getCallType() == SipCall::inSubscribeCall)
			{
				if(current->getMember(uri))
				{
					delete current;
				}
			}
		}

		return;
	}
	if( _isOnline )
	{
		connect( member, SIGNAL( statusUpdated( SipCallMember * ) ), _pSipAuthentication, SLOT( authRequest( SipCallMember * ) ) );
		sendNotify( ONLINE, member );
	}

	current = NULL;
	it.toFront();
	while(it.hasNext())
	{
		current = it.next();
		if( current->getCallType() == SipCall::outSubscribeCall )
		{
			if( current->getCallStatus() != SipCall::callDead )
			{
				if( current->getMember( uri ) )
				{
					if( sendSubscribe )
					{
						if( _pSipRegister->getRegisterState() == SipRegister::Connected )
						{
							QString uristr = current->getSubject();
							QString contactStr = current->getContactStr();
							if( current->getCallStatus() == SipCall::callInProgress )
							{
								current->getMember( uri )->requestClearSubscribe();
							}
							delete current;
							SipCall *newcall = new SipCall( _pSipUser, QString::null, SipCall::outSubscribeCall );
							newcall->setSubject( uristr );
							SipUri remoteuri( uristr );
							member = new SipCallMember( newcall, remoteuri );
							//connect( member, SIGNAL( statusUpdated( SipCallMember * ) ), clist, SLOT( auditList() ) );
							connect( member, SIGNAL( statusUpdated( SipCallMember * ) ), _pSipAuthentication, SLOT( authRequest( SipCallMember * ) ) );
							member->requestSubscribe( _subscribeExpiresTime );
							newcall->setContactStr( contactStr );
							break;
						}
					}
				}
			}
		}
	}
}




void SipPhoneProxy::localStatusUpdate( void )
{
	QString subject;
	//QIcon icon;
	SipCallIterator it( _pSipClient->getCallList() );
	if( _pSipRegister == 0 )
	{
		return;
	}
	if( _setSubscribeOffline && _pSipRegister->getRegisterState() == SipRegister::Connected )
	{
		_setSubscribeOffline = false;
		_isOnline = false;
		//QIcon icon;
		//icon.setPixmap(SHARE_DIR "/icons/offline.png", QIcon::Automatic );
		////buttonOffOnline->setIconSet( icon );
		////buttonUpdate->setEnabled( false );
		////buttonOffOnline->setEnabled( false );

		SipCall* current = NULL;
		it.toFront();
		while(it.hasNext())
		{
			current = it.next();
			if( current->getCallType() == SipCall::outSubscribeCall )
			{
				if( current->getCallStatus() == SipCall::callInProgress )
				{
					SipCallMemberIterator mIt = current->getMemberList();
					mIt.toFront();
					SipCallMember *member = mIt.next();
					if( member )
					{
						member->requestClearSubscribe();
					}
				}
			}
		}
	}
	else
	{
		if( _pSipRegister->getRegisterState() == SipRegister::NotConnected )
		{
			_isOnline = false;
			//QIcon icon1;
			//icon1.setPixmap(SHARE_DIR "/icons/offline.png", QIcon::Automatic );
			//buttonOffOnline->setIconSet( icon1 );
			//buttonUpdate->setEnabled( false );
			//buttonOffOnline->setEnabled( false );
		}
		else if( _pSipRegister->getRegisterState() == SipRegister::Connected )
		{
			if( _buttonSetOffline )
			{
				//buttonOffOnline->setEnabled( true );
			}
			else
			{
				_isOnline = true;
				//QIcon icon2;
				//icon2.setPixmap(SHARE_DIR "/icons/online.png", QIcon::Automatic );
				//buttonOffOnline->setIconSet( icon2 );
				//buttonUpdate->setEnabled( true );
				//buttonOffOnline->setEnabled( true );

				SipCall* current = NULL;
				it.toFront();
				while(it.hasNext())
				{
					current = it.next();
					if( current->getCallType() == SipCall::outSubscribeCall )
					{
						if( current->getCallStatus() != SipCall::callDead )
						{
							SipCallMemberIterator mIt = current->getMemberList();
							mIt.toFront();
							SipCallMember *member = mIt.next();
							if( member )
							{
								member->requestSubscribe( _subscribeExpiresTime );
							}
						}
					}
				}
			}
		}
	}
	emit ( stateChanged() );
}

void SipPhoneProxy::showIdentities( void )
{
	registrations->showIdentity();
}



void SipPhoneProxy::updateIdentity( SipUser *newUser, SipRegister *newReg )
{
	if( newReg != 0 )
	{
		if( _pSipRegister != 0 )
		{
			disconnect( _pSipRegister, SIGNAL( statusUpdated() ), this, SLOT( localStatusUpdate() ) );

			// ПОПОВ memory leaks finding
			delete _pSipRegister;
			_pSipRegister = NULL;
		}
		_pSipRegister = newReg;
		connect( _pSipRegister, SIGNAL( statusUpdated() ), this, SLOT( localStatusUpdate() ) );
		//localStatusUpdate();
	}
	////buttonSipUri->setText( newUser->getUri().uri() );
	//ui.currentUser->setText( newUser->getUri().uri() );

	_pSipUser = newUser;
}

SipPhoneWidget* SipPhoneProxy::DoCall( QString num, SipCall::CallType ctype )
{
	SipCall *newcall = new SipCall( _pSipUser, QString::null, ctype );

	QString subject;
	subject.sprintf( _pSipUser->getUri().uri().toLocal8Bit() );
	newcall->setSubject( subject );

	_pCallAudio = new CallAudio( this );
	_pCallAudio->readAudioSettings();
	_pCallAudio->readVideoSettings();

	SipPhoneWidget *widget = new SipPhoneWidget( _pSipAuthentication, _pCallAudio, newcall, this );
	connect(widget, SIGNAL(callDeleted(bool)), this, SIGNAL(callDeletedProxy(bool)));
	
	//CustomBorderContainer * border = CustomBorderStorage::staticStorage(RSR_STORAGE_CUSTOMBORDER)->addBorder(widget, CBS_WINDOW);


	if(_pWorkWidget != NULL)
	{
		delete _pWorkWidget;
		_pWorkWidget = NULL;
	}

	//cwList.append( widget );
	_pWorkWidget = widget;
	widget->setRemote( num );
	if( !num.isEmpty() )
	{
		widget->clickDial();
	}
	//connect( widget, SIGNAL( redirectCall( const SipUri &, const QString & ) ), this, SLOT( redirectCall( const SipUri &, const QString & ) ) );
	
	widget->show();
	//border->show();

	return widget;
}

void SipPhoneProxy::makeNewCall( const QString& uri )
{
	//if (DoCall(ui.remote->text(), SipCall::StandardCall))
	if (DoCall(uri, SipCall::videoCall))
		;//ui.remote->setText("");
}

void SipPhoneProxy::makeVideoCall( const QString& uri )
{
	//if (DoCall(uri, SipCall::videoCall))
	//  lineeditCall->setText("");
}

void SipPhoneProxy::incomingCall( SipCall *call, QString body )
{
	if( call->getSubject() == QString::null )
	{
		call->setSdpMessageMask( body );
		call->setSubject( tr("Incoming call") );
		_pCallAudio = new CallAudio( this );
		_pCallAudio->readAudioSettings();
		_pCallAudio->readVideoSettings();
		SipPhoneWidget *widget = new SipPhoneWidget(0, _pCallAudio, call, this );
		connect(widget, SIGNAL(callDeleted(bool)), this, SIGNAL(callDeletedProxy(bool)));
		//cwList.append( widget );
		
		if(_pWorkWidget != NULL)
		{
			delete _pWorkWidget;
			_pWorkWidget = NULL;
		}
		
		_pWorkWidget = widget;
		connect( widget, SIGNAL( redirectCall( const SipUri &, const QString & ) ), this, SLOT( redirectCall( const SipUri &, const QString & ) ) );
		widget->show();
		widget->clickDial();
	}
}

void SipPhoneProxy::contactCall()
{
	//////////QString subject = "";

	//////////subject = ui.remote->text();

	//////////SipCall *newcall = new SipCall( _pSipUser, QString::null, SipCall::StandardCall );
	//////////newcall->setSubject( _pSipUser->getUri().uri() );
	//////////_pCallAudio = new CallAudio( this );
	//////////_pCallAudio->readAudioSettings();
	//////////_pCallAudio->readVideoSettings();
	//////////RCallWidget *widget = new RCallWidget( _pSipAuthentication, _pCallAudio, newcall, this );
	////////////cwList.append( widget );
	//////////_pWorkWidget = widget;
	//////////connect( widget, SIGNAL( redirectCall( const SipUri &, const QString & ) ),
	//////////	this, SLOT( redirectCall( const SipUri &, const QString & ) ) );
	//////////widget->pleaseDial(subject);
	//////////widget->show();
}

void SipPhoneProxy::hideCallWidget( SipCall *call )
{
	if(_pWorkWidget->getCall() == call)
	{
		_pWorkWidget->setHidden(true);
	}
}

void SipPhoneProxy::stateUpdated( int id )
{
	QString subject;
	if( id == OFFLINE )
	{
		_isOnline = false;
		//buttonUpdate->setEnabled( false );
		//QIcon icon;
		//icon.setPixmap(SHARE_DIR "/icons/offline.png", QIcon::Automatic );
		//buttonOffOnline->setIconSet( icon );
	}
	SipCallIterator it( _pSipClient->getCallList() );

	SipCall* current = NULL;
	it.toFront();
	while(it.hasNext())
	{
		current = it.next();
		if( current->getCallType() == SipCall::inSubscribeCall )
		{
			if( current->getCallStatus() == SipCall::callInProgress )
			{
				SipCallMemberIterator mIt = current->getMemberList();
				mIt.toFront();
				SipCallMember *member = mIt.next();
				if( member )
				{
					sendNotify( id, member );
				}
			}
			else
			{
				delete current;
			}
		}
	}

	emit( stateChanged() );
}




void SipPhoneProxy::sendNotify( int id, SipCallMember *member )
{

}

void SipPhoneProxy::kphoneQuit( void )
{
	if(_pWorkWidget != NULL && !_pWorkWidget->isHidden())
		_pWorkWidget->clickHangup();

	registrations->unregAllRegistration();

	//quitTimer = new QTimer( this );
	//connect( quitTimer, SIGNAL( timeout() ), qApp, SLOT( quit() ) );
	//quitTimer->start( quitTime );
}

//void SipPhoneProxy::closeEvent(QCloseEvent * ce)
//{
//	//QWidget::closeEvent(ce);
//	ce->ignore();
//	kphoneQuit();
//}







void SipPhoneProxy::makeRegisterProxySlot(const Jid& AStreamJid, const Jid& AContactJid)
{
	FRegAsInitiator = true;
	FStreamJid = AStreamJid;
	FContactJid = AContactJid;
	registrations->makeRegister();
}

void SipPhoneProxy::makeRegisterResponderProxySlot(const QString& AStreamId)
{
	FRegAsInitiator = false;
	FStreamId = AStreamId;
	registrations->makeRegister();
}

void SipPhoneProxy::makeClearRegisterProxySlot()
{
	registrations->clearRegister();
}

void SipPhoneProxy::makeInviteProxySlot(const Jid &AClientSIP)
{

}

void SipPhoneProxy::makeByeProxySlot(const Jid &AClientSIP)
{

}

void SipPhoneProxy::hangupCall()
{
	QMessageBox::information(NULL, "SipPhoneProxy::hangupCall()", "");
	if(_pWorkWidget)
	{
		_pWorkWidget->clickHangup();
	}
}