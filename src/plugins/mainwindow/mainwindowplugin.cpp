#include "mainwindowplugin.h"

#include <QKeyEvent>
#include <QApplication>
#include <QDesktopWidget>
#include <definitions/resources.h>
#include <definitions/customborder.h>
#ifdef Q_WS_MAC
# include <utils/macwidgets.h>
#endif

MainWindowPlugin::MainWindowPlugin()
{
	FPluginManager = NULL;
	FOptionsManager = NULL;
	FTrayManager = NULL;
#ifdef Q_WS_WIN
	FMainWindow = new MainWindow(NULL, Qt::Window|Qt::CustomizeWindowHint|Qt::WindowTitleHint|Qt::WindowCloseButtonHint);
#elif defined(Q_WS_MAC)
	FMainWindow = new MainWindow(NULL, Qt::Window);
	setWindowGrowButtonEnabled(FMainWindow, false);
#else
	FMainWindow = new MainWindow(NULL, Qt::Window|Qt::CustomizeWindowHint|Qt::WindowTitleHint|Qt::WindowCloseButtonHint);
#endif
	FMainWindow->setObjectName("mainWindow");
	FMainWindowBorder = NULL;

	FOpenAction = NULL;
	FActivationChanged = QTime::currentTime();

}

MainWindowPlugin::~MainWindowPlugin()
{
	mainWindowTopWidget()->deleteLater();
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

	connect(FPluginManager->instance(),SIGNAL(shutdownStarted()),SLOT(onShutdownStarted()));

	return true;
}

bool MainWindowPlugin::initObjects()
{
	Action *action = new Action(this);
	action->setText(tr("Quit"));
	action->setData(Action::DR_SortString,QString("900"));
	connect(action,SIGNAL(triggered()),FPluginManager->instance(),SLOT(quit()));
	FMainWindow->mainMenu()->addAction(action,AG_MMENU_MAINWINDOW_QUIT,true);
	FMainWindow->mainMenu()->setTitle(tr("Menu"));

	FOpenAction = new Action(this);
	FOpenAction->setVisible(false);
	FOpenAction->setText(tr("Show Contacts"));
	connect(FOpenAction,SIGNAL(triggered(bool)),SLOT(onShowMainWindowByAction(bool)));

	if (FTrayManager)
		FTrayManager->contextMenu()->addAction(FOpenAction,AG_TMTM_MAINWINDOW_SHOW,true);

//#ifdef Q_OS_MAC
//	connect(MacDockHandler::instance(), SIGNAL(dockIconClicked()), SLOT(onDockIconClicked()));
//#endif

	return true;
}

bool MainWindowPlugin::initSettings()
{
	const QSize defSize(300, 550);
	QDesktopWidget *desktop = QApplication::desktop();
	QRect ps = desktop->availableGeometry(desktop->primaryScreen());
	QRect defRect = QStyle::alignedRect(Qt::LeftToRight, Qt::AlignRight | Qt::AlignTop, defSize, ps);

	Options::setDefaultValue(OPV_MAINWINDOW_SHOW,true);
	Options::setDefaultValue(OPV_MAINWINDOW_SIZE, defSize);
	Options::setDefaultValue(OPV_MAINWINDOW_POSITION, defRect.topLeft());
	Options::setDefaultValue(OPV_MAINWINDOW_STAYONTOP,false);
	Options::setDefaultValue(OPV_MAINWINDOW_MINIMIZETOTRAY_W7,false);

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
		widgets.insertMulti(OWO_ROSTER_MAINWINDOW_STAYONTOP, FOptionsManager->optionsNodeWidget(Options::node(OPV_MAINWINDOW_STAYONTOP),tr("Stay on top of other windows"),AParent));
#ifdef Q_OS_WIN
		if (QSysInfo::windowsVersion() == QSysInfo::WV_WINDOWS7)
			widgets.insertMulti(OWO_ROSTER_MAINWINDOW_MINIMIZETOTRAY, FOptionsManager->optionsNodeWidget(Options::node(OPV_MAINWINDOW_MINIMIZETOTRAY_W7),tr("Show application icon in tray"),AParent));
#endif
	}
	return widgets;
}

IMainWindow *MainWindowPlugin::mainWindow() const
{
	return FMainWindow;
}

QWidget *MainWindowPlugin::mainWindowTopWidget() const
{
	return FMainWindow->window();
}

bool MainWindowPlugin::isMinimizeToTray() const
{
#ifdef Q_WS_WIN
	return !(QSysInfo::windowsVersion()==QSysInfo::WV_WINDOWS7) || Options::node(OPV_MAINWINDOW_MINIMIZETOTRAY_W7).value().toBool();
#elif defined(Q_WS_X11)
	return QString(getenv("XDG_CURRENT_DESKTOP"))!="Unity" && QString(getenv("DESKTOP_SESSION"))!="gnome";
#endif
	return true;
}

void MainWindowPlugin::showMainWindow() const
{
	if (!Options::isNull())
	{
		correctWindowPosition();
		WidgetManager::showActivateRaiseWindow(mainWindowTopWidget());
	}
}

