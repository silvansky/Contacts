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

#ifndef SIPCLIENT_H_INCLUDED
#define SIPCLIENT_H_INCLUDED

#include "sipprotocol_global.h"

#include <qobject.h>
#include <qstring.h>
//#include <qptrlist.h>
#include <qlist.h>

#include "sipuri.h"
#include "udpmessagesocket.h"
#include "tcpmessagesocket.h"
#include "sipcall.h"
#include "sipuser.h"

class SipStatus;
class SipMessage;
class SipCallMember;
class MimeContentType;

typedef unsigned char  UInt8;
typedef unsigned short UInt16;
typedef unsigned int   UInt32;
struct UInt128 { unsigned char octet[16]; };

// define types for a stun message
const UInt16 BindRequestMsg          = 0x0001;
const UInt16 BindResponseMsg         = 0x0101;
const UInt16 BindErrorMsg            = 0x0111;
const UInt16 SharedSecretRequestMsg  = 0x0002;
const UInt16 SharedSecretResponseMsg = 0x0102;

/// define a structure to hold a stun address
const UInt8  IPv4Family = 0x01;
const UInt8  IPv6Family = 0x02;

// define  stun attribute
const UInt16 MappedAddress = 0x0001;

struct StunAddrHdr
{
  UInt8 pad;
  UInt8 family;
  UInt16 port;
};

struct StunAtrHdr
{
  UInt16 type;
  UInt16 length;
};

struct StunMsgHdr
{
  UInt16 msgType;
  UInt16 msgLength;
  UInt128 id;
};

struct StunAddress
{
  StunAddrHdr addrHdr;
  union addr
  {
    UInt32  v4addr;
    UInt128 v6addr;
  };
};

struct StunAtrAddress
{
  UInt16 type;
  UInt16 length;
  StunAddress address;
};

struct StunAtrAddress4
{
  UInt16 type;
  UInt16 length;
  StunAddrHdr addrHdr;
  UInt32  v4addr;
};

struct StunRequestSimple
{
  StunMsgHdr msgHdr;
};


class SIPPROTOCOL_EXPORT SipClient : public QObject
{
  Q_OBJECT

  // Дружественные классы
  //friend class SipUser;
  friend class SipCall;

public:
  SipClient( QObject *parent = 0, const char *name = 0, unsigned int newListenport = 0,
             bool newLooseRoute = true, bool newStrictRoute = true, QString socketStr = QString::null );
  ~SipClient( void );

public:
  void doSelect( bool block );
  void sendStunRequest( const QString uristr = QString::null );
  const SipUri &getContactUri( void ) { return _contactUri; }
  SipUser *getUser( void ) const { return _sipUser; }
  void setUser( SipUser *user );
  void setCallForwardUri( const SipUri &u );
  const SipUri &getCallForwardUri( void ) { return _forwardUri; }
  void setCallForward( bool onoff );
  bool getCallForward( void ) const { return _fwMode; }
  void setCallForwardMessage( const QString &newmessage );
  QString getCallForwardMessage( void ) const { return _fwBody; }
  void setBusy( bool onoff );
  bool getBusy( void ) const { return _busyMode; }
  void setBusyMessage( const QString &newmessage );
  QString getBusyMessage( void ) const { return _busyBody; }
  bool getProxyMode( void ) { return _useProxyDial; }
  void setExplicitProxyMode( bool eproxy );
  bool getExplicitProxyMode( void ) { return _useExplicitProxy; }
  void setExplicitProxyAddress( const QString &newproxy );
  const QString &getExplicitProxyUri( void ) { return _proxy; }
  QString getExplicitProxyAddress( void );
  void setMaxForwards( int newmax );
  int getMaxForwards( void ) const { return _maxForwards; }

  /**
  * Via hiding mode.
  */
  enum HideViaMode
  {
    DontHideVia,
    HideHop,
    HideRoute
  };

  void setHideViaMode( HideViaMode newmode );
  HideViaMode getHideViaMode( void ) { return _hideMode; }

  /**
  * set rport parameter and be symmetic
  */
  void setSymmetricMode( bool newmode );

  /**
  * True if we are symmetric
  */
  bool getSymmetricMode( void ) { return _symmetricMode; }

