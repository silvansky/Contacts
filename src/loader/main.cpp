#include <signal.h>

#include <QUrl>
#include <QLibrary>
#include <QApplication>
#include <QScopedPointer>
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
		QMap<QString,QString> params;
		params.insert(ARP_REPORTTYPE,"error");
		params.insert(ARP_REPORTDESCRIPTION,QString("Segmentation fault with code %1").arg(ASigNum));
		Log::sendReport(Log::generateReport(params));
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

#ifdef Q_WS_WIN
	// WARNING! DIRTY HACK!
	// totally ignoring all args and simulating "-style windows" args
	// don't know why only this works...

	char **newArgv = new char*[3];
	// copying 0 arg
	newArgv[0] = new char[strlen(argv[0])];
	// adding our fake args
	newArgv[1] = "-style";

	newArgv[2] = "windows";
	// replace original argc and argv and passing them to app's ctor
	argc = 3;
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
	QPalette pal = QApplication::palette();
	pal.setColor(QPalette::Link,QColor(Qt::white));
	QApplication::setPalette(pal);

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
