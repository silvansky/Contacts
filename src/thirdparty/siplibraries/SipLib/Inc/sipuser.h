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

#ifndef SIPUSER_H_INCLUDED
#define SIPUSER_H_INCLUDED

#include "sipprotocol_global.h"

#include <qstring.h>
//#include <qptrlist.h>
#include <qlist.h>

#include "sipuri.h"
#include "sipregister.h"

class SipClient;
class SipCall;

/**
* @short	Class for describing a user of a SipClient.
* @author	Billy Biggs <bbiggs@div8.net>
*
*/
class SIPPROTOCOL_EXPORT SipUser
{
public:
  /**
  * Creates a new SipUser associated with the given parent SipClient.
  * Constructs the URI using the given fullname username, and hostname.
  */
  SipUser( SipClient *parent, QString fullname, QString username, QString athostname );

  /**
  * Creates a new SipUser associated with the given parent SipClient for
  * the given URI.
  */
  SipUser( SipClient *parent, const SipUri &inituri );

  /**
  * SipUser destructor.  Deletes this user from the parent SipClient.
  * All active calls made using this user will remain active.
  */
  ~SipUser( void );

  /**
  * Creates a new SIP register server object given the hostname and port
  * and returns a pointer to the register object.  The register server
  * object should be removed from this SipUser by calling
  * @ref removeServer() before deleting.
  */
  void addServer( SipRegister *server );

  /**
  * Removes the given server from the list of register servers.
  *
  * @see addServer()
  */
  void removeServer( SipRegister *server );


  /**
  * Sets the URI this user will be known as.  Any modifications to the
  * URI will not affect existing calls, but will be used for new
  * incoming calls or locally created calls.
  */
  void setUri( const SipUri &newuri );

  /**
  * Returns the URI this user is known as.
  */
  const SipUri &getUri( void ) { return _myUri; }

  SipUri *getMyUri( void ) { return &_myUri; }

  /**
  * Returns the SipClient parent of this user.  A SipUser cannot belong
  * to more than one SipClient.
  */
  SipClient *parent( void ) { return _sipClient; }

  /**
  * Returns an iterator for the list of registration servers for this
  * user.
  */
  SipRegisterIterator getSipRegisterList( void ) const { return SipRegisterIterator( _regServers ); }

private:
  SipClient *_sipClient;
  SipUri _myUri;
  QList<SipRegister*> _regServers;
	//QPtrList<SipRegister> servers;
};

//typedef QPtrListIterator<SipUser> SipUserIterator;
typedef QListIterator<SipUser> SipUserIterator;

#endif // SIPUSER_H_INCLUDED
