#include "stdafx.h"
#include "RNamedObjectsHelper.h"
#include "RUserLight.h"
#include "Common/rdebug.h"
#include <Objbase.h>

#define kRamblerGlobalPrefix _T("Global\\RH")

namespace holdem_utils
{

RNamedObject::RNamedObject(LPCTSTR prefix, LPCTSTR name):name_(NULL){
	//RASSERT1(prefix);
	//RASSERT1(name);
	InternalInit(prefix, name);
}

RNamedObject::RNamedObject(LPCTSTR prefix, const GUID& guid):name_(NULL){
	//RASSERT1(prefix);
	TCHAR guid_str[40] = {0};
	::StringFromGUID2(guid, guid_str, 40);
	::CharUpper(guid_str);
	InternalInit(prefix, guid_str);
}

RNamedObject::~RNamedObject(){
	if (name_){
		delete[] name_;
	}
}

LPCTSTR RNamedObject::GetName() const{
	return name_;
}

/*CSecurityAttributes* RNamedObject::GetSA() {
  return &sa_;
}*/

void RNamedObject::InternalInit(LPCTSTR prefix, LPCTSTR name){
	//RASSERT1(prefix);
	//RASSERT1(name);

	// TODO(Omaha): Enable this code after we have a better understanding of
	// Private Object Namespaces.
#if 0
	if (IsPrivateNamespaceAvailable(is_machine)) {
		attr->name = kGoopdatePrivateNamespacePrefix;
	} else {
		ASSERT1(!SystemInfo::IsRunningOnVistaOrLater());
#endif
		size_t bsize = _tcslen(name)+_tcslen(prefix);

		LPTSTR sid_string = NULL;
		if (holdem_utils::GetCurrentSID(&sid_string)){
			bsize += _tcslen(sid_string);
		}
		name_ = new TCHAR[bsize+2];
		if (sid_string){
			wsprintf(name_, _T("%s%s%s"), prefix, name, sid_string);
			LocalFree(sid_string);
		}else{
			wsprintf(name_, _T("%s%s"), prefix, name);
		}
	}

}
