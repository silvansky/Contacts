#include <QLibrary>
#include <QApplication>
#include <QCleanlooksStyle>
#include "pluginmanager.h"
#include "proxystyle.h"

int main(int argc, char *argv[])
{
	QApplication app(argc, argv);
	app.setQuitOnLastWindowClosed(false);
	app.addLibraryPath(app.applicationDirPath());
	QApplication::setStyle(new QCleanlooksStyle);
	QApplication::setStyle(new ProxyStyle);
	QLibrary utils(app.applicationDirPath()+"/utils",&app);
	utils.load();
	PluginManager pm(&app);
	pm.restart();
	return app.exec();
}
