#include "mainwindowplugin.h"

#include <QApplication>
#include <QDesktopWidget>
#include <definitions/resources.h>
#include <definitions/customborder.h>

#ifdef Q_OS_MAC
# include <utils/macdockhandler.h>
#endif

MainWindowPlugin::MainWindowPlugin()
{
	FPluginManager = NULL;
	FOptionsManager = NULL;
	FTrayManager = NULL;

	FOpenAction = NULL;
	FActivationChanged = QTime::currentTime();
#ifdef Q_WS_WIN
	FMainWindow = new MainWindow(NULL, Qt::Window|Qt::CustomizeWindowHint|Qt::WindowTitleHint|Qt::WindowCloseButtonHint);
#else
	FMainWindow = new MainWindow(NULL, Qt::Window|Qt::CustomizeWindowHint|Qt::WindowTitleHint|Qt::WindowCloseButtonHint);
#endif
	FMainWindow->setObjectName("mainWindow");
	FMainWindowBorder = CustomBorderStorage::staticStorage(RSR_STORAGE_CUSTOMBORDER)->addBorder(FMainWindow, CBS_ROSTER);

	if (FMainWindowBorder)
	{
		FMainWindowBorder->setMaximizeButtonVisible(false);
		FMainWindowBorder->setMinimizeButtonVisible(false);
		FMainWindowBorder->setDockingEnabled(true);
#ifdef Q_WS_WIN
		if (QSysInfo::windowsVersion() == QSysInfo::WV_WINDOWS7)
			FMainWindowBorder->setMinimizeOnClose(true);
		else
			FMainWindowBorder->setShowInTaskBar(false);
#endif
	}

#ifdef Q_WS_WIN
		if (QSysInfo::windowsVersion() == QSysInfo::WV_WINDOWS7)
			connect(FMainWindowBorder ? (QObject*)FMainWindowBorder : (QObject*)FMainWindow, SIGNAL(closed()), SLOT(onMainWindowClosed()));
#endif


	FMainWindow->installEventFilter(this);
	//WidgetManager::setWindowSticky(FMainWindowBorder ? (QWidget*)FMainWindowBorder : (QWidget*)FMainWindow, true);
}

MainWindowPlugin::~MainWindowPlugin()
{
	if (FMainWindowBorder)
		FMainWindowBorder->deleteLater();
	else
		FMainWindow->deleteLater();
}

void MainWindowPlugin::pluginInfo(IPluginInfo *APluginInfo)
{
	APluginInfo->name = tr("Main Window");
	APluginInfo->description = tr("Allows other modules to place their widgets in the main window");
	APluginInfo->version = "1.0";
	APluginInfo->author = "Potapov S.A. aka Lion";
	APluginInfo->homePage = "http://contacts.rambler.ru";
}

bool MainWindowPlugin::initConnections(IPluginManager *APluginManager, int &AInitOrder)
{
	Q_UNUSED(AInitOrder);
	FPluginManager = APluginManager;

	IPlugin *plugin = FPluginManager->pluginInterface("IOptionsManager").value(0,NULL);
	if (plugin)
	{
		FOptionsManager = qobject_cast<IOptionsManager *>(plugin->instance());
		if (FOptionsManager)
		{
			connect(FOptionsManager->instance(), SIGNAL(profileRenamed(const QString &, const QString &)),
				SLOT(onProfileRenamed(const QString &, const QString &)));
		}
	}

	plugin = APluginManager->pluginInterface("ITrayManager").value(0,NULL);
	if (plugin)
	{
		FTrayManager = qobject_cast<ITrayManager *>(plugin->instance());
		if (FTrayManager)
		{
			connect(FTrayManager->instance(),SIGNAL(notifyActivated(int, QSystemTrayIcon::ActivationReason)),
				SLOT(onTrayNotifyActivated(int,QSystemTrayIcon::ActivationReason)));
		}
	}

	connect(Options::instance(),SIGNAL(optionsOpened()),SLOT(onOptionsOpened()));
	connect(Options::instance(),SIGNAL(optionsClosed()),SLOT(onOptionsClosed()));
	connect(Options::instance(),SIGNAL(optionsChanged(const OptionsNode &)),SLOT(onOptionsChanged(const OptionsNode &)));

	connect(FPluginManager->instance(),SIGNAL(quitStarted()),SLOT(onApplicationQuitStarted()));

	return true;
}

