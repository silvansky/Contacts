#ifndef DEF_TEXTFLAGS_H
#define DEF_TEXTFLAGS_H

#include <qnamespace.h>

// HINT: TF_NOSHADOW definition depends on Qt version, see Qt::TextFlag at qnamespace.h:230

#define TF_NOSHADOW          (Qt::TextBypassShaping << 1)
#define TF_DARKSHADOW        (TF_NOSHADOW << 1)
#define TF_LIGHTSHADOW       (TF_DARKSHADOW << 1)

#endif //DEF_TEXTFLAGS_H
