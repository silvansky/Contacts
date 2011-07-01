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

#ifndef SIPURI_H_INCLUDED
#define SIPURI_H_INCLUDED

#include "sipprotocol_global.h"

#include "SipProtoInit.h"

#include <stdlib.h>
#include <qstring.h>
#include <qstringlist.h>

#include "sipprotocol.h"
#include "parameterlist.h"

/**
* Class to parse and reference a complete SIP URI.
*/
class SIPPROTOCOL_EXPORT SipUri
{
public:
  /**
  * Constructor to create a blank SIP URI.
  */
  SipUri( void );

  /**
  * Constructs a SIP URI by parsing.
  */
  SipUri( const QString& parseinput );

  /**
  * Deletes the SIP URI.
  */
  ~SipUri( void );

  /**
  * Returns whether the given URI is a valid SIP URI after parsing.
  */
  bool isValid( void ) const { return _isValid; }

  /**
  * Returns the well-known SIP port.  This seems to be a worthy
  * namespace for this function.
  */
  static const unsigned int getSipPort( void ) { return 5060; };
  //static const unsigned int getSipPort( void ) { return SipProtoInit::ListenSipPort(); };


  enum UserParam
  {
    Phone,
    IP,
    NoUserParam
  };

  /**
  * Static method to return the string corresponding to a given SIP URI
  * user parameter.
  */
  static const QString getUserParamString( UserParam u );


  enum TransportParam
  {
    UDP,
    TCP,
    NoTransportParam
  };

  /**
  * Static method to return the string corresponding to a given SIP URI
  * transport parameter.
  */
  static const QString getTransportParamString( TransportParam t );

  /**
  * Set the protocol name of the URI.
  */
  void setProtocolName( QString protocol ) { _protocolName = protocol; }

  /**
  * Returns the protocol name of the URI.  For example, an http URI has
  * a protocol name of 'http'.
  */
  QString getProtocolName( void ) const { return _protocolName; }

  /**
  * Retuns only hostname and port.
  */
  QString proxyUri( void ) const;

  /**
  * Retuns only the URI (no fullname) including transport, user, method,
  * ttl, and maddr parameters.
  */
  QString uri( void ) const;

  /**
  * Returns the complete URI in name-addr form.
  */
  QString nameAddr( void ) const;

  /**
  * Returns the complete URI in name-addr form without Tag.
  */
  QString nameAddr_noTag( void ) const;

  /**
  * Returns the URI in a form acceptable for a request URI.
  */
  QString reqUri( void ) const;

  /**
  * Returns the URI for route header.
  */
  QString getRouteUri( void ) const;

  /**
  * Returns the URI for register target uri in case of loose route.
  */
  QString getRegisterUri( void ) const;

  /**
  * Returns only the user associated with this URI.  That is,
  * [username@]hostname.  Useful for determining call ends.
  */
  QString user( void ) const;

  /**
  * Returns the full name part of the URI.
  */
  QString getFullname( void ) const { return _fullName; }

  /**
  * Returns the username part of the URI.
  */
  QString getUsername( void ) const { return _userName; }

  /**
  * Returns the username for proxy.
  */
  QString getProxyUsername( void ) const { return _proxyUserName; }

  /**
  * Returns the password part of the URI.
  */
  QString getPassword( void ) const { return _password; }

  /**
  * Returns the hostname part of the URI.
  */
  QString getHostname( void ) const { return _hostName; }

  /**
  * Returns the tag associated with this URI.
  */
  QString getTag( void ) const { return _tag; }

  /**
  * Generates a tag for this URI and enables it.
  */
  void generateTag( void );

  /**
  * Sets the tag associated with this URI.
  */
  void setTag( const QString &newtag );

  /**
  * Sets the full name for this URI.
  */
  void setFullname( const QString &newfname );

  /**
  * Sets the username for this URI.
  */
  void setUsername( const QString &u );

  /**
  * Sets the proxyusername for this URI.
  */
  void setProxyUsername( const QString &u );

  /**
  * Sets the hostname for this URI.
  */
  void setHostname( const QString &hname );

  /**
  * Sets the password for this URI.
  */
  void setPassword( const QString &p );

  /**
  * Returns true if there is a username part contained in this URI.
  */
  bool hasUserInfo( void ) const { return _hasUserInfo; }

  /**
  * Returns true if there is a proxyusername part contained in this URI.
  */
  bool hasProxyUsername( void ) const { return _hasProxyUserName; }

  /**
  * Returns true if there is a password part contained in this URI.
  */
  bool hasPassword( void ) const { return _hasPassword; }