bool MainWindowPlugin::initObjects()
{
	Action *action = new Action(this);
	action->setText(tr("Quit"));
	action->setData(Action::DR_SortString,QString("900"));
	//action->setIcon(RSR_STORAGE_MENUICONS,MNI_MAINWINDOW_QUIT);
	connect(action,SIGNAL(triggered()),FPluginManager->instance(),SLOT(quit()));
	FMainWindow->mainMenu()->addAction(action,AG_MMENU_MAINWINDOW_QUIT,true);

	FOpenAction = new Action(this);
	FOpenAction->setVisible(false);
	FOpenAction->setText(tr("Show Contacts"));
	FOpenAction->setIcon(RSR_STORAGE_MENUICONS,MNI_MAINWINDOW_SHOW_ROSTER);
	connect(FOpenAction,SIGNAL(triggered(bool)),SLOT(onShowMainWindowByAction(bool)));

	if (FTrayManager)
		FTrayManager->contextMenu()->addAction(FOpenAction,AG_TMTM_MAINWINDOW,true);

#ifdef Q_OS_MAC
	connect(MacDockHandler::instance(), SIGNAL(dockIconClicked()), SLOT(onDockIconCLicked()));
#endif
	return true;
}

bool MainWindowPlugin::initSettings()
{
	Options::setDefaultValue(OPV_MAINWINDOW_SHOW,true);
	const QSize defSize(300, 550);
	Options::setDefaultValue(OPV_MAINWINDOW_SIZE, defSize);
	QDesktopWidget * desktop = QApplication::desktop();
	QRect ps = desktop->availableGeometry(desktop->primaryScreen());
	QRect defRect = QStyle::alignedRect(Qt::LeftToRight, Qt::AlignRight | Qt::AlignTop, defSize, ps);
	Options::setDefaultValue(OPV_MAINWINDOW_POSITION, defRect.topLeft());
	Options::setDefaultValue(OPV_MAINWINDOW_STAYONTOP,false);

	if (FOptionsManager)
	{
		FOptionsManager->insertServerOption(OPV_MAINWINDOW_STAYONTOP);
		FOptionsManager->insertOptionsHolder(this);
	}

	return true;
}

bool MainWindowPlugin::startPlugin()
{
	updateTitle();
	return true;
}

QMultiMap<int, IOptionsWidget *> MainWindowPlugin::optionsWidgets(const QString &ANodeId, QWidget *AParent)
{
	QMultiMap<int, IOptionsWidget *> widgets;
	if (FOptionsManager && ANodeId == OPN_ROSTER)
	{
		widgets.insertMulti(OWO_ROSTER_MAINWINDOW, FOptionsManager->optionsNodeWidget(Options::node(OPV_MAINWINDOW_STAYONTOP),tr("Stay on top of other windows"),AParent));
	}
	return widgets;
}

IMainWindow *MainWindowPlugin::mainWindow() const
{
	return FMainWindow;
}

CustomBorderContainer * MainWindowPlugin::mainWindowBorder() const
{
	return FMainWindowBorder;
}

void MainWindowPlugin::showMainWindow() const
{
	if (!Options::isNull())
	{
		correctWindowPosition();
		WidgetManager::showActivateRaiseWindow(FMainWindowBorder ? (QWidget*)FMainWindowBorder : (QWidget*)FMainWindow);
		QApplication::processEvents();
	}
}

void MainWindowPlugin::updateTitle()
{
	FMainWindow->setWindowTitle(tr("Contacts"));
}

void MainWindowPlugin::correctWindowPosition() const
{
	QRect windowRect = FMainWindowBorder ? FMainWindowBorder->geometry() : FMainWindow->geometry();
	QRect screenRect = qApp->desktop()->availableGeometry(qApp->desktop()->screenNumber(windowRect.topLeft()));
	if (!screenRect.isEmpty() && !screenRect.adjusted(10,10,-10,-10).intersects(windowRect))
	{
		if (windowRect.right() <= screenRect.left())
			windowRect.moveLeft(screenRect.left());
		else if (windowRect.left() >= screenRect.right())
			windowRect.moveRight(screenRect.right());
		if (windowRect.top() >= screenRect.bottom())
			windowRect.moveBottom(screenRect.bottom());
		else if (windowRect.bottom() <= screenRect.top())
			windowRect.moveTop(screenRect.top());
		FMainWindowBorder ? FMainWindowBorder->move(windowRect.topLeft()) : FMainWindow->move(windowRect.topLeft());
	}
}

