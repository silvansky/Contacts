#ifndef SYSTEMMANAGER_H
#define SYSTEMMANAGER_H

#include <QObject>
#include "utilsexport.h"

class UTILS_EXPORT SystemManager : 
	public QObject
{
	Q_OBJECT;
	struct SystemManagerData;
public:
	static SystemManager *instance();
	static int systemIdle();
	static bool isSystemIdleActive();
	static bool isWorkstationLocked();
	static bool isScreenSaverRunning();
	static bool isFullScreenMode();
public:
	void startSystemIdle();
	void stopSystemIdle();
signals:
	void systemIdleChanged(int ASeconds);
	void screenSaverChanged(bool ARunning);
	void workstationLockChanged(bool ALocked);
	void fullScreenModeChanged(bool AFullScreen);
protected slots:
	void onTimerTimeout();
	void onIdleChanged(int ASeconds);
private:
	static SystemManagerData *d;
};

#endif // SYSTEMMANAGER_H
