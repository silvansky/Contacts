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

#ifndef SIPMESSAGE_H_INCLUDED
#define SIPMESSAGE_H_INCLUDED

#include "sipprotocol_global.h"

#include <Winsock2.h> // ононб

//#include <sys/time.h>
#include <qstring.h>
//#include <qptrlist.h>
#include <qlist.h>

#include "sipheader.h"
#include "sipuri.h"
#include "sipstatus.h"
#include "sipprotocol.h"
#include "sipvialist.h"
#include "sipurilist.h"

/**
* @short	Representation of a single SIP message.
* @author	Billy Biggs <bbiggs@div8.net>
*
* @ref SipMessage is used for creating and for parsing individual SIP
* messages.  It also contains a retransmission timer for use when sending the
* message.
*/
class SIPPROTOCOL_EXPORT SipMessage
{
public:
  /**
  * Create a blank SIP message.
  */
  SipMessage( void );

  /**
  * Parse the given input as a SIP message.
  */
  SipMessage( const QString& parseinput );

  /**
  * SipMessage destructor.
  */
  ~SipMessage( void );

  enum MsgType
  {
    Request,
    Response,
    BadType
  };

  /**
  * Sets the type of SIP message this represents.  See @ref
  * SipMessage::getType() for a list of possible values.
  */
  void setType( MsgType newtype ) { _msgType = newtype; }

  /**
  * Returns the type of SIP message represented.
  */
  MsgType getType( void ) const { return _msgType; }

  /**
  * Sets the method on this SIP message.  Valid only for SIP requests.
  */
  void setMethod( Sip::Method newmethod ) { _sipMethod = newmethod; }

  /**
  * Returns the method of this SIP message. Valid only for SIP requests.
  */
  Sip::Method getMethod( void ) const { return _sipMethod; }

  /**
  * Returns true if the message has a body.
  */
  bool haveBody( void ) const { return _haveBody; }

  /**
  * Sets the body of the SIP message.
  */
  void setBody( const QString& newbody );

  /**
  * Generates and returns the version string for this message.
  */
  QString getVersionString( void ) const;

  /**
  * Generates and returns the start line for this message.
  */
  QString startLine( void ) const;

  /**
  * Generates and returns all of the heads of this message.
  */
  QString messageHeaders( void );

  /**
  * Returns the message body.
  */
  QString messageBody( void ) const;

  /**
  * Generates and returns the SIP message in full.
  */
  QString message( void );

  /**
  * Parses the given input as a SIP message.
  */
  void parseMessage( const QString& parseinput );

  /**
  * Inserts a new header into the message.
  */
  void insertHeader( SipHeader::SipHeaderId id, QString data );

  /**
  * Returns true if the message contains the specified header.
  */
  bool hasHeader( SipHeader::SipHeaderId id );

  /**
  * Returns the data contained in the specified header.
  */
  QString getHeaderData( SipHeader::SipHeaderId id );

  /**
  * Sets the request URI for the message.  Relevant only if it is a SIP
  * request.  This will also set the destination host and port for the
  * message.
  */
  void setRequestUri( const SipUri &newrequri );

  /**
  * Returns the request URI for this message.  Relevant only if it is a
  * SIP request.
  */
  SipUri &getRequestUri( void ) { return _requestUri; }

  /**
  * Sets the status of the message.  Relevant only if it is a SIP
  * response.
  */
  void setStatus( const SipStatus &stat );

  /**
  * Returns the status of the message. Relevant only if it is a SIP
  * response.
  */
  const SipStatus &getStatus( void ) const { return _sipStatus; }

  /**
  * Returns a reference to the via list for this message.  Usually
  * useful for getting and setting the topmost via entry.
  */
  SipViaList &getViaList( void ) { return _viaList; }

  /**
  * Returns a reference to the Record-Route for this message.
  */
  SipUriList &getRecordRoute( void ) { return _recordRoute; }

  /**
  * Returns a reference to the contact list for this message.
  */
  SipUriList &getContactList( void ) { return _contactList; }

  /**
  * Sets the via list to be a copy of an existing via list.  Useful for
  * building responses.
  */
  void setViaList( const SipViaList &copylist );

  /**
  * Sets the contact list for this message.
  */
  void setContactList( const SipUriList &newclist );

  /**
  * Sets the record route for this message.
  */
  void setRecordRoute( const SipUriList &newrr );

  /**
  * Returns the initial timestamp placed on the message.
  */
  struct timeval *getInitialTimestamp( void ) { return &_initialTimeStamp; }

  /**
  * Returns the timestamp of the last retransmission.
  */
  struct timeval *getTimestamp( void ) { return &_timeStamp; }

  /**
  * Recalculates the timestamp on the message.
  */
  void setTimestamp( void );

  /**
  * Returns the last time tick.
  */
  unsigned int lastTimeTick( void ) const { return _lastTimeTick; }

  /**
  * Sets the current time tick.
  */
  void setTimeTick( unsigned int newtt );

  /**
  * Returns the current value of the retransmission counter for this
  * message.
  */
  unsigned int sendCount( void ) const { return _sendCount; }

  /**
  * Increments the retransmission coutner.
  */
  void incrSendCount( void );

  /**
  * Static method to create a new SIP Call ID.
  */
  static QString createCallId( void );

  void setQvalue( const QString& value );

  bool isValid( void );


private:
  void setDefaultVars( void );
  void parseStartLine( QString startline );
  void parseHeaders( const QString &inbuf );


private:
  SipViaList _viaList;
  //QPtrList<SipHeader> headerlist;
  QList<SipHeader*> _headerList;

  SipUriList _recordRoute;
  SipUriList _contactList;

  MsgType _msgType;
  Sip::Method _sipMethod;

  SipStatus _sipStatus;

  SipUri _requestUri;

  bool _haveBody;
  QString _messageBody;

  struct timeval _timeStamp;
  struct timeval _initialTimeStamp;
  unsigned int _lastTimeTick;
  unsigned int _sendCount;

  bool _haveQ;
  QString _qValue;
};

#endif // SIPMESSAGE_H_INCLUDED
