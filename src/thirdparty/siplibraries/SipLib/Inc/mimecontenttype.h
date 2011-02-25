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

#ifndef MIMECONTENTTYPE_H_INCLUDED
#define MIMECONTENTTYPE_H_INCLUDED

#include "sipprotocol_global.h"

#include <qstring.h>
#include "parameterlist.h"


class SIPPROTOCOL_EXPORT MimeContentType
{
public:
  MimeContentType( void );
  MimeContentType( const QString &initialtype, const QString &initialsubtype );
  MimeContentType( const QString &parseinput );
  ~MimeContentType( void );

  QString getType( void ) const { return m_type; }
  QString getSubType( void ) const { return m_subtype; }

  void setType( QString newtype );
  void setSubType( QString newsubtype );

  void setParameter( const QString &param, const QString &value );
  QString queryParameter( const QString &param );

  void parseContentType( const QString &param );

  QString type( void ) const;

  static const MimeContentType null;

  MimeContentType &operator=( const MimeContentType &t );

  bool operator==( const MimeContentType &t ) const;
  bool operator!=( const MimeContentType &t ) const;
  bool operator==( const QString &t ) const;
  bool operator!=( const QString &t ) const;

private:
  QString m_type;
  QString m_subtype;
  ParameterList parameters;
};

#endif // MIMECONTENTTYPE_H_INCLUDED
