#include "stdafx.h"
#include "RShutDownManager.h"
#include "RNamedObjectsHelper.h"
#include "Common/scoped_any.h"
#include "Common/rdebug.h"
#include "HoldemUtilsConfig.h"


namespace holdem_utils
{

RShutDownManager::RShutDownManager(LPCTSTR name,
				   IShutDownManagerCallback* callback):
	callback_(callback),
	wait_handle_(INVALID_HANDLE_VALUE){
	RNamedObject named_object(kRamblerGlobalPrefixEvent, name);
	reset(shutdown_event_, ::CreateEvent(NULL, true, false, named_object.GetName()));
}

RShutDownManager::RShutDownManager(
	const GUID& guid,
	IShutDownManagerCallback* callback):
	callback_(callback),
	wait_handle_(INVALID_HANDLE_VALUE){
	RNamedObject named_object(kRamblerGlobalPrefixEvent, guid);
	reset(shutdown_event_, ::CreateEvent(NULL, true, false, named_object.GetName()));
}

void RShutDownManager::Start(){
	if (wait_handle_ != INVALID_HANDLE_VALUE){
		return;//-->
	}
	::RegisterWaitForSingleObject(&wait_handle_,
				      get(shutdown_event_),
				      &RShutDownManager::Callback,
				      callback_,
				      INFINITE,
				      0);
}

void RShutDownManager::FireShutdown(){
	::SetEvent(get(shutdown_event_));
}

RShutDownManager::~RShutDownManager(){
	if (wait_handle_ != INVALID_HANDLE_VALUE){
		::UnregisterWaitEx(wait_handle_, INVALID_HANDLE_VALUE);
	}
}

void __stdcall RShutDownManager::Callback(void* param, BOOLEAN timer_or_wait) {
	(void)timer_or_wait;
	//ATLASSERT(param);
	//ATLASSERT(!timer_or_wait);
	// Since we wait an INFINITE the wait handle is always signaled.
	IShutDownManagerCallback* cb = static_cast<IShutDownManagerCallback*>(param);
	//  ASSERT1(state->reactor);
	//  state->reactor->DoCallback(state);
	cb->OnShutDown();
}

// -----------------------------------------------------------------------

RAutoShutDownManager::RAutoShutDownManager(LPCTSTR name,
					   UINT thread_message_id):
	thread_message_id_(thread_message_id){
	main_thread_id_ = ::GetCurrentThreadId();
	shut_down_manager_.reset(new RShutDownManager(name, this));
}
RAutoShutDownManager::RAutoShutDownManager(const GUID& guid,
					   UINT thread_message_id):
	thread_message_id_(thread_message_id){
	main_thread_id_ = ::GetCurrentThreadId();
	shut_down_manager_.reset(new RShutDownManager(guid, this));
}

void RAutoShutDownManager::FireShutdown(){
	shut_down_manager_->FireShutdown();
}

RAutoShutDownManager::~RAutoShutDownManager(){
}

void RAutoShutDownManager::OnShutDown(){
	ShutdownInternal();
}

void RAutoShutDownManager::Start(){
	shut_down_manager_->Start();
}

HRESULT RAutoShutDownManager::ShutdownInternal() const {
	//  OPT_LOG(L1, (_T("[Google Update is shutting down...]")));
	//RASSERT1(::GetCurrentThreadId() != main_thread_id_);
	if (::PostThreadMessage(main_thread_id_, thread_message_id_, 0, 0)) {
		return S_OK;
	}
	//  LOG(ERROR)<<_T("Failed to post WM_QUIT");
	unsigned int exit_code = static_cast<unsigned int>(E_ABORT);
	::TerminateProcess(::GetCurrentProcess(), exit_code);
	return S_OK;
}


}

