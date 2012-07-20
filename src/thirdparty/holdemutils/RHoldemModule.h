#ifndef _HoldemUtils_RHoldemModule_h_
#define _HoldemUtils_RHoldemModule_h_

#include <windows.h>
#include "RGlobalLock.h"
#include "Common/scoped_ptr.h"
#include "Common/rbase.h"
#include "RNamedObjectsHelper.h"
#include "RShutDownManager.h"
#include "objectshutdownmanager.h"
#include <QObject>

namespace holdem_utils
{

class RHoldemModule : public QObject
{
	Q_OBJECT
public:
	RHoldemModule(LPCTSTR name, UINT shutdown_thread_message = (WM_USER + 1323));
	RHoldemModule(const GUID& guid, UINT shutdown_thread_message = (WM_USER + 1323));
	~RHoldemModule();
	bool IsStatusOk();
	bool FireShutdown();
signals:
	void shutdownRequested();
public slots:
	void shutDown();
private:
	void Init();
	//  RHoldemModule(){}
	DISALLOW_EVIL_CONSTRUCTORS(RHoldemModule);
	scoped_ptr<RGlobalLock> glock_;
	RNamedObject named_object_;
	UINT shutdown_thread_message_;
	scoped_ptr<ObjectShutdownManager> auto_shutdown_;
};

};

#endif