bool MainWindowPlugin::eventFilter(QObject *AWatched, QEvent *AEvent)
{
	if (AWatched==FMainWindow && AEvent->type()==QEvent::ActivationChange)
		FActivationChanged = QTime::currentTime();
	return QObject::eventFilter(AWatched,AEvent);
}

void MainWindowPlugin::onOptionsOpened()
{
	QWidget *widget = FMainWindowBorder ? (QWidget*)FMainWindowBorder : (QWidget*)FMainWindow;
	widget->resize(Options::node(OPV_MAINWINDOW_SIZE).value().toSize());
	widget->move(Options::node(OPV_MAINWINDOW_POSITION).value().toPoint());
	FOpenAction->setVisible(true);
	onOptionsChanged(Options::node(OPV_MAINWINDOW_STAYONTOP));
	if (Options::node(OPV_MAINWINDOW_SHOW).value().toBool())
		showMainWindow();
#ifdef Q_WS_WIN
	else if (QSysInfo::windowsVersion() == QSysInfo::WV_WINDOWS7)
		widget->showMinimized();
#endif
	updateTitle();
}

void MainWindowPlugin::onOptionsClosed()
{
	QWidget *widget = FMainWindowBorder ? (QWidget*)FMainWindowBorder : (QWidget*)FMainWindow;
	Options::node(OPV_MAINWINDOW_SIZE).setValue(widget->size());
	Options::node(OPV_MAINWINDOW_POSITION).setValue(widget->pos());
#ifdef Q_WS_WIN
	if (QSysInfo::windowsVersion() == QSysInfo::WV_WINDOWS7)
		widget->hide();
	else
#endif
		widget->close();
	FOpenAction->setVisible(false);
	updateTitle();
}

void MainWindowPlugin::onOptionsChanged(const OptionsNode &ANode)
{
	QWidget * widget = FMainWindowBorder ? (QWidget*)FMainWindowBorder : (QWidget*)FMainWindow;
	if (ANode.path() == OPV_MAINWINDOW_STAYONTOP)
	{
		bool show = widget->isVisible();
		if (ANode.value().toBool())
			widget->setWindowFlags(widget->windowFlags() | Qt::WindowStaysOnTopHint);
		else
			widget->setWindowFlags(widget->windowFlags() & ~Qt::WindowStaysOnTopHint);
		if (show)
			showMainWindow();
	}
}

void MainWindowPlugin::onProfileRenamed(const QString &AProfile, const QString &ANewName)
{
	Q_UNUSED(AProfile);
	Q_UNUSED(ANewName);
	updateTitle();
}

void MainWindowPlugin::onTrayNotifyActivated(int ANotifyId, QSystemTrayIcon::ActivationReason AReason)
{
	if (ANotifyId<0 && AReason==QSystemTrayIcon::DoubleClick)
	{
		QWidget * widget = FMainWindowBorder ? (QWidget*)FMainWindowBorder : (QWidget*)FMainWindow;
		if (FMainWindow->isActive() || qAbs(FActivationChanged.msecsTo(QTime::currentTime()))<qApp->doubleClickInterval())
		{
#ifdef Q_WS_WIN
			if (QSysInfo::windowsVersion() == QSysInfo::WV_WINDOWS7)
				widget->hide();
			else
#endif
				widget->close();
		}
		else
			showMainWindow();
	}
}

void MainWindowPlugin::onShowMainWindowByAction(bool)
{
	showMainWindow();
}

void MainWindowPlugin::onMainWindowClosed()
{
#ifdef Q_WS_WIN
	if (QSysInfo::windowsVersion() == QSysInfo::WV_WINDOWS7)
		FPluginManager->quit();
#endif
}

#ifdef Q_OS_MAC
void MainWindowPlugin::onDockIconCLicked()
{
	if (!FMainWindow->isVisible())
		showMainWindow();
}
#endif


void MainWindowPlugin::onApplicationQuitStarted()
{
	if (!Options::isNull())
	{
		QWidget *widget = FMainWindowBorder ? (QWidget*)FMainWindowBorder : (QWidget*)FMainWindow;
#ifdef Q_WS_WIN
		if (QSysInfo::windowsVersion() == QSysInfo::WV_WINDOWS7)
			Options::node(OPV_MAINWINDOW_SHOW).setValue(!widget->isMinimized());
		else
#endif
			Options::node(OPV_MAINWINDOW_SHOW).setValue(widget->isVisible());
	}
}

Q_EXPORT_PLUGIN2(plg_mainwindow, MainWindowPlugin)
