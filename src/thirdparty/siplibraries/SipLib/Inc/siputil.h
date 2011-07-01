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

#ifndef SIPUTIL_H_INCLUDED
#define SIPUTIL_H_INCLUDED

#include "sipprotocol_global.h"
/**
* Container class for useful functions needed by the stack in general.  I'd
* like to put these somewhere else.  They're not needed outside of the stack.
*/
class SIPPROTOCOL_EXPORT SipUtil
{
public:
  SipUtil( void ) {}
  ~SipUtil( void ) {}

  static char *getLocalFqdn( void );
  static void setLocalFqdn( const char * );
  static bool checkFilename( const char *filename );
};

#endif // SIPUTIL_H_INCLUDED
