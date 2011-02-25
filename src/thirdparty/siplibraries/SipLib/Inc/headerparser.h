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

#ifndef HEADERPARSER_H_INCLUDED
#define HEADERPARSER_H_INCLUDED

#include "sipprotocol_global.h"

#include <sys/types.h>
#include <qstring.h>

/**
* Container class for the header parsing function used by dissipate.  This
* should be fixed and moved somewhere more appropriate.
*/
class SIPPROTOCOL_EXPORT HeaderParser
{
public:
  HeaderParser( void );
  ~HeaderParser( void );

  /**
  * Returns the position where the header ends, the body begins, and the
  * length of the body of the header.
  */
  static bool parse( const QString &buf, size_t bufEnd, int *headerend, int *bodystart, int *bodylength );
};

#endif // HEADERPARSER_H_INCLUDED
