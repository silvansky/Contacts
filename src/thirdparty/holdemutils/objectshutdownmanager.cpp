#include "objectshutdownmanager.h"


ObjectShutdownManager::ObjectShutdownManager(const GUID& guid) :
	QObject(NULL), RAutoShutDownManager(guid, WM_USER + 1323)
{

}

ObjectShutdownManager::ObjectShutdownManager(LPCTSTR name) :
	QObject(NULL), RAutoShutDownManager(name, WM_USER + 1323)
{

}

void ObjectShutdownManager::OnShutDown()
{
	emit shutdownRequested();
}
