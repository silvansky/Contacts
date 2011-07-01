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

#ifndef MESSAGESOCKET_H_INCLUDED
#define MESSAGESOCKET_H_INCLUDED

#include "sipprotocol_global.h"

#include <Winsock2.h>
#include <Ws2tcpip.h>
//#include <netdb.h>
//#include <netinet/in.h>

class SIPPROTOCOL_EXPORT MessageSocket
{
public:

  MessageSocket( void );
  virtual ~MessageSocket( void );

  /**
  * Connect to the remote host on the given port.
  */
  virtual int connect( unsigned int portnum ) = 0;

  /**
  * Send the given buffer of the given length.
  */
  virtual int send( const char *sendbuffer, unsigned int length ) = 0;

  /**
  * Receive into the given buffer up to the given length.
  */
  virtual int receive( char *recvbuffer, unsigned int maxlength ) = 0;

  /**
  * Open up the socket for listening on the given port.
  */
  virtual unsigned int listen( unsigned int portnum ) = 0;

  /**
  * Accept the incoming connection.
  */
  virtual int accept( void ) = 0;

  /**
  * For RTP badness.
  */
  virtual int listenOnEvenPort( int min = 0, int max = 0 ) = 0;

  /**
  * Sets the remote hostname for this socket. Be careful, this function
  * currently calls gethostbyname.
  */
  bool setHostname( const char *hostname );

  enum SocketType
  {
    None,
    SocketTCP,
    SocketUDP
  };

  /**
  * Return the socket file descriptor for use in select.
  */
  int getFileDescriptor( void ) const;// { return socketfd; }

  /**
  * Returns the socket type, either MessageSocket::SocketTCP or
  * MessageSocket::SocketUDP.
  */
  SocketType getSocketType( void ) const { return type; }

  /**
  * Returns the port number we are currently connected to or listening
  * on.
  */
  unsigned int getPortNumber( void ) const { return ourport; }

  /**
  * Forces our local port number to be something specific.
  */
  void forcePortNumber( unsigned int newport );

protected:
  struct hostent *he;
  int socketfd;
  SocketType type;
  unsigned int ourport;
  struct sockaddr_in socketaddress;
  struct sockaddr_in remoteaddress;
  bool bound;

private:
  int buffsize;
  char *ipaddress;
  char *hostname;
  char *callid;
  int port;
};

#endif // MESSAGESOCKET_H_INCLUDED
