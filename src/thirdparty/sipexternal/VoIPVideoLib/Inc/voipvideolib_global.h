#ifndef VOIPVIDEOLIB_GLOBAL_H
#define VOIPVIDEOLIB_GLOBAL_H

#include <QtCore/qglobal.h>

#ifdef VOIPVIDEOLIB_LIB
# define VOIPVIDEOLIB_EXPORT Q_DECL_EXPORT
#else
# define VOIPVIDEOLIB_EXPORT Q_DECL_IMPORT
#endif

#endif // VOIPVIDEOLIB_GLOBAL_H