#include "stdafx.h"
#include "RGlobalLock.h"
#include "Common/rdebug.h"

namespace holdem_utils
{

// Constructor.
RGlobalLock::RGlobalLock() : mutex_(NULL) {
}

bool RGlobalLock::InitializeWithSecAttr(const TCHAR* name,
					LPSECURITY_ATTRIBUTES lock_attributes) {
	//RASSERT1(!mutex_);
	mutex_ = ::CreateMutex(lock_attributes, false, name);
	//CreateMutexWithSyncAccess(name, lock_attributes);
	return mutex_ != NULL;
}

// Create mutex return the status of creation. Sets to default DACL.
bool RGlobalLock::Initialize(const TCHAR* name) {
	return InitializeWithSecAttr(name, NULL);
}

// Clean up.
RGlobalLock::~RGlobalLock() {
	if (mutex_) {
		//RVERIFY1(::CloseHandle(mutex_));
	}
};

// Wait until signaled.
bool RGlobalLock::Lock() const {
	return Lock(INFINITE);
}

bool RGlobalLock::Lock(DWORD dwMilliseconds) const {
	//RASSERT1(mutex_);

	DWORD ret = ::WaitForSingleObject(mutex_, dwMilliseconds);
	if (ret == WAIT_OBJECT_0) {
		return true;//-->
	} else if (ret == WAIT_ABANDONED) {
		return true;//-->
	}
	return false;//-->
}

// Release.
bool RGlobalLock::Unlock() const {
	//RASSERT1(mutex_);
	bool ret = (false != ::ReleaseMutex(mutex_));
	//  ASSERT(ret, (_T("ReleaseMutex failed.  Err=%i"), ::GetLastError()));
	//RASSERT(ret, (_T("ReleaseMutex failed.  Err=%i"), ::GetLastError()));
	return ret;
}

}
