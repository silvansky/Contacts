/*
* Copyright (c) 2000 Billy Biggs <bbiggs@div8.net>
* Copyright (c) 2004 Wirlab <kphone@wirlab.net>
*
* This library is free software; you can redistribute it and/or modify it
* under the terms of the GNU Library General Public License as published by
* the Free Software Foundation; either version 2 of the License, or (at your
* option) any later version.
* 
* This library is distributed in the hope that it will be useful, but WITHOUT
* ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
* FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
* License for more details.
* 
* You should have received a copy of the GNU Library General Public License
* along with this library; see the file COPYING.LIB.  If not, write to the
* Free Software Foundation, Inc., 59 Temple Place - Suite 330, Boston,
* MA 02111-1307, USA.
*
*/

#ifndef SIPREGISTER_H_INCLUDED
#define SIPREGISTER_H_INCLUDED

#include "sipprotocol_global.h"

#include <qobject.h>
//#include <qptrlist.h>
#include <qlist.h>
#include <qtimer.h>

#include "sipuri.h"

class QTimer;
class SipCall;
class SipUser;
class SipClient;
class SipTransaction;

/**
* Class representing a SIP registration server and a user status with it.
*/
class SIPPROTOCOL_EXPORT SipRegister : public QObject
{
  Q_OBJECT
public:
  /**
  * Creates a new SipRegister object representing the registration with
  * the given proxy.
  */
  SipRegister( SipUser *user, const SipUri &serveruri, int expires, QString qvalue );

  /**
  * SipRegister destructor.
  */
  ~SipRegister( void );

  enum RegisterState
  {
    NotConnected,
    TryingServer,
    AuthenticationRequired,
    Connected,
    Disconnecting,
    TryingServerWithPassword,
    AuthenticationRequiredWithNewPassword,
    Reconnecting
  };

  enum AuthType
  {
    BasicAuthenticationRequired,
    DigestAuthenticationRequired,
    ProxyBasicAuthenticationRequired,
    ProxyDigestAuthenticationRequired
  };

  /**
  * Returns the current state of the registration with the server.
  */
  RegisterState getRegisterState( void ) const { return _registerState; }

  /**
  * Returns the current type of authentication required for this
  * registration.
  */
  AuthType getAuthenticationType( void ) const { return _authenticationType; }

  /**
  * Returns the SIP URI of the server.
  */
  SipUri getServerUri( void ) const { return _registrationServer; }

  /**
  * Sets the URI of the registration server.
  */
  void setOutboundProxyUri( const SipUri &newregserver );

  /**
  * Requests registration from the proxy.
  */
  void requestRegister( const QString &username = QString::null, const QString &password = QString::null );

  /**
  * Performs a 3rd party registration for the given user.
  */
  void requestRegister( const SipUri &foruri );

  /**
  * Creates a request to clear this registration.
  */
  void requestClearRegistration( void );

  /**
  * Returns a pointer to the call used for performing the registrations.
  */
  SipCall *getRegisterCall( void ) { return _pRegisterCall; }

  /**
  * Returns the realm from the authentication request.
  */
  QString getAuthRealm( void );

  /**
  * Returns the realm from the proxy authentication request.
  */
  QString getProxyAuthRealm( void );

  /**
  * Returns pointer to transaction.
  */
  SipTransaction *getCurtrans( void ) { return _pCurrSipTransaction; }

  void setAutoRegister( bool autoreg ) { _autoRegister = autoreg; }
  bool getAutoRegister( void ) { return _autoRegister; }
  void updateRegister( void );
  QString getOutboundProxy( void ) { return _outboundProxy; }
  QString getQvalue( void ) { return _qValue; }

signals:
  /**
  * This signal is sent whenever the status of this registrations has
  * changed.
  */
  void statusUpdated( void );

  /**
  * Сигнал посылается в случае пограничных статусов регистрации.
  * Когда регистрация выполнена(true) или наоборот отменена(false)
  */
  void trueRegistrationStatus( bool registrationTrue);

private slots:
  void localStatusUpdated( void );
  void register_timeout( void );

private:

  enum RegisterOp
  {
    NoRegOpPending,
    RegRequest,
    RegClear
  };
  RegisterOp _regOp;

  bool _autoRegister;
  QTimer *_pRegisterTimer;
  SipUser *_pRegUser;
  int _expiresTime;
  bool _cleanRegister;
  SipCall *_pRegisterCall;
  SipTransaction *_pCurrSipTransaction;

  RegisterState _registerState;
  AuthType _authenticationType;

  SipUri _registrationServer;

  QString _authenticationString;
  QString _authenticationResponse;
  
  QString _proxyAuthenticationString;
  QString _proxyAuthenticationResponse;

  QString _outboundProxy;
  QString _qValue;
};

//typedef QPtrListIterator<SipRegister> SipRegisterIterator;
typedef QListIterator<SipRegister*> SipRegisterIterator;

#endif // SIPREGISTER_H_INCLUDED
