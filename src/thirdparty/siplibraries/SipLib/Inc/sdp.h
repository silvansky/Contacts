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

#ifndef SDP_H_INCLUDED
#define SDP_H_INCLUDED

#include "sipprotocol_global.h"

#include <qstring.h>

//#define MESSENGER

enum SIPPROTOCOL_EXPORT codecType
{
  codecUnknown,
  codecPCMU,
  codecGSM,
  codecPCMA,
  codecILBC_20,
  codecILBC_30,
  codecH261,
  codecH263,
  codecSpeex, // ПОПОВ добавил Speex
  codecH264 // ПОПОВ добавил H.264
};

/**
* Class for representing an SDP message.
*/
class SIPPROTOCOL_EXPORT SdpMessage
{
public:
  /**
  * Construct a blank SDP message object.
  */
  SdpMessage( void );

  /**
  * Parse the given SDP message.
  */
  SdpMessage( const QString &parseinput );

  /**
  * Deletes the SDP message.
  */
  ~SdpMessage( void );

  /**
  * Returns true if the parsed SDP message is valid.
  */
  bool isValid( void ) const { return _validSdp; }

  /**
  * Returns true if this SDP implies the call is on hold.
  */
  bool isOnHold( void ) const;

  /**
  * Returns the unique session name.  This is the s= line of SDP.
  */
  QString getName( void ) const { return _sessionName; }

  /**
  * Returns the IP address of the session described.
  */
  QString getIpAddress( void ) const { return _ipAddress; }

  /**
  * Returns the port on which media is being received.
  */
  unsigned int getPort( void ) const { return _portForAudio; }

  /**
  * Returns the port on which video media is being received.
  */
  unsigned int getVideoPort( void ) const { return _portForVideo; }

  /**
  * Sets the IP address to which media should be directed.
  */
  void setIpAddress( const QString &newaddr );

  /**
  * Sets the port on which audio media is to be received.
  */
  void setPort( unsigned int newport );

  /**
  * Sets the port on which video media is to be received.
  */
  void setVideoPort( unsigned int newport );

  /**
  * Sets the name of the session (the SDP s= line).
  */
  void setName( const QString &newname );

  /**
  * Parses the given input as an SDP message.
  */
  void parseInput( const QString &parseinput );

  /**
  * Builds and returns an SDP message given the contents.
  */
  QString message( const codecType c = codecPCMU,
    const codecType v = codecUnknown, QString body = QString::null ) const;

  static const SdpMessage null;

  /**
  * Compares the address and port of two messages.
  */
  bool operator==( const SdpMessage &m ) const;

  bool operator!=( const SdpMessage &m ) const;

  SdpMessage &operator=( const SdpMessage &m );

private:
  bool _validSdp;
  QString _sessionName;
  QString _ipAddress;
  unsigned int _portForAudio;
  unsigned int _portForVideo;
};

#endif // SDP_H_INCLUDED
