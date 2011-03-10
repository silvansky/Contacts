#ifndef _HoldemUtils_RUserLight_h_
#define _HoldemUtils_RUserLight_h_


#include <windows.h>
#include "Common/scoped_any.h"

namespace holdem_utils
{
bool GetCurrentToken(scoped_access_token* token);
//use LocalFree to free memory sid_buffer
bool GetCurrentSID(LPTSTR* sid_buffer);
}


#endif