  SipCallIterator getCallList( void ) const { return SipCallIterator( _sipCallsList ); }
  static const QString getUserAgent( void );
  //QString getAuthenticationUsername( void ) const { return authentication_username; }
  //QString getAuthenticationPassword( void ) const { return authentication_password; }
  QString getSipProxy( void ) const { return _proxy; }

  /**
  * Return NULL if uri don't match
  */
  SipUser *getUser( SipUri uri );

  void updateIdentity( SipUser *user, QString newproxy);

  enum Socket
  {
    UDP,
    TCP
  };
  bool isTcpSocket( void ) const { return _socketMode == TCP; }
  void setSocketMode( Socket socket ) { _socketMode = socket; }
  
  QString getSipProxySrv( QString dname );
  bool isLooseRoute( void ) const { return _looseRoute; }
  bool isStrictRoute( void ) const { return _strictRoute; }
  void sendTestMessage( QString sendaddr, unsigned int port, QString msg );
  void setTest( bool on ) { _testOn = on; }
  void setUseProxyDial( bool on ) { _useProxyDial = on; }
  QString getNAPTR( QString strUri );
  QString getSRV( QString naptr );
  void printStatus();
  TCPMessageSocketIterator getTcpSocketList( void ) const { return TCPMessageSocketIterator( _tcpSockets ); }


private slots:
  void callMemberUpdated( void );

signals:
  void incomingCall( SipCall *, QString );
  void hideCallWidget( SipCall * ); // НЕТ
  void callListUpdated( void );
  void incomingInstantMessage( SipMessage * );
  void incomingNotify( SipMessage * ); // НЕТ
  void incomingSubscribe( SipCallMember *, bool );
  void incomingTestMessage();
  void updateSubscribes( void ); // НЕТ


private:
  // Audit pending messages (retransmissions).
  void auditPending( void );

  void setupContactUri(  SipUser *user = 0 );
  bool setupSocketStuff( unsigned int newListenport, QString socketStr );
  void incomingMessage( int socketfd );
  void parseMessage( QString fullmessage );
  void sendQuickResponse( SipMessage *origmessage, const SipStatus &status,
                          const QString &body = QString::null,
                          const MimeContentType &bodytype = MimeContentType::null );

  void sendAck( SipMessage *origmessage );

  // These methods are for SipCall
  void addCall( SipCall *call );
  void deleteCall( SipCall *call );
  bool sendRequest( SipMessage *msg, bool contact = true,
                    const SipUri &regProxy = SipUri::null, const QString &branch = QString::null );
  void sendResponse( SipMessage *msg, bool contact = true );
  void sendRaw( SipMessage *msg );
  void callTypeUpdated( void );
  QString getResSearch( QString dname, int type, bool UDP );


private:
  QList<SipCall*> _sipCallsList;
  //QPtrList<SipCall> calls;

  //QString authentication_username;
  //QString authentication_password;
  Socket _socketMode;
  // Для TCP сокета. Внутреннее использование
  int _clilen, _newsockfd;
  struct sockaddr_in _cli_addr;

  // Our proxy, if applicable.
  QString _proxy;
  unsigned int _proxyPort;
  bool _useProxyDial;
  bool _useExplicitProxy;
  SipUri _sipProxyUri;
  QString _sipProxySrv;
  QString _sipProxyName;
  SipUri _contactUri;

  // Call forwarding
  bool _fwMode;
  SipUri _forwardUri;
  QString _fwBody;

  // Busy
  bool _busyMode;
  QString _busyBody;

  // Max-forwards
  int _maxForwards;

  // Via hide mode
  HideViaMode _hideMode;

  // Symmetric signalling mode
  bool _symmetricMode;

  SipUser *_sipUser;
  UDPMessageSocket _listenerUDP;
  TCPMessageSocket _listenerTCP;

  // Log stuff to a file
  //int loggerfd;

private:
  QString _messageCID;
  QString _subscribeCID;
  bool _useStunProxy;
  SipUri _stunProxy;
  QString _messageCSeq;
  QString _subscribeCSeq;
  bool _looseRoute;  // Свободный роутинг
  bool _strictRoute; // Определенный роутинг

  bool _testOn;

  TCPMessageSocket *_tcpSocket;
  QList<TCPMessageSocket*> _tcpSockets;
  //QPtrList<TCPMessageSocket> tcpSockets;
};

#endif // SIPCLIENT_H_INCLUDED
