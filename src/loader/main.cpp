#include <QLibrary>
#include <QApplication>
#include <QCleanlooksStyle>
#include "pluginmanager.h"
#include "proxystyle.h"

#ifdef Q_WS_WIN32
# include <thirdparty/holdemutils/RHoldemModule.h>
# define VIRTUS_GUID "{9732304B-B640-4C54-B2CD-3C2297D649A1}"
#endif

int main(int argc, char *argv[])
{
	QApplication app(argc, argv);

	// styles
	QApplication::setStyle(new QCleanlooksStyle);
	QApplication::setStyle(new ProxyStyle);

	app.setQuitOnLastWindowClosed(false);

	// utils
	app.addLibraryPath(app.applicationDirPath());
	QLibrary utils(app.applicationDirPath()+"/utils",&app);
	utils.load();

	// plugin manager
	PluginManager pm(&app);

#ifdef Q_WS_WIN32
	GUID guid = (GUID)QUuid(VIRTUS_GUID);
	holdem_utils::RHoldemModule *holdem_module = new holdem_utils::RHoldemModule(guid);
	QObject::connect(holdem_module, SIGNAL(shutdownRequested()), &pm, SLOT(shutdownRequested()));
#endif

	pm.restart();

	return app.exec();
}