  /**
  * Sets the port number given after the hostname in this URI.
  */
  void setPortNumber( unsigned int p );

  /**
  * Returns the port number given in this URI, or 5060 if none was given.
  */
  unsigned int getPortNumber( void ) const { return _sipPort; }

  /**
  * Returns true if this URI contained a loose parameter.
  */
  bool isLooseRouting( void ) const { return _isLooseRouting; }

  /**
  * Returns true if this URI contains an lr param.
  */
  bool haslrparam( void ) const { return _has_lr; }

  /**
  * Returns true if this URI contained a transport parameter.
  */
  bool hasTransportParam( void ) const { return _hasTransportParam; }

  /**
  * Sets the transport parameter for this URI.
  */
  void setTransportParam( TransportParam t );

  /**
  * Returns true if this URI contains a user parameter.
  */
  bool hasUserParam( void ) const { return _hasUserParam; }

  /**
  * Sets the user parameter for this URI.
  */
  void setUserParam( UserParam u );

  /**
  * Returns true if this URI contains a method parameter.
  */
  bool hasMethodParam( void ) const { return _hasMethodParam; }

  /**
  * Sets the method parameter for this URI.
  */
  void setMethodParam( Sip::Method m );

  /**
  * Returns true if this URI contains a TTL parameter.
  */
  bool hasTtlParam( void ) const { return _hasTtlParam; }

  /**
  * Sets the TTL for this URI.
  */
  void setTtl( unsigned char t );

  /**
  * Returns true if this URI contains an maddr.
  */
  bool hasMaddrParam( void ) const { return _hasMaddrParam; }

  /**
  * Returns true if this URI contains an ftag.
  */
  bool hasFtag( void ) const { return _hasFTag; }

  /**
  * Returns the maddr for this URI.
  */
  QString getMaddrParam( void ) const { return _maddrHostName; }

  /**
  * Sets the maddr for this URI.
  */
  void setMaddrParam( const QString &newmaddr );

  /**
  * Returns true if this URI contains a tag.
  */
  bool hasTag( void ) const { return _hasTag; }

  /**
  * Update the uri for this URI.
  */
  void updateUri( const QString &u );

  QString getPrior( void ) const { return _qValue; };

  /**
  * Copies this URI.
  */
  SipUri &operator=( const SipUri &uri );

  /**
  * Compares this @ref SipUri with @param uri and returns true if they
  * are equal.
  * Tags are not checked by this operator, and should be checked
  * separately if required.
  */
  bool operator==( const SipUri &uri ) const;

  /**
  * Compares this @ref SipUri with @param url.  @param url is parsed
  * first.  Returns true if they are equal.
  *
  * Tags are not checked by this operator, and should be checked
  * separately if required.
  */
  bool operator==( const QString &url ) const;

  /**
  * Compares this @ref SipUri with @param uri and returns true if they
  * are not equal.
  *
  * Tags are not checked by this operator.
  */
  bool operator!=( const SipUri &uri ) const;

  /**
  * Compares this @ref SipUri with @param url.  @param url is parsed
  * first.  Returns true if they are not equal.
  *
  * Tags are not checked by this operator.
  */
  bool operator!=( const QString &url ) const;

  static const SipUri null;

private:
  void clear( void );
  void parseUri( const QString &parseinput );
  void parseParameters( const QString &parseinput, bool beforeangle );

private:
  QString _protocolName; // sip:/tel:
  QString _fullName;
  bool _hasUserInfo;
  QString _userName;

  bool _hasProxyUserName;
  QString _proxyUserName;
  
  bool _hasPassword;
  QString _password;

  bool _hasSipPort;
  unsigned int _sipPort;
  bool _isValid;

  bool _hasTransportParam;
  TransportParam _transportParam;

  bool _hasUserParam;
  UserParam _userParam;

  bool _hasMethodParam;
  Sip::Method _sipMethod;

  bool _hasTtlParam;
  unsigned char _ttl;

  bool _hasMaddrParam;
  QString _maddrHostName;

  bool _hasTag;
  QString _tag;

  QString _hostName;
  ParameterList _insideParams;
  ParameterList _outsideParams;

  bool _hasFTag;
  QString _ftag;

  bool _isLooseRouting;
  bool _has_lr;
  QString _lr;
  bool _has_q;
  QString _qValue;

  //Rest of params of the uri
  ParameterList _listOfParams;
  ParameterList _listOfParamsUri;
};

#endif // SIPURI_H_INCLUDED
