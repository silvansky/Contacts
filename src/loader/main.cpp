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
	QApplication app(argc, argv);
	app.setQuitOnLastWindowClosed(false);

	// styles
	QApplication::setStyle(new QCleanlooksStyle);
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
