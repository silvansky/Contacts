#include "stdafx.h"
#include "RHoldemModule.h"
#include "RNamedObjectsHelper.h"
#include "HoldemUtilsConfig.h"

namespace holdem_utils
{

RHoldemModule::RHoldemModule(LPCTSTR name,
			     UINT shutdown_thread_message):
	named_object_(kRamblerGlobalPrefixMutex, name),
	shutdown_thread_message_(shutdown_thread_message){
	auto_shutdown_.reset(new ObjectShutdownManager(name));
	connect(auto_shutdown_.get(), SIGNAL(shutdownRequested()), SIGNAL(shutdownRequested()));
	Init();
}

RHoldemModule::RHoldemModule(const GUID& guid,
			     UINT shutdown_thread_message):
	named_object_(kRamblerGlobalPrefixMutex, guid),
	shutdown_thread_message_(shutdown_thread_message){
	auto_shutdown_.reset(new ObjectShutdownManager(guid));
	connect(auto_shutdown_.get(), SIGNAL(shutdownRequested()), SIGNAL(shutdownRequested()));
	Init();
}

void RHoldemModule::Init(){
	glock_.reset(new RGlobalLock());
	glock_->Initialize(named_object_.GetName());
	if (!glock_->Lock(0)){
		glock_.reset();
	}else{
		auto_shutdown_->Start();
	}
}

RHoldemModule::~RHoldemModule(){
	if (glock_.get()){
		glock_->Unlock();
	}
}

bool RHoldemModule::IsStatusOk(){
	return glock_.get() != NULL;
}

bool RHoldemModule::FireShutdown(){
	auto_shutdown_->FireShutdown();
	return true;
}

void RHoldemModule::shutDown()
{
	FireShutdown();
}

}
