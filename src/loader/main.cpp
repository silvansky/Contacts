#include <QLibrary>
#include <QApplication>
#include <QCleanlooksStyle>
#include "pluginmanager.h"
#include "proxystyle.h"

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
	pm.restart();

	return app.exec();
}
