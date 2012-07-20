#include "systemmanager.h"

#include <QDir>
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
# ifndef __MINGW32__
#  include <comutil.h>
# endif
typedef BOOL (WINAPI *IW64PFP)(HANDLE, BOOL *);
#endif

#if defined(Q_WS_X11)
# include <sys/utsname.h>
#elif defined(Q_WS_MAC)
# include <Carbon/Carbon.h>
#endif

#ifdef Q_WS_X11
static QString resolveXVersion()
{
  QString osver;
	// TODO: resolve DE type (Gnome/Unity/KDE/etc...) and version
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
		process.start(found, QStringList()<<"--description" << "--short", QIODevice::ReadOnly);
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
  return osver;
}
#endif

#ifdef Q_WS_WIN
static QString windowsLanguage()
{
#ifndef __MINGW32__
	LANGID lid = GetUserDefaultUILanguage();
#else
	LANGID lid = 0x0409; // debug only! TODO: fix this function to fit mingw
#endif
	LCID lcid = MAKELCID(lid, SORT_DEFAULT);
	wchar_t * buff = new wchar_t[10];
	int size = GetLocaleInfo(lcid, LOCALE_SLANGUAGE, 0, 0);
	if (size)
	{
		buff = new wchar_t[size];
		int ret = GetLocaleInfo(lcid, LOCALE_SLANGUAGE, buff, size);
		QString res = QString::fromWCharArray(buff);
		delete buff;
		return ret ? res : QString::null;
	}
	return QString::null;
}

static QString windowsSP()
{
	OSVERSIONINFO ovi;
	ovi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);

	if (GetVersionEx(&ovi))
	{
		if (ovi.szCSDVersion == L"")
			return "no SP";
		else
			return QString::fromWCharArray(ovi.szCSDVersion);
	}
	else
		return QString::null;
}

static QString windowsBitness()
{
	IW64PFP IW64P = (IW64PFP)GetProcAddress(GetModuleHandle(L"kernel32"), "IsWow64Process");
	BOOL res = FALSE;
	if (IW64P != NULL)
	{
		IW64P(GetCurrentProcess(), &res);
	}
	return res ? "64" : "32";
}

static QString resolveWidowsVersion(QSysInfo::WinVersion ver)
{
	QString win("Windows %1 %2 %3 %4-bit");
	QString version;
	switch (ver)
	{
	case QSysInfo::WV_32s:
		version = "32s";
		break;
	case QSysInfo::WV_95:
		version = "95";
		break;
	case QSysInfo::WV_98:
		version = "98";
		break;
	case QSysInfo::WV_Me:
		version = "Me";
		break;
	case QSysInfo::WV_DOS_based:
		version = "DOS based";
		break;
	case QSysInfo::WV_NT:
		version = "NT";
		break;
	case QSysInfo::WV_2000:
		version = "2000";
		break;
	case QSysInfo::WV_XP:
		version = "XP";
		break;
	case QSysInfo::WV_2003:
		version = "2003/XP64";
		break;
	case QSysInfo::WV_VISTA:
		version = "Vista";
		break;
	case QSysInfo::WV_WINDOWS7:
		version = "Seven";
		break;
	case QSysInfo::WV_NT_based:
		version = "NT Based";
		break;
	default:
		version = "Unknown";
		break;
	}
	return win.arg(version, windowsSP(), windowsLanguage(), windowsBitness());
}
#endif

#ifdef Q_WS_MAC
static QString resolveMacVersion(QSysInfo::MacVersion ver)
{
	Q_UNUSED(ver)
	QString mac("Mac OS X %1.%2.%3 (%4)");
	QString version;
	SInt32 majVer = 0, minVer = 0, fixVer = 0;
	Gestalt(gestaltSystemVersionMajor, &majVer);
	Gestalt(gestaltSystemVersionMinor, &minVer);
	Gestalt(gestaltSystemVersionBugFix, &fixVer);
	switch(minVer)
	{
	case 3:
		version = "Panther";
		break;
	case 4:
		version = "Tiger";
		break;
	case 5:
		version = "Leopard";
		break;
	case 6:
		version = "Snow Leopard";
		break;
	case 7:
		version = "Lion";
		break;
	case 8:
		version = "Mountain Lion";
		break;
	default:
		version = "Unknown";
		break;
	}
	return mac.arg(majVer).arg(minVer).arg(fixVer).arg(version);
}
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
	// TODO: Linux and MacOSX implementations
	return false;
}

bool SystemManager::isScreenSaverRunning()
{
#ifdef Q_WS_WIN
	BOOL aRunning;
	if (SystemParametersInfo(SPI_GETSCREENSAVERRUNNING,0,&aRunning,0))
		return aRunning;
#endif
	// TODO: Linux and MacOSX implementations
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
	// TODO: Linux and MacOSX implementations
	return false;
}

QString SystemManager::systemOSVersion()
{
	QString osver;

#ifdef Q_WS_X11

	osver = resolveXVersion();

#elif defined(Q_WS_WIN) || defined(Q_OS_CYGWIN)

	osver = resolveWidowsVersion(QSysInfo::windowsVersion());

#elif defined(Q_WS_MAC)

	osver = resolveMacVersion(QSysInfo::MacintoshVersion);

#endif

	if (osver.isEmpty())
		 osver = "Unknown OS";

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
