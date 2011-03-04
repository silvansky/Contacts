#include "systemmanager.h"

#include <QTimer>
#include <thirdparty/idle/idle.h>

#ifdef Q_WS_WIN
#include <windows.h>
#endif

struct SystemManager::SystemManagerData
{
	SystemManagerData() {
		idle = NULL;
		idleSeconds = 0;
		workstationLocked = false;
		screenSaverRunning = false;
	}
	Idle *idle;
	QTimer *timer;
	int idleSeconds;
	bool workstationLocked;
	bool screenSaverRunning;
};

SystemManager::SystemManagerData *SystemManager::d = new SystemManager::SystemManagerData;

SystemManager *SystemManager::instance()
{
	static SystemManager *manager = NULL;
	if (!manager)
	{
		manager = new SystemManager;
		manager->d->idle = new Idle;
		connect(manager->d->idle,SIGNAL(secondsIdle(int)),manager,SLOT(onIdleChanged(int)));

		manager->d->timer = new QTimer(manager);
		manager->d->timer->setInterval(1000);
		manager->d->timer->setSingleShot(false);
		manager->d->timer->start();
		connect(manager->d->timer,SIGNAL(timeout()),manager,SLOT(onTimerTimeout()));
	}
	return manager;
}

int SystemManager::systemIdle()
{
	return d->idleSeconds;
}

bool SystemManager::isSystemIdleActive()
{
	return d->idle!=NULL ? d->idle->isActive() : false;
}

bool SystemManager::isWorkstationLocked()
{
#ifdef Q_WS_WIN
	HDESK hDesk = OpenInputDesktop(0, FALSE, DESKTOP_READOBJECTS);
	if (hDesk == NULL)
		return TRUE;

	TCHAR szName[80];
	DWORD cbName;
	BOOL bLocked;

	bLocked = !GetUserObjectInformation(hDesk, UOI_NAME, szName, 80, &cbName) || lstrcmpi(szName, L"default") != 0;

	CloseDesktop(hDesk);
	return bLocked;
#endif
	return false;
}

bool SystemManager::isScreenSaverRunning()
{
#ifdef Q_WS_WIN
	BOOL fResult;
	BOOL aRunning;
	fResult = SystemParametersInfo(SPI_GETSCREENSAVERRUNNING,0,&aRunning,0);
	return fResult && aRunning;
#endif
	return false;
}

void SystemManager::startSystemIdle()
{
	if (d->idle && !d->idle->isActive())
		d->idle->start();
}

void SystemManager::stopSystemIdle()
{
	if (d->idle && d->idle->isActive())
		d->idle->stop();
}

void SystemManager::onTimerTimeout()
{
	bool saverRunning = isScreenSaverRunning();
	if (d->screenSaverRunning != saverRunning)
	{
		d->screenSaverRunning = saverRunning;
		emit screenSaverChanged(saverRunning);
	}

	bool stationLocked = isWorkstationLocked();
	if (d->workstationLocked != stationLocked)
	{
		d->workstationLocked = stationLocked;
		emit workstationLockChanged(stationLocked);
	}
}

void SystemManager::onIdleChanged(int ASeconds)
{
	d->idleSeconds = ASeconds;
	emit systemIdleChanged(ASeconds);
}
