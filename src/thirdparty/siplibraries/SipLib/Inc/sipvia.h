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

#ifndef SIPVIA_H_INCLUDED
#define SIPVIA_H_INCLUDED

#include "sipprotocol_global.h"

#include <qstring.h>
#include "parameterlist.h"

/**
* This class represents information for a single SIP Via header.
*/
class SIPPROTOCOL_EXPORT SipVia
{
public:
  /**
  * Creates a blank @ref SipVia object.
  */
  SipVia( void );

  /**
  * Parses the input as a SIP Via.
  */
  SipVia( const QString &parseinput );

  /**
  * SipVia destructor.
  */
  ~SipVia( void );

  enum Transport
  {
    UDP,
    TCP,
    TLS,
    BadTransport
  };

  /**
  * Static method to convert a transport parameter into a string.
  */
  static const QString getTransportString( Transport t );

  /**
  * Static method to convert a transport parameter into the
  * SipVia::Transport enum.
  */
  static Transport matchTransport( const QString t );

  enum ViaParam
  {
    Hidden,
    Ttl,
    Maddr,
    Received,
    Branch,
    BadViaParam,
    Rport
  };

  /**
  * Static method to convert a Via parameter into a string.
  */
  static const QString getViaParamString( ViaParam v );

  /**
  * Static method to convert a Via parameter into a SipVia::ViaParam
  * enum.
  */
  static ViaParam matchViaParam( const QString v );

  /**
  * Returns true if the parsed Via is valid.
  */
  bool isValid( void ) const { return _isValid; }

  /**
  * Sets the hostname to place in the Via.
  */
  void setHostname( const QString& hname );

  /**
  * Returns the hostname in this via.
  */
  QString getHostname( void ) const { return _hostName; }

  /**
  * Sets the transport type for this Via, TCP or UDP.
  */
  void setTransport( Transport t );

  /**
  * Generates and returns the Via header data.
  */
  QString via( void );

  /**
  * Returns the protocol name listed in the via.
  */
  QString getProtocolName( void ) const { return _protocolName; }

  /**
  * Returns the protocol version listed in the via.
  */
  QString getProtocolVer( void ) const { return _protocolVersion; }

  /**
  * Returns the transport for this Via, TCP or UDP.
  */
  Transport getTransport( void ) const { return _transport; }

  /**
  * Sets the port number listed in this via.
  */
  void setPortNumber( unsigned int p );

  /**
  * Returns the port number listed for this via, using 5060 as the
  * default.
  */
  unsigned int getPortNumber( void ) const { return _port; }

  /**
  * Returns true if the via has a received tag.
  */
  bool hasReceivedParam( void ) const { return _hasReceived; }

  /**
  * Returns the address in the received tag of the Via, if present.
  */
  const QString &getReceivedParam( void ) const { return _received; }

  /**
  * Sets the address in the received tag of the Via.  If QString::null
  * is given, then there is no received.  This is the default.
  */
  void setReceivedParam( const QString &newreceived );

  /**
  * Returns true if the via has an rport tag.
  */
  bool hasRportParam( void ) const { return _hasRPort; }

  /**
  * Returns the port in the rport tag of the Via, if present.
  */
  const QString &getRportParam( void ) const { return _rport; }

  /**
  * Sets the port in the rport tag of the Via. If QString::null
  * is given, then there is no rport. This is the default.
  */
  void setRportParam( const QString &newrport );

  /**
  * Returns true if the via is hidden.
  */
  bool isHidden( void ) const { return _isHidden; }

  /**
  * Sets the 'hidden' flag for this via.  By default, Via headers are
  * not hidden.
  */
  void setHidden( bool hidden );

  /**
  * Returns true if this via has a ttl parameter.
  */
  bool hasTtlParam( void ) const { return _hasTtl; }

  /**
  * Returns the TTL for this Via.
  */
  const QString &getTtlParam( void ) const { return _ttl; }

  /**
  * Sets the ttl parameter for this via.  If QString::null is given,
  * there is no ttl.  This is the default.
  */
  void setTtlParam( const QString &newttl );

  /**
  * Returns true if the via has a maddr parameter.
  */
  bool hasMaddrParam( void ) const { return _hasMaddr; }

  /**
  * Returns the maddr for this Via.
  */
  const QString &getMaddrParam( void ) const { return _maddr; }

  /**
  * Sets the maddr parameter for this via.  If QString::null is given,
  * there is no maddr.  This is the default.
  */
  void setMaddrParam( const QString &newmaddr );

  /**
  * Returns true if the via has a branch parameter.
  */
  bool hasBranchParam( void ) const { return _hasBranch; }

  /**
  * Returns the branch for this Via.
  */
  const QString &getBranchParam( void ) const { return _branch; }

  /**
  * Sets the branch parameter for this via.  If QString::null is given,
  * there is no branch.  This is the default.
  */
  void setBranchParam( const QString &newbranch );

  /**
  * Generate a random branch parameter.
  */
  void generateBranchParam( void );

  SipVia &operator=( const SipVia &v );

  bool operator==( const SipVia &v ) const;
  bool operator!=( const SipVia &v ) const;
  bool operator==( const QString &v ) const;
  bool operator!=( const QString &v ) const;


private:
  void clear( void );
  void parseVia( const QString& parseinput );

private:
  bool _isValid;

  QString _hostName;

  Transport _transport;

  QString _protocolName;
  QString _protocolVersion;

  bool _hasReceived;
  QString _received;

  bool _hasRPort;
  QString _rport;

  bool _isHidden;

  bool _hasTtl;
  QString _ttl;

  bool _hasMaddr;
  QString _maddr;

  bool _hasBranch;
  QString _branch;

  unsigned int _port;
  bool _hasPort; 

  //rest of via params
  ParameterList _restOfViaParams;

};

#endif // SIPVIA_H_INCLUDED
