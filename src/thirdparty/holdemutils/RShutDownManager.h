#ifndef _HoldemUtils_RShutDownManager_h_
#define _HoldemUtils_RShutDownManager_h_

#include "Common/rbase.h"
#include "Common/scoped_any.h"
#include "Common/scoped_ptr.h"
#include "stdafx.h"

namespace holdem_utils
{

class IShutDownManagerCallback
{
public:
	virtual void OnShutDown() = 0;
};

class RShutDownManager
{
public:
	RShutDownManager(LPCTSTR name,
			 IShutDownManagerCallback* callback);
	RShutDownManager(const GUID& guid,
			 IShutDownManagerCallback* callback);
	~RShutDownManager();
	static void __stdcall Callback(void* param, BOOLEAN timer_or_wait);
	void Start();
	void FireShutdown();
private:
	DISALLOW_EVIL_CONSTRUCTORS(RShutDownManager);
	IShutDownManagerCallback* callback_;
	RShutDownManager(){}

	scoped_event shutdown_event_;
	HANDLE wait_handle_;
};

class RAutoShutDownManager:public IShutDownManagerCallback
{
public:
	RAutoShutDownManager(LPCTSTR name,
			     UINT thread_message_id = WM_QUIT);
	RAutoShutDownManager(const GUID& guid,
			     UINT thread_message_id = WM_QUIT);
	~RAutoShutDownManager();
	void Start();
	void FireShutdown();
	virtual void OnShutDown();
private:
	DISALLOW_EVIL_CONSTRUCTORS(RAutoShutDownManager);
	HRESULT ShutdownInternal() const;

	DWORD main_thread_id_;        // The id of the thread that runs Core::Main.
	scoped_ptr<RShutDownManager> shut_down_manager_;
	UINT thread_message_id_;

};

}

#endif
