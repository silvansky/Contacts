#include <QLibrary>
#include <QApplication>
#include <QCleanlooksStyle>
#include <QScopedPointer>
#include "pluginmanager.h"
#include "proxystyle.h"

#ifdef Q_WS_WIN32
# include <thirdparty/holdemutils/RHoldemModule.h>
# define RAMBLERFRIENDS_GUID "{9732304B-B640-4C54-B2CD-3C2297D649A1}"
#endif

int main(int argc, char *argv[])
{

	// styles
	QApplication::setStyle(new QCleanlooksStyle); // damn, no effect at all!
	// but we can simulate "-style cleanlooks" arguments and pass them to app's ctor...
	// that would be a really dirty hack!
	// but it solves selection problem on windows vista/seven...
	// here the code:
/*
	// totally ignoring all args
	char **newArgv = new char*[3];
	// copying 0 arg
	newArgv[0] = new char[strlen(argv[0])];
	// adding our fake args
	newArgv[1] = "-style";
	newArgv[2] = "cleanlooks";
	// replace original argc and argv and passing them to app's ctor
	argc = 3;
	argv = newArgv;
	// I think leak of these few bytes isn't so important...
	// but we can put this at the end of main() instead of return app.exec():
	int ret = app.exec();
	for (int i = 0; i < argc; i++)
		delete argv[i];
	delete argv;
	return ret;
	// remark: we can set windows style explicitly to override vista/seven selection
	// cleanlooks style brings ugly combo popups...
*/
	QApplication app(argc, argv);
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

#ifdef Q_WS_WIN32
	GUID guid = (GUID)QUuid(RAMBLERFRIENDS_GUID);
	QScopedPointer<holdem_utils::RHoldemModule> holdem_module(new holdem_utils::RHoldemModule(guid));
	QObject::connect(holdem_module.data(), SIGNAL(shutdownRequested()), &pm, SLOT(shutdownRequested()));
#endif

	pm.restart();

	return app.exec();
}
