#include "systemmanager.h"

#include <QTimer>
#include <QProcess>
#include <QTextStream>
#include <QStringList>
#include <thirdparty/idle/idle.h>

#ifdef Q_WS_WIN
// dirty hack for MinGW compiler which think that XP is NT 4.0
# if _WIN32_WINNT < 0x0501
#  include <QSysInfo>
static const QSysInfo::WinVersion wv = QSysInfo::windowsVersion();
static const int dummy = (wv >= QSysInfo::WV_2000) ?
#   define _WIN32_WINNT 0x0501
	      0 : 1;
# endif
# include <windows.h>
#endif

#if defined(Q_WS_X11)
# include <sys/utsname.h>
#endif

struct SystemManager::SystemManagerData
{
	SystemManagerData() {
		idle = NULL;
		idleSeconds = 0;
		workstationLocked = false;
		screenSaverRunning = false;
		fullScreenEnabled = false;
	}
	Idle *idle;
	QTimer *timer;
	int idleSeconds;
	bool workstationLocked;
	bool screenSaverRunning;
	bool fullScreenEnabled;
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
	BOOL aRunning;
	if (SystemParametersInfo(SPI_GETSCREENSAVERRUNNING,0,&aRunning,0))
		return aRunning;
#endif
	return false;
}

bool SystemManager::isFullScreenMode()
{
#ifdef Q_WS_WIN
# if (_WIN32_WINNT >= 0x0500)
	static HWND shellHandle = GetShellWindow();
	static HWND desktopHandle = GetDesktopWindow();

	HWND hWnd = GetForegroundWindow();
	if (hWnd!=NULL && hWnd!=shellHandle && hWnd!=desktopHandle)
	{
		RECT appBounds;
		GetWindowRect(hWnd, &appBounds);

		RECT scrBounds;
		GetWindowRect(desktopHandle, &scrBounds);

		return appBounds.right-appBounds.left==scrBounds.right-scrBounds.left && appBounds.bottom-appBounds.top==scrBounds.bottom-scrBounds.top;
	}
# endif
#endif
	return false;
}

QString SystemManager::systemOSVersion()
{
	QString osver = "System Unknown";

#ifdef Q_WS_X11

	QStringList path;
	foreach(QString env, QProcess::systemEnvironment())
	{
		if (env.startsWith("PATH="))
			path = env.split('=').value(1).split(':');
	}

	QString found;
	foreach(QString dirname, path)
	{
		QDir dir(dirname);
		QFileInfo cand(dir.filePath("lsb_release"));
		if (cand.isExecutable())
		{
			found = cand.absoluteFilePath();
			break;
		}
	}

	if (!found.isEmpty())
	{
		QProcess process;
		process.start(found, QStringList()<<"--description"<<"--short", QIODevice::ReadOnly);
		if (process.waitForStarted())
		{
			QTextStream stream(&process);
			while (process.waitForReadyRead())
				osver += stream.readAll();
			process.close();
			osver = osver.trimmed();
		}
	}

	if (osver.isEmpty())
	{
		utsname buf;
		if (uname(&buf) != -1)
		{
			osver.append(buf.release).append(QLatin1Char(' '));
			osver.append(buf.sysname).append(QLatin1Char(' '));
			osver.append(buf.machine).append(QLatin1Char(' '));
			osver.append(QLatin1String(" (")).append(buf.machine).append(QLatin1Char(')'));
		}
		else
		{
			osver = ("Linux/Unix Unknown");
		}
	}

#elif defined(Q_WS_WIN) || defined(Q_OS_CYGWIN)

	static const struct { int ver; QString name; } versions[] = 
	{
		{ QSysInfo::WV_NT,          "Windows NT" },
		{ QSysInfo::WV_2000,        "Windows 2000" },
		{ QSysInfo::WV_XP,          "Windows XP" },
		{ QSysInfo::WV_2003,        "Windows 2003" },
		{ QSysInfo::WV_VISTA,       "Windows Vista" },
		{ QSysInfo::WV_WINDOWS7,    "Windows 7" },
		{ -1,                       "Windows Unknown" },
	};

	int index = 0;
	while (versions[index].ver>0 && versions[index].ver!=QSysInfo::WindowsVersion)
		index++;
	osver = versions[index].name;

#elif defined(Q_WS_MAC)

	static const struct { int ver; QString name; } versions[] = 
	{
		{ QSysInfo::MV_SNOWLEOPARD, "MacOS 10.6 (SnowLeopard)" },
		{ QSysInfo::MV_LEOPARD,     "MacOS 10.5 (Leopard)" },
		{ QSysInfo::MV_TIGER,       "MacOS 10.4 (Tiger)" },
		{ QSysInfo::MV_PANTHER,     "MacOS 10.3 (Panther)" },
		{ QSysInfo::MV_JAGUAR,      "MacOS 10.2 (Jaguar)" },
		{ QSysInfo::MV_PUMA,        "MacOS 10.1 (Puma)" },
		{ QSysInfo::MV_CHEETAH,     "MacOS 10.0(Cheetah)" },
		{ QSysInfo::MV_9,           "MacOS 9" },
		{ -1,                       "MacOS Unknown" }
	};

	int index = 0;
	while (versions[index].ver>0 && versions[index].ver!=QSysInfo::MacintoshVersion)
		index++;
	osver = versions[index].name;

#endif

	return osver;
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

	bool fullScreen = isFullScreenMode();
	if (d->fullScreenEnabled != fullScreen)
	{
		d->fullScreenEnabled = fullScreen;
		emit fullScreenModeChanged(fullScreen);
	}
}

void SystemManager::onIdleChanged(int ASeconds)
{
	d->idleSeconds = ASeconds;
	emit systemIdleChanged(ASeconds);
}
