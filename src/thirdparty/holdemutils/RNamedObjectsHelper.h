#ifndef _RNamedObjectsHelper_h_
#define _RNamedObjectsHelper_h_

#include "Common/rbase.h"
//#include <atlsecurity.h>


namespace holdem_utils
{

class RNamedObject
{
public:
	RNamedObject(LPCTSTR prefix, LPCTSTR name);
	RNamedObject(LPCTSTR prefix, const GUID& guid);
	~RNamedObject();
	LPCTSTR GetName() const;
	//  CSecurityAttributes* GetSA();
private:
	RNamedObject(){}
	DISALLOW_EVIL_CONSTRUCTORS(RNamedObject);
	void InternalInit(LPCTSTR prefix, LPCTSTR name);

	LPTSTR name_;
	//  CSecurityAttributes sa_;
};

/*void GetNamedObjectAttributes(const TCHAR* base_name,
			      bool is_machine,
			      NamedObjectAttributes* attr) {*/



};

#endif