void MainWindowPlugin::closeMainWindow() const
{
	if (isMinimizeToTray())
	{
		mainWindowTopWidget()->close();
	}
	else if (mainWindowTopWidget()->isVisible())
	{
		if (FMainWindowBorder)
			FMainWindowBorder->minimizeWidget();
		else
			FMainWindow->showMinimized();
	}
}

void MainWindowPlugin::updateTitle()
{
	FMainWindow->setWindowTitle(tr("Contacts"));
}

void MainWindowPlugin::correctWindowPosition() const
{
	QRect windowRect = FMainWindowBorder ? FMainWindowBorder->geometry() : FMainWindow->geometry();
	if (FMainWindowBorder)
	{
		// correcting rect
		windowRect.setLeft(windowRect.left() - FMainWindowBorder->leftBorderWidth());
		windowRect.setRight(windowRect.right() + FMainWindowBorder->rightBorderWidth());
		windowRect.setTop(windowRect.top() - FMainWindowBorder->topBorderWidth());
		windowRect.setBottom(windowRect.bottom() + FMainWindowBorder->bottomBorderWidth());
	}

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
		if (FMainWindowBorder)
		{
			// correcting rect back
			windowRect.setLeft(windowRect.left() + FMainWindowBorder->leftBorderWidth());
			windowRect.setRight(windowRect.right() - FMainWindowBorder->rightBorderWidth());
			windowRect.setTop(windowRect.top() + FMainWindowBorder->topBorderWidth());
			windowRect.setBottom(windowRect.bottom() - FMainWindowBorder->bottomBorderWidth());
		}
		mainWindowTopWidget()->move(windowRect.topLeft());
	}
}

bool MainWindowPlugin::eventFilter(QObject *AWatched, QEvent *AEvent)
{
	if (AWatched == mainWindowTopWidget())
	{
		if (AEvent->type() == QEvent::ActivationChange)
		{
			FActivationChanged = QTime::currentTime();
		}
		else if (AEvent->type() == QEvent::KeyPress)
		{
			QKeyEvent *keyEvent = static_cast<QKeyEvent *>(AEvent);
			if (keyEvent && keyEvent->key()==Qt::Key_Escape)
				closeMainWindow();
		}
	}
	return QObject::eventFilter(AWatched,AEvent);
}

void MainWindowPlugin::onOptionsOpened()
{
	FMainWindowBorder = CustomBorderStorage::staticStorage(RSR_STORAGE_CUSTOMBORDER)->addBorder(FMainWindow, CBS_ROSTER);
	if (FMainWindowBorder)
	{
		FMainWindowBorder->setMaximizeButtonVisible(false);
		FMainWindowBorder->setMinimizeButtonVisible(false);
		FMainWindowBorder->setDockingEnabled(true);
		FMainWindowBorder->installEventFilter(this);
		connect(FMainWindowBorder, SIGNAL(closed()), SLOT(onMainWindowClosed()));
	}
	else
	{
		FMainWindow->installEventFilter(this);
		connect(FMainWindow, SIGNAL(closed()),SLOT(onMainWindowClosed()));
	}

	mainWindowTopWidget()->resize(Options::node(OPV_MAINWINDOW_SIZE).value().toSize());
	mainWindowTopWidget()->move(Options::node(OPV_MAINWINDOW_POSITION).value().toPoint());
	FOpenAction->setVisible(true);
	updateTitle();

	onOptionsChanged(Options::node(OPV_MAINWINDOW_STAYONTOP));
	onOptionsChanged(Options::node(OPV_MAINWINDOW_MINIMIZETOTRAY_W7));
}

void MainWindowPlugin::onOptionsClosed()
{
	Options::node(OPV_MAINWINDOW_SIZE).setValue(mainWindowTopWidget()->size());
	Options::node(OPV_MAINWINDOW_POSITION).setValue(mainWindowTopWidget()->pos());
	FOpenAction->setVisible(false);
	closeMainWindow();
	updateTitle();
}

void MainWindowPlugin::onOptionsChanged(const OptionsNode &ANode)
{
	QWidget *widget = mainWindowTopWidget();
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
	else if (ANode.path() == OPV_MAINWINDOW_MINIMIZETOTRAY_W7)
	{
		if (FMainWindowBorder)
		{
			FMainWindowBorder->setMinimizeOnClose(!isMinimizeToTray());
			FMainWindowBorder->setShowInTaskBar(!isMinimizeToTray());
		}
		if (!isMinimizeToTray() && !mainWindowTopWidget()->isVisible())
		{
			if (FMainWindowBorder)
				FMainWindowBorder->minimizeWidget();
			else
				FMainWindow->showMinimized();
		}
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
		if (FMainWindow->isActive() || qAbs(FActivationChanged.msecsTo(QTime::currentTime()))<qApp->doubleClickInterval())
			closeMainWindow();
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
	if (!isMinimizeToTray())
		FPluginManager->quit();
}

void MainWindowPlugin::onShutdownStarted()
{
	if (!Options::isNull())
	{
		Options::node(OPV_MAINWINDOW_SHOW).setValue(isMinimizeToTray() ? mainWindowTopWidget()->isVisible() : !mainWindowTopWidget()->isMinimized());
	}
}

Q_EXPORT_PLUGIN2(plg_mainwindow, MainWindowPlugin)
