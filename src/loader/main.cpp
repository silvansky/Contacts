#include <signal.h>

#include <QUrl>
#include <QUuid>
#include <QLibrary>
#include <QSettings>
#include <QApplication>
#include <QScopedPointer>
#include <definitions/commandline.h>
#include <definitions/applicationreportparams.h>
#include <utils/log.h>
#include <utils/networking.h>
#include "pluginmanager.h"
#include "proxystyle.h"
#include "singleapp.h"

#ifdef Q_WS_WIN32
# include <thirdparty/holdemutils/RHoldemModule.h>
#endif

void generateSegfaultReport(int ASigNum)
{
	static bool fault = false;
	if (!fault)
	{
		fault = true;
		ReportError("SEGFAULT",QString("Segmentation fault with code %1").arg(ASigNum));
	}
	signal(ASigNum, SIG_DFL);
	exit(ASigNum);
}

int main(int argc, char *argv[])
{
#ifndef DEBUG_ENABLED // Позволяем отладчику обрабатывать эти ошибки
	foreach(int sig, QList<int>() << SIGSEGV << SIGILL << SIGFPE << SIGTERM << SIGABRT)
		signal(sig, generateSegfaultReport);
#endif

	// Генерируем уникальный идентификатор системы
	QSettings settings(QSettings::NativeFormat,QSettings::UserScope,"Rambler");
	QUuid systemUuid = settings.value("system/uuid").toString();
	if (systemUuid.isNull())
	{
		systemUuid = QUuid::createUuid();
		settings.setValue("system/uuid",systemUuid.toString());
	}
	Log::setStaticReportParam(ARP_SYSTEM_UUID,systemUuid.toString());

	//Ищем наличие ключа -checkinstall
	for (int i=1; i<argc; i++)
	{
		if (!strcmp(argv[i],CLO_CHECK_INSTALL))
			return 0;
	}

#ifdef Q_WS_WIN
	// WARNING! DIRTY HACK!
	// adding "-style windows" args
	// don't know why only this works...
	bool changeStyle = true;
	char **newArgv = new char*[argc+2];
	for (int i=0; i<argc; i++)
	{
		int argLen = strlen(argv[i])+1;
		newArgv[i] = new char[argLen];
		memcpy(newArgv[i],argv[i],argLen);
		changeStyle = changeStyle && strcmp(argv[i],"-style")!=0;
	}
	newArgv[argc] = changeStyle ? "-style" : "";
	newArgv[argc+1] = changeStyle ? "windows" : "";

	argc = argc+2;
	argv = newArgv;
#endif

	SingleApp app(argc, argv, "Rambler.Contacts");

#ifndef DEBUG_ENABLED
	if (app.isRunning())
	{
		app.sendMessage("show");
		return 0;
	}
#endif

	app.setQuitOnLastWindowClosed(false);

	QApplication::setStyle(new ProxyStyle);

	// fixing menu/combo/etc problems - disabling all animate/fade effects
	QApplication::setEffectEnabled(Qt::UI_AnimateMenu, false);
	QApplication::setEffectEnabled(Qt::UI_AnimateCombo, false);
	QApplication::setEffectEnabled(Qt::UI_AnimateTooltip, false);
	QApplication::setEffectEnabled(Qt::UI_AnimateToolBox, false);
	QApplication::setEffectEnabled(Qt::UI_FadeMenu, false);
	QApplication::setEffectEnabled(Qt::UI_FadeTooltip, false);

	// This should be done in Style Sheet
//	QPalette pal = QApplication::palette();
//	pal.setColor(QPalette::Link,QColor(Qt::white));
//	QApplication::setPalette(pal);

	// utils
	app.addLibraryPath(app.applicationDirPath());
	QLibrary utils(app.applicationDirPath()+"/utils",&app);
	utils.load();

	// plugin manager
	PluginManager pm(&app);

	QObject::connect(&app, SIGNAL(messageAvailable(const QString&)), &pm, SLOT(showMainWindow()));

#ifdef Q_WS_WIN
	GUID guid = (GUID)QUuid(CLIENT_GUID);
	QScopedPointer<holdem_utils::RHoldemModule> holdem_module(new holdem_utils::RHoldemModule(guid));
	QObject::connect(holdem_module.data(), SIGNAL(shutdownRequested()), &pm, SLOT(shutdownRequested()));
#endif

	// Starting plugin manager
	pm.restart();

	int ret = app.exec();

#ifdef Q_WS_WIN
	for (int i = 0; i < argc; i++)
		delete argv[i];
	delete argv;
#endif

	return ret;
}
