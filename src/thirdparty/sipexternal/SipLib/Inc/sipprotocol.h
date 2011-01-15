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

#ifndef SIPPROTOCOL_H_INCLUDED
#define SIPPROTOCOL_H_INCLUDED

#include "sipprotocol_global.h"

#include <qstring.h>


#define NO_QOP 0
#define AUTH_QOP 1
#define AUTH_INT_QOP 2
#define UNKNOWN_QOP 256


/**
* This class is a container for items of global interest to a Sip stack.  It
* also holds a string containing the best-known local address to be placed in
* a Via or Contact header.
*/
class SIPPROTOCOL_EXPORT Sip
{
public:
  Sip( void );
  ~Sip( void );

  enum Method
  {
    MESSAGE,
    INVITE,
    ACK,
    BYE,
    OPTIONS,
    CANCEL,
    REGISTER,
    MSG,
    SUBSCRIBE,
    NOTIFY,
    INFO,
    REFER,
    BadMethod
  };

  /**
  * Static method to translate the Method enum into strings.
  */
  static const QString getMethodString( Method m );

  /**
  * String compare on the method to return a valid Method enum.
  */
  static Method matchMethod( const QString m );

  /**
  * Returns the local address we are known by.  Useful if you're trying
  * to fill in data for an SDP message, for example.
  */
  static QString getLocalAddress( void );

  /**
  * Explicitly set which IP address to put in the contact header and in
  * the Via.  This is useful when the local machine has multiple
  * interfaces and the code is unable to choose the correct one to
  * announce itself as in messages.
  *
  * SIP is inherently very NAT-unfriendly.
  *
  */
  static void setLocalAddress( const QString localaddr );

  /**
  * Parse Qop string
  */
  static int parseQop(const QString &qop);

  /**
  * Calculate the Digest authentication response.
  */
  static QString getDigestResponse( const QString &user, const QString &password,
    const QString &method, const QString &requri, const QString &authstr );

  /**
  * Calculate the Basic authentication response.
  */
  static QString getBasicResponse( const QString &user, const QString &password );
};

#endif // SIPPROTOCOL_H_INCLUDED
