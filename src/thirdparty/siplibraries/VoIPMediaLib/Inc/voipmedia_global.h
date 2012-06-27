#ifndef VOIPMEDIA_GLOBAL_H
#define VOIPMEDIA_GLOBAL_H

//#include <Qt/qglobal.h>
#include <qglobal.h>

#ifdef VOIPMEDIA_LIB
# define VOIPMEDIA_EXPORT Q_DECL_EXPORT
#else
# define VOIPMEDIA_EXPORT Q_DECL_IMPORT
#endif

#endif // VOIPMEDIA_GLOBAL_H
