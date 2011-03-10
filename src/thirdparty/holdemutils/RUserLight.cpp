#include "stdafx.h"
#include "RUserLight.h"
#include "Common/rdebug.h"

#ifdef _WIN32
// dirty hack for MinGW compiler which think that XP is NT 4.0
# ifndef WINVER
#  define WINVER 0x0501
# endif
# if WINVER < 0x0501
#  include <QSysInfo>
static const QSysInfo::WinVersion wv = QSysInfo::windowsVersion();
static const int dummy = (wv >= QSysInfo::WV_2000) ?
	#   undef WINVER
	#   define WINVER 0x0501
	0 : 1;
# endif
# include <windows.h>
#endif

#include <Sddl.h>

namespace holdem_utils
{

bool GetCurrentToken(scoped_access_token* token){
	//RASSERT1(token);
	scoped_handle process_handle;
	reset(process_handle, OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, GetCurrentProcessId()));
	if (!process_handle){
		return false;//-->
	}
	HANDLE h_token;
	if (!OpenProcessToken(get(process_handle), TOKEN_QUERY, &h_token)){
		return false;//-->
	}
	reset(*token, h_token);
	return true;
}

bool GetCurrentSID(LPTSTR* sid_buffer){
	DWORD dwLength = 0;
	PTOKEN_USER pts = NULL;

	scoped_access_token access_token;
	if (!GetCurrentToken(&access_token)){
		return false;//-->
	}

	// Get required buffer size and allocate the TOKEN_GROUPS buffer.

	if (!GetTokenInformation(
				get(access_token),         // handle to the access token
				TokenUser,    // get information about the token's user
				(LPVOID) pts,   // pointer to TOKEN_GROUPS buffers
				0,              // size of buffer
				&dwLength       // receives required buffer size
				)){
		if (GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
			return false;//-->
		}
		pts = (PTOKEN_USER)HeapAlloc(GetProcessHeap(),
					     HEAP_ZERO_MEMORY, dwLength);

		if (pts == NULL){
			return false;//-->
		}
	}else{
		return false;//-->
	}

	// Get the token group information from the access token.

	if (!GetTokenInformation(
				get(access_token),         // handle to the access token
				TokenUser,    // get information about the token's user
				(LPVOID) pts,   // pointer to TOKEN_GROUPS buffer
				dwLength,       // size of buffer
				&dwLength       // receives required buffer size
				)) {
		return false;//-->
	}

	bool re = true;
	if (!ConvertSidToStringSid(pts->User.Sid, sid_buffer)){
		re = false;
	}
	HeapFree(GetProcessHeap(), 0, (LPVOID)pts);
	return re;//-->
}


}
