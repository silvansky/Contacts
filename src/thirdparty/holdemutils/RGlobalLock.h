#ifndef _HoldemUtils_RGlobalLock_h_
#define _HoldemUtils_RGlobalLock_h_

#include "Common/rbase.h"

namespace holdem_utils
{

class RGlobalLock
{
public:
	RGlobalLock();
	virtual ~RGlobalLock();

	// Create mutex return the status of creation.
	// Takes a SECURITY_ATTRIBUTES structure.
	bool InitializeWithSecAttr(const TCHAR* name,
				   LPSECURITY_ATTRIBUTES lock_attributes);
	// Create mutex return the status of creation. Sets to default DACL.
	bool Initialize(const TCHAR* name);

	virtual bool Lock() const;
	virtual bool Lock(DWORD dwMilliseconds) const;
	virtual bool Unlock() const;

private:
	mutable HANDLE mutex_;
	DISALLOW_EVIL_CONSTRUCTORS(RGlobalLock);
};

}


#endif
