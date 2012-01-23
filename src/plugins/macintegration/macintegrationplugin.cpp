#include "macintegrationplugin.h"

#include "macintegration_p.h"

#include <QApplication>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QTextDocumentFragment>
#include <QTextEdit>
#include <QWebView>
#include <QClipboard>
#include <QStyle>
#include <QDesktopServices>
#include <interfaces/imessagewidgets.h>
#include <interfaces/imainwindow.h>
#include <utils/custombordercontainer.h>
#include <utils/macwidgets.h>
#include <utils/imagemanager.h>
#include <utils/options.h>
#include <utils/custominputdialog.h>
#include <definitions/optionnodes.h>
#include <definitions/optionvalues.h>

#ifdef DEBUG_ENABLED
# include <QDebug>
#endif

extern void qt_mac_set_dock_menu(QMenu *); // Qt internal function

static ITabWindow * findTabWindow(QObject * parent)
{
	ITabWindow * tw = qobject_cast<ITabWindow*>(parent);
	if (parent && !tw)
	{
		foreach (QObject * child, parent->children())
		{
			if ((tw = qobject_cast<ITabWindow*>(child)))
			{
				break;
			}
			else
			{
				tw = findTabWindow(child);
				if (tw)
					break;
			}
		}
	}
	return tw;
}

static IMainWindow * findMainWindow(QObject * parent)
{
	IMainWindow * mw = qobject_cast<IMainWindow*>(parent);
	if (parent && !mw)
	{
		foreach (QObject * child, parent->children())
		{
			if ((mw = qobject_cast<IMainWindow*>(child)))
			{
				break;
			}
			else
			{
				mw = findMainWindow(child);
				if (mw)
					break;
			}
		}
	}
	return mw;
}


MacIntegrationPlugin::MacIntegrationPlugin()
{
	_dockMenu = _fileMenu = _editMenu = _viewMenu = _statusMenu = _windowMenu = _helpMenu = NULL;
	_menuBar = NULL;
	lastFocusedWidget = NULL;
	// dummi translations
	tr("New Message");
	tr("New E-Mail");
	tr("Mood Changed");
	tr("Status Changed");
	tr("Birthday Reminder");
	tr("Error");
	tr("Subscription Message");
	// custom window frame
	// TODO: read these colors from style
	MacIntegrationPrivate::installCustomFrame();
	setCustomBorderColor(QColor(65, 70, 77, 255).lighter());
	setCustomTitleColor(QColor(240, 240, 240, 255));
	//setCustomTitleColor(QColor(0, 0, 0, 255));
}

MacIntegrationPlugin::~MacIntegrationPlugin()
{
	delete _dockMenu;
	delete _menuBar;
}

void MacIntegrationPlugin::pluginInfo(IPluginInfo *APluginInfo)
{
	APluginInfo->name = tr("Mac OS X Integration");
	APluginInfo->description = tr("Allow integration in Mac OS X Dock and Menu Bar");
	APluginInfo->version = "1.0";
	APluginInfo->author = "Valentine Gorshkov aka Silvansky";
	APluginInfo->homePage = "http://contacts.rambler.ru";
}

bool MacIntegrationPlugin::initConnections(IPluginManager *APluginManager, int &AInitOrder)
{
	Q_UNUSED(AInitOrder)

	// init menus and dock
	initMenus();
	initDock();

	pluginManager = APluginManager;

	if (APluginManager)
	{
		connect(APluginManager->instance(),SIGNAL(aboutToQuit()),SLOT(onAboutToQuit()));

		IPlugin *plugin = APluginManager->pluginInterface("IAccountManager").value(0,NULL);
		if (plugin)
		{
			accountManager = qobject_cast<IAccountManager *>(plugin->instance());
		}

		plugin = APluginManager->pluginInterface("IRosterChanger").value(0,NULL);
		if (plugin)
		{
			rosterChanger = qobject_cast<IRosterChanger *>(plugin->instance());
		}

		plugin = APluginManager->pluginInterface("IOptionsManager").value(0,NULL);
		if (plugin)
		{
			optionsManager = qobject_cast<IOptionsManager *>(plugin->instance());
			if (optionsManager)
			{
				connect(optionsManager->instance(), SIGNAL(profileOpened(const QString &)), SLOT(onProfileOpened(const QString &)));
				connect(optionsManager->instance(), SIGNAL(profileClosed(const QString &)), SLOT(onProfileClosed(const QString &)));
			}
		}

		plugin = APluginManager->pluginInterface("IRosterSearch").value(0,NULL);
		if (plugin)
		{
			rosterSearch = qobject_cast<IRosterSearch *>(plugin->instance());
		}

		plugin = APluginManager->pluginInterface("IMainWindowPlugin").value(0,NULL);
		if (plugin)
		{
			mainWindow = qobject_cast<IMainWindowPlugin *>(plugin->instance());
		}

		plugin = APluginManager->pluginInterface("IMetaContacts").value(0,NULL);
		if (plugin)
		{
			metaContacts = qobject_cast<IMetaContacts *>(plugin->instance());
			if (metaContacts)
			{
				connect(metaContacts->instance(), SIGNAL(metaTabWindowCreated(IMetaTabWindow *)), SLOT(onMetaTabWindowCreated(IMetaTabWindow *)));
			}
		}

		plugin = APluginManager->pluginInterface("IStatusChanger").value(0,NULL);
		if (plugin)
		{
			statusChanger = qobject_cast<IStatusChanger *>(plugin->instance());
			if (statusChanger)
			{
				connect(statusChanger->instance(), SIGNAL(statusChanged(const Jid&, int)), SLOT(onStatusChanged(const Jid&,int)));
				connect(statusChanger->instance(), SIGNAL(statusItemAdded(int)), SLOT(onStatusItemAdded(int)));
				connect(statusChanger->instance(), SIGNAL(statusItemChanged(int)), SLOT(onStatusItemChanged(int)));
				connect(statusChanger->instance(), SIGNAL(statusItemRemoved(int)), SLOT(onStatusItemRemoved(int)));
			}
		}

		plugin = APluginManager->pluginInterface("IEmoticons").value(0,NULL);
		if (plugin)
		{
			emoticons = qobject_cast<IEmoticons *>(plugin->instance());
		}

		plugin = APluginManager->pluginInterface("IMessageWidgets").value(0,NULL);
		if (plugin)
		{
			messageWidgets = qobject_cast<IMessageWidgets *>(plugin->instance());
		}
	}

	connect(Options::instance(), SIGNAL(optionsChanged(const OptionsNode&)), SLOT(onOptionsChanged(const OptionsNode&)));
	return true;
}

bool MacIntegrationPlugin::initObjects()
{
	return true;
}

bool MacIntegrationPlugin::initSettings()
{
	return true;
}

Menu * MacIntegrationPlugin::dockMenu()
{
	return _dockMenu;
}

QMenuBar * MacIntegrationPlugin::menuBar()
{
	return _menuBar;
}

Menu * MacIntegrationPlugin::fileMenu()
{
	return _fileMenu;
}

Menu * MacIntegrationPlugin::editMenu()
{
	return _editMenu;
}

Menu * MacIntegrationPlugin::viewMenu()
{
	return _viewMenu;
}

Menu * MacIntegrationPlugin::statusMenu()
{
	return _statusMenu;
}

Menu * MacIntegrationPlugin::windowMenu()
{
	return _windowMenu;
}

Menu * MacIntegrationPlugin::helpMenu()
{
	return _helpMenu;
}

void MacIntegrationPlugin::setDockBadge(const QString & badgeText)
{
	MacIntegrationPrivate::setDockBadge(badgeText);
}

void MacIntegrationPlugin::setDockOverlayImage(const QImage & image, Qt::Alignment alignment, bool showAppIcon)
{
	MacIntegrationPrivate::setDockOverlay(image, alignment, showAppIcon);
}

void MacIntegrationPlugin::requestUserAttention()
{
	MacIntegrationPrivate::requestAttention();
}

void MacIntegrationPlugin::postGrowlNotify(const QImage & icon, const QString & title, const QString & text, const QString & type, int id)
{
	QString croppedText = text;
	if (croppedText.length() > 140)
		croppedText = croppedText.left(140) + "...";
	MacIntegrationPrivate::postGrowlNotify(icon, title, croppedText, type, id);
}

void MacIntegrationPlugin::showGrowlPreferencePane()
{
	if (MacIntegrationPrivate::isGrowlInstalled())
		MacIntegrationPrivate::showGrowlPrefPane();
	else
	{
		CustomInputDialog * info = new CustomInputDialog(CustomInputDialog::Info);
		info->setAttribute(Qt::WA_DeleteOnClose, true);
		info->setCaptionText(tr("Growl application not found"));
		info->setInfoText(tr("Growl is required to show notifications about new messages. Please, install it from <a href=\"http://growl.info/documentation/growl-package-install.php\">official website</a>."));
		info->setAcceptButtonText(tr("OK"));
		connect(info, SIGNAL(linkActivated(const QString &)), SLOT(onLinkActivated(const QString &)));
		info->open();
	}
}

void MacIntegrationPlugin::setCustomBorderColor(const QColor & color)
{
	MacIntegrationPrivate::setCustomBorderColor(color);
}

void MacIntegrationPlugin::setCustomTitleColor(const QColor & color)
{
	MacIntegrationPrivate::setCustomTitleColor(color);
}

void MacIntegrationPlugin::setWindowMovableByBackground(QWidget * window, bool movable)
{
	MacIntegrationPrivate::setWindowMovableByBackground(window, movable);
}

void MacIntegrationPlugin::initMenus()
{
	_menuBar = new QMenuBar(NULL); // this must be the first parentless QMenuBar

	// File
	_fileMenu = new Menu();
	_fileMenu->setTitle(tr("File"));
	_menuBar->addMenu(_fileMenu);

	newContactAction = new Action;
	newContactAction->setText(tr("New Contact..."));
	newContactAction->setShortcut(QKeySequence("Ctrl+N"));
	newContactAction->setEnabled(false);
	connect(newContactAction, SIGNAL(triggered()), SLOT(onNewContactAction()));
	_fileMenu->addAction(newContactAction);

	newGroupAction = new Action;
	newGroupAction->setText(tr("New Group..."));
	newGroupAction->setEnabled(false);
	connect(newGroupAction, SIGNAL(triggered()), SLOT(onNewGroupAction()));
	_fileMenu->addAction(newGroupAction);

	newAccountAction = new Action;
	newAccountAction->setText(tr("New Account..."));
	newAccountAction->setEnabled(false);
	connect(newAccountAction, SIGNAL(triggered()), SLOT(onNewAccountAction()));
	_fileMenu->addAction(newAccountAction);

	closeTabAction = new Action;
	closeTabAction->setText(tr("Close Tab"));
	closeTabAction->setShortcut(QKeySequence("Ctrl+W"));
	closeTabAction->setEnabled(false);
	connect(closeTabAction, SIGNAL(triggered()), SLOT(onCloseTabAction()));
	_fileMenu->addAction(closeTabAction, 600);

	closeAllTabsAction = new Action;
	closeAllTabsAction->setText(tr("Close All Tabs"));
	closeAllTabsAction->setShortcut(QKeySequence("Shift+Ctrl+W"));
	closeAllTabsAction->setEnabled(false);
	connect(closeAllTabsAction, SIGNAL(triggered()), SLOT(onCloseAllTabsAction()));
	_fileMenu->addAction(closeAllTabsAction, 600);

	// app menu
	checkForUpdatesAction = new Action;
	checkForUpdatesAction->setMenuRole(QAction::ApplicationSpecificRole);
	checkForUpdatesAction->setText(tr("Check for Updates..."));
	connect(checkForUpdatesAction, SIGNAL(triggered()), SLOT(onCheckForUpdatesAction()));
	_fileMenu->addAction(checkForUpdatesAction);

	// Edit
	_editMenu = new Menu;
	_editMenu->setTitle(tr("Edit"));
	_menuBar->addMenu(_editMenu);

	copyAction = new Action;
	copyAction->setText(tr("Copy"));
	copyAction->setShortcut(QKeySequence("Ctrl+C"));
	connect(copyAction, SIGNAL(triggered()), SLOT(onCopyAction()));
	_editMenu->addAction(copyAction);

	cutAction = new Action;
	cutAction->setText(tr("Cut"));
	cutAction->setShortcut(QKeySequence("Ctrl+X"));
	connect(cutAction, SIGNAL(triggered()), SLOT(onCutAction()));
	_editMenu->addAction(cutAction);

	pasteAction = new Action;
	pasteAction->setText(tr("Paste"));
	pasteAction->setShortcut(QKeySequence("Ctrl+V"));
	connect(pasteAction, SIGNAL(triggered()), SLOT(onPasteAction()));
	_editMenu->addAction(pasteAction);

	undoAction = new Action;
	undoAction->setText(tr("Undo"));
	undoAction->setShortcut(QKeySequence("Ctrl+Z"));
	connect(undoAction, SIGNAL(triggered()), SLOT(onUndoAction()));
	_editMenu->addAction(undoAction, 400);

	redoAction = new Action;
	redoAction->setText(tr("Redo"));
	redoAction->setShortcut(QKeySequence("Shift+Ctrl+Z"));
	connect(redoAction, SIGNAL(triggered()), SLOT(onRedoAction()));
	_editMenu->addAction(redoAction, 400);

	selectallAction = new Action;
	selectallAction->setText(tr("Select All"));
	selectallAction->setShortcut(QKeySequence("Ctrl+A"));
	connect(selectallAction, SIGNAL(triggered()), SLOT(onSelectAllAction()));
	_editMenu->addAction(selectallAction);

	findAction = new Action;
	findAction->setText(tr("Find"));
	findAction->setShortcut(QKeySequence("Ctrl+F"));
	findAction->setEnabled(false);
	connect(findAction, SIGNAL(triggered()), SLOT(onFindAction()));
	_editMenu->addAction(findAction, 600);

	// View
	_viewMenu = new Menu;
	_viewMenu->setTitle(tr("View"));
	_menuBar->addMenu(_viewMenu);

	fullViewAction = new Action;
	fullViewAction->setText(tr("Full"));
	fullViewAction->setCheckable(true);
	fullViewAction->setEnabled(false);
	connect(fullViewAction, SIGNAL(triggered(bool)), SLOT(onFullViewAction(bool)));
	_viewMenu->addAction(fullViewAction);

	simpleViewAction = new Action;
	simpleViewAction->setText(tr("Simplifyed"));
	simpleViewAction->setCheckable(true);
	simpleViewAction->setEnabled(false);
	connect(simpleViewAction, SIGNAL(triggered(bool)), SLOT(onSimpleViewAction(bool)));
	_viewMenu->addAction(simpleViewAction);

	compactViewAction = new Action;
	compactViewAction->setText(tr("Compact"));
	compactViewAction->setCheckable(true);
	compactViewAction->setEnabled(false);
	connect(compactViewAction, SIGNAL(triggered(bool)), SLOT(onCompactViewAction(bool)));
	_viewMenu->addAction(compactViewAction);

	sortByNameAction = new Action;
	sortByNameAction->setText(tr("Sort By Name"));
	sortByNameAction->setCheckable(true);
	sortByNameAction->setEnabled(false);
	connect(sortByNameAction, SIGNAL(triggered(bool)), SLOT(onSortByNameAction(bool)));
	_viewMenu->addAction(sortByNameAction, 600);

	sortByStatusAction = new Action;
	sortByStatusAction->setText(tr("Sort By Status"));
	sortByStatusAction->setCheckable(true);
	sortByStatusAction->setEnabled(false);
	connect(sortByStatusAction, SIGNAL(triggered(bool)), SLOT(onSortByStatusAction(bool)));
	_viewMenu->addAction(sortByStatusAction, 600);

	showOfflineAction = new Action;
	showOfflineAction->setText(tr("Show Offline"));
	showOfflineAction->setCheckable(true);
	showOfflineAction->setEnabled(false);
	connect(showOfflineAction, SIGNAL(triggered(bool)), SLOT(onShowOfflineAction(bool)));
	_viewMenu->addAction(showOfflineAction, 700);

	stayOnTopAction = new Action;
	stayOnTopAction->setText(tr("Stay On Top"));
	stayOnTopAction->setCheckable(true);
	stayOnTopAction->setEnabled(false);
	connect(stayOnTopAction, SIGNAL(triggered(bool)), SLOT(onStayOnTopAction(bool)));
	_viewMenu->addAction(stayOnTopAction, 800);

	// Status
	_statusMenu = new Menu;
	_statusMenu->setTitle(tr("Status"));
	_menuBar->addMenu(_statusMenu);

	manageAccountsAction = new Action;
	manageAccountsAction->setText(tr("Manage Accounts..."));
	manageAccountsAction->setEnabled(false);
	connect(manageAccountsAction, SIGNAL(triggered()), SLOT(onNewAccountAction()));
	//_statusMenu->addAction(manageAccountsAction, 550);

	autoStatusAction = new Action;
	autoStatusAction->setText(tr("Change Status to \"Away\" on Idle"));
	autoStatusAction->setCheckable(true);
	autoStatusAction->setEnabled(false);
	connect(autoStatusAction, SIGNAL(toggled(bool)), SLOT(onAutoStatusAction(bool)));
	_statusMenu->addAction(autoStatusAction, 200);

	// Window
	_windowMenu = new Menu;
	_windowMenu->setTitle(tr("Window"));
	_menuBar->addMenu(_windowMenu);

	minimizeAction = new Action();
	minimizeAction->setText(tr("Minimize to Dock"));
	minimizeAction->setShortcut(QKeySequence("Ctrl+M"));
	connect(minimizeAction, SIGNAL(triggered()), SLOT(onMinimizeAction()));
	_windowMenu->addAction(minimizeAction);

	zoomAction = new Action();
	zoomAction->setText(tr("Zoom"));
	connect(zoomAction, SIGNAL(triggered()), SLOT(onZoomAction()));
	_windowMenu->addAction(zoomAction);

	closeAction = new Action;
	closeAction->setText(tr("Close"));
	closeAction->setShortcut(QKeySequence("Ctrl+W"));
	connect(closeAction, SIGNAL(triggered()), SLOT(onCloseAction()));
	_windowMenu->addAction(closeAction);

	prevTabAction = new Action;
	prevTabAction->setText(tr("Select Previous Tab"));
	prevTabAction->setShortcut(QKeySequence("Meta+Shift+Tab"));
	connect(prevTabAction, SIGNAL(triggered()), SLOT(onPrevTabAction()));
	_windowMenu->addAction(prevTabAction, 550);

	nextTabAction = new Action;
	nextTabAction->setText(tr("Select Next Tab"));
	nextTabAction->setShortcut(QKeySequence("Meta+Tab"));
	connect(nextTabAction, SIGNAL(triggered()), SLOT(onNextTabAction()));
	_windowMenu->addAction(nextTabAction, 550);

	bringAllToTopAction = new Action;
	bringAllToTopAction->setText(tr("Bring All to Front"));
	connect(bringAllToTopAction, SIGNAL(triggered()), SLOT(onBringAllToTopAction()));
	_windowMenu->addAction(bringAllToTopAction, 600);

	Action * showMainWindowAction = new Action;
	showMainWindowAction->setText(tr("Contact List"));
	showMainWindowAction->setShortcut(QKeySequence("Ctrl+/"));
	connect(showMainWindowAction, SIGNAL(triggered()), SLOT(onShowMainWindowAction()));
	_windowMenu->addAction(showMainWindowAction, 650);

	chatsAction = new Action;
	chatsAction->setText(tr("Chats"));
	chatsAction->setEnabled(false);
	chatsAction->setVisible(false);
	_windowMenu->addAction(chatsAction, 650);

	// Help
	_helpMenu = new Menu;
	_helpMenu->setTitle(tr("Help"));
	_menuBar->addMenu(_helpMenu);

	onlineHelpAction =  new Action;
	onlineHelpAction->setText(tr("Online Help"));
	connect(onlineHelpAction, SIGNAL(triggered()), SLOT(onOnlineHelpAction()));
	_helpMenu->addAction(onlineHelpAction);

	feedbackAction = new Action;
	feedbackAction->setText(tr("Leave Feedback..."));
	feedbackAction->setEnabled(false);
	connect(feedbackAction, SIGNAL(triggered()), SLOT(onFeedbackAction()));
	_helpMenu->addAction(feedbackAction, 600);

	facebookAction = new Action;
	facebookAction->setText(tr("Facebook Group"));
	connect(facebookAction, SIGNAL(triggered()), SLOT(onFacebookAction()));
	_helpMenu->addAction(facebookAction, 600);

	rulesAction = new Action;
	rulesAction->setText(tr("Terms of Usage And Confidentiality"));
	connect(rulesAction, SIGNAL(triggered()), SLOT(onRulesAction()));
	_helpMenu->addAction(rulesAction, 600);

	connect(_helpMenu, SIGNAL(aboutToShow()), SLOT(onHelpMenuAboutToShow()));

	funAction = new Action;
	connect(funAction, SIGNAL(triggered()), SLOT(onFunAction()));
	_helpMenu->addAction(funAction, 900);

	funLinks.insert("http://eda.ru/", tr("What to cook for dinner?"));
	funLinks.insert("http://avia.rambler.ru/anytime?f=MOW&t=TR&min_days=7&max_days=14&one_way=&week_days=&stops=&airlines=", tr("When is it cheaper to fly to Turkey?"));
	funLinks.insert("http://www.afisha.ru/#eventsbyfriends", tr("Where my friends are going to?"));
	funLinks.insert("http://weather.rambler.ru", tr("What will the weather be tomorrow?"));
	funLinks.insert("http://finance.rambler.ru/currency/world/", tr("How much is one buck?"));
	funLinks.insert("http://audio.rambler.ru/radios/echo", tr("What do they talk about at Echo of Moscow?"));
	funLinks.insert("http://maps.rambler.ru/?ZinGs2o", tr("Where Lahdenpohja is?"));
	// TODO: replace google shortener with rambler's one
	funLinks.insert("http://goo.gl/TMwtu", tr("Medical certificate for swimming pool"));
}

void MacIntegrationPlugin::initDock()
{
	_dockMenu = new Menu();

	qt_mac_set_dock_menu(_dockMenu); // setting dock menu

	dockShowMainWindowAction = new Action;
	dockShowMainWindowAction->setText(tr("Contact List"));
	connect(dockShowMainWindowAction, SIGNAL(triggered()), SLOT(onShowMainWindowDockAction()));
	_dockMenu->addAction(dockShowMainWindowAction);
	foreach(QAction * a, _dockMenu->actions())
		if (a->isSeparator())
		{
			qobject_cast<QMenu*>(_dockMenu)->removeAction(a);
			a->deleteLater();
		}

	//	dockChatsAction = new Action;
	//	dockChatsAction->setText(tr("Chats"));
	//	dockChatsAction->setEnabled(false);
	//	dockChatsAction->setVisible(false);
	//	_dockMenu->addAction(dockChatsAction);

	connect(_dockMenu, SIGNAL(aboutToShow()), SLOT(onDockMenuAboutToShow()));

	connect(MacIntegrationPrivate::instance(), SIGNAL(dockClicked()), SIGNAL(dockClicked()));
	connect(MacIntegrationPrivate::instance(), SIGNAL(growlNotifyClicked(int)), SIGNAL(growlNotifyClicked(int)));
	connect(qApp, SIGNAL(focusChanged(QWidget*,QWidget*)), SLOT(onFocusChanged(QWidget*,QWidget*)));
}

void MacIntegrationPlugin::updateActions()
{
	if (accountManager && rosterChanger)
	{
		bool newContactEnabled;
		IAccount * acc = accountManager->accounts().isEmpty() ? NULL : accountManager->accounts().first();
		newContactEnabled = (acc && acc->isActive());
		newContactAction->setEnabled(newContactEnabled);
		//newGroupAction->setEnabled(newContactEnabled);
		newAccountAction->setEnabled(newContactEnabled);
	}
	if (lastFocusedWidget)
	{
		bool copyEnabled = false;
		bool cutEnabled = false;
		bool pasteEnabled = false;
		bool undoEnabled = false;
		bool redoEnabled = false;
		bool selectallEnabled = false;
		if (QLineEdit * le = qobject_cast<QLineEdit*>(lastFocusedWidget))
		{
			copyEnabled = !le->selectedText().isEmpty();
			cutEnabled = copyEnabled && !le->isReadOnly();
			pasteEnabled = !le->isReadOnly();
			undoEnabled = le->isUndoAvailable();
			redoEnabled = le->isRedoAvailable();
			selectallEnabled = !le->text().isEmpty();
		}
		else if (QPlainTextEdit * pte = qobject_cast<QPlainTextEdit*>(lastFocusedWidget))
		{
			copyEnabled = !pte->textCursor().selection().isEmpty();
			cutEnabled = copyEnabled && !pte->isReadOnly();
			pasteEnabled = !pte->isReadOnly();
			undoEnabled = redoEnabled = pte->isUndoRedoEnabled();
			selectallEnabled = !pte->toPlainText().isEmpty();
		}
		else if (QTextEdit * te = qobject_cast<QTextEdit*>(lastFocusedWidget))
		{
			copyEnabled = !te->textCursor().selection().isEmpty();
			cutEnabled = copyEnabled && !te->isReadOnly();
			pasteEnabled = !te->isReadOnly();
			undoEnabled = redoEnabled = te->isUndoRedoEnabled();
			selectallEnabled = !(te->toPlainText().isEmpty() && te->toHtml().isEmpty());
		}
		else if (QWebView * wv = qobject_cast<QWebView*>(lastFocusedWidget))
		{
			copyEnabled = wv->page()->action(QWebPage::Copy)->isEnabled();
			cutEnabled = wv->page()->action(QWebPage::Cut)->isEnabled();
			pasteEnabled = wv->page()->action(QWebPage::Paste)->isEnabled();
			undoEnabled = wv->page()->action(QWebPage::Undo)->isEnabled();
			redoEnabled = wv->page()->action(QWebPage::Redo)->isEnabled();
			selectallEnabled = wv->page()->action(QWebPage::SelectAll)->isEnabled();
		}
		copyAction->setEnabled(copyEnabled);
		cutAction->setEnabled(cutEnabled);
		pasteAction->setEnabled(pasteEnabled);
		undoAction->setEnabled(undoEnabled);
		redoAction->setEnabled(undoEnabled);
		selectallAction->setEnabled(selectallEnabled);
	}
}

void MacIntegrationPlugin::updateContactActions()
{

}

void MacIntegrationPlugin::onLinkActivated(const QString &url)
{
	QDesktopServices::openUrl(QUrl(url));
	if (CustomInputDialog * dialog = qobject_cast<CustomInputDialog*>(sender()))
	{
		dialog->accept();
	}
}

void MacIntegrationPlugin::onAboutToQuit()
{
	MacIntegrationPrivate::release();
}

void MacIntegrationPlugin::onOptionsChanged(const OptionsNode &ANode)
{
	if (ANode.path() == OPV_AUTOSTARTUS_AWAYONLOCK)
	{
		autoStatusAction->setChecked(ANode.value().toBool());
	}
	else if (ANode.path() == OPV_ROSTER_SHOWOFFLINE)
	{
		showOfflineAction->setChecked(ANode.value().toBool());
	}
	else if (ANode.path() == OPV_ROSTER_SORTBYNAME)
	{
		sortByNameAction->setChecked(ANode.value().toBool());
		sortByStatusAction->setChecked(!ANode.value().toBool());
	}
	else if (ANode.path() == OPV_ROSTER_SORTBYSTATUS)
	{
		sortByNameAction->setChecked(!ANode.value().toBool());
		sortByStatusAction->setChecked(ANode.value().toBool());
	}
	else if ((ANode.path() == OPV_AVATARS_SHOW) || (ANode.path() == OPV_ROSTER_SHOWSTATUSTEXT))
	{
		bool showAvatars = Options::node(OPV_AVATARS_SHOW).value().toBool();
		bool showStatus = Options::node(OPV_ROSTER_SHOWSTATUSTEXT).value().toBool();
		if (showAvatars && showStatus)
		{
			// full view
			fullViewAction->setChecked(true);
			simpleViewAction->setChecked(false);
			compactViewAction->setChecked(false);
		}
		else if (showAvatars)
		{
			// simplifyed view
			fullViewAction->setChecked(false);
			simpleViewAction->setChecked(true);
			compactViewAction->setChecked(false);
		}
		else
		{
			// compact view
			fullViewAction->setChecked(false);
			simpleViewAction->setChecked(false);
			compactViewAction->setChecked(true);
		}
	}
	else if (ANode.path() == OPV_MAINWINDOW_STAYONTOP)
	{
		stayOnTopAction->setChecked(ANode.value().toBool());
	}
}

void MacIntegrationPlugin::onFocusChanged(QWidget * old, QWidget * now)
{
	Q_UNUSED(old)
	if (lastFocusedWidget)
		lastFocusedWidget->disconnect(this);
	lastFocusedWidget = now;
	if (now)
	{
#ifdef DEBUG_ENABLED
		qDebug() << "focused: " << now->objectName()
			 << " of class " << now->metaObject()->className();
		QStringList hierarchy;
		QWidget * parent = now->parentWidget();
		while (parent)
		{
			hierarchy << QString("%1 (%2)").arg(parent->objectName(), parent->metaObject()->className());
			parent = parent->parentWidget();
		}
		qDebug() << "hierarchy: " << hierarchy.join(" -> ");
#endif
		updateActions();
		if (QLineEdit * le = qobject_cast<QLineEdit*>(now))
		{
			connect(le, SIGNAL(selectionChanged()), SLOT(onSelectionChanged()));
			connect(le, SIGNAL(textChanged(QString)), SLOT(onTextChanged()));
		}
		else if (QPlainTextEdit * pte = qobject_cast<QPlainTextEdit*>(now))
		{
			connect(pte, SIGNAL(selectionChanged()), SLOT(onSelectionChanged()));
			connect(pte, SIGNAL(textChanged()), SLOT(onTextChanged()));

		}
		else if (QTextEdit * te = qobject_cast<QTextEdit*>(now))
		{
			connect(te, SIGNAL(selectionChanged()), SLOT(onSelectionChanged()));
			connect(te, SIGNAL(textChanged()), SLOT(onTextChanged()));
		}
		else if (QWebView * wv = qobject_cast<QWebView*>(now))
		{
			connect(wv->page(), SIGNAL(selectionChanged()), SLOT(onSelectionChanged()));
			connect(wv->page(), SIGNAL(contentsChanged()), SLOT(onTextChanged()));
		}
	}
	if (qApp->activeWindow())
	{
		ITabWindow * tw = findTabWindow(qApp->activeWindow());
		closeAction->setEnabled(!tw);
		closeTabAction->setEnabled(tw);
		closeAllTabsAction->setEnabled(tw);
		nextTabAction->setEnabled(tw);
		prevTabAction->setEnabled(tw);
		IMainWindow * mw = findMainWindow(qApp->activeWindow());
		findAction->setEnabled(mw);
		minimizeAction->setEnabled(true);
		zoomAction->setEnabled(isWindowGrowButtonEnabled(qApp->activeWindow()));
	}
	else
	{
		closeAction->setEnabled(false);
		closeTabAction->setEnabled(false);
		closeAllTabsAction->setEnabled(false);
		nextTabAction->setEnabled(false);
		prevTabAction->setEnabled(false);
		minimizeAction->setEnabled(false);
		zoomAction->setEnabled(false);
	}
}

void MacIntegrationPlugin::onProfileOpened(const QString & name)
{
	Q_UNUSED(name)
	foreach(Action* a, availableStatuses.values())
		a->setEnabled(true);
	manageAccountsAction->setEnabled(true);
	autoStatusAction->setEnabled(true);
	newContactAction->setEnabled(true);
	//newGroupAction->setEnabled(true);
	newAccountAction->setEnabled(true);

	fullViewAction->setEnabled(true);
	simpleViewAction->setEnabled(true);
	compactViewAction->setEnabled(true);
	sortByStatusAction->setEnabled(true);
	sortByNameAction->setEnabled(true);
	showOfflineAction->setEnabled(true);
	stayOnTopAction->setEnabled(true);

	feedbackAction->setEnabled(true);

	onOptionsChanged(Options::node(OPV_ROSTER_SORTBYSTATUS));
	onOptionsChanged(Options::node(OPV_ROSTER_SORTBYNAME));
	onOptionsChanged(Options::node(OPV_ROSTER_SHOWOFFLINE));
	onOptionsChanged(Options::node(OPV_ROSTER_SHOWSTATUSTEXT));
	onOptionsChanged(Options::node(OPV_AVATARS_SHOW));
	onOptionsChanged(Options::node(OPV_AUTOSTARTUS_AWAYONLOCK));
	onOptionsChanged(Options::node(OPV_MAINWINDOW_STAYONTOP));
}

void MacIntegrationPlugin::onProfileClosed(const QString & name)
{
	Q_UNUSED(name)
	foreach(Action* a, availableStatuses.values())
		a->setEnabled(false);
	manageAccountsAction->setEnabled(false);
	autoStatusAction->setEnabled(false);
	newContactAction->setEnabled(false);
	//newGroupAction->setEnabled(false);
	newAccountAction->setEnabled(false);

	fullViewAction->setEnabled(false);
	simpleViewAction->setEnabled(false);
	compactViewAction->setEnabled(false);
	sortByStatusAction->setEnabled(false);
	sortByNameAction->setEnabled(false);
	showOfflineAction->setEnabled(false);
	stayOnTopAction->setEnabled(false);

	feedbackAction->setEnabled(false);
}

void MacIntegrationPlugin::onMetaTabWindowCreated(IMetaTabWindow *AWindow)
{
	if (AWindow)
	{
		IAccount * acc = accountManager->accounts().isEmpty() ? NULL : accountManager->accounts().first();
		if (acc && acc->isActive())
		{
			IMetaRoster * mr = metaContacts->findMetaRoster(acc->xmppStream()->streamJid());
			if (mr)
			{
				QImage avatarImage = mr->metaAvatarImage(AWindow->metaId(), false);
				Action * contactAction = new Action;
				QIcon contactIcon;
				contactIcon.addPixmap(QPixmap::fromImage(avatarImage.scaled(16, 16)));
				contactAction->setIcon(contactIcon);
				QString contactName = metaContacts->metaContactName(mr->metaContact(AWindow->metaId()));
				if (contactName.length() > 31)
					contactName = contactName.left(31) + "...";
				contactAction->setText(contactName);
				contactAction->setCheckable(true);
				connect(contactAction, SIGNAL(triggered()), SLOT(onContactAction()));
				activeChatsActions.insert(AWindow, contactAction);
				if (!chatsAction->isVisible())
					chatsAction->setVisible(true);

				connect(AWindow->instance(), SIGNAL(tabPageClosed()), SLOT(onMetaTabPageClosed()));
				connect(AWindow->instance(), SIGNAL(tabPageActivated()), SLOT(onMetaTabPageActivated()));

				_windowMenu->addAction(contactAction, 650);
			}
		}
	}
}

void MacIntegrationPlugin::onMetaTabPageClosed()
{
	IMetaTabWindow * window = qobject_cast<IMetaTabWindow*>(sender());
	if (window)
	{
		Action * a = activeChatsActions.take(window);
		if (a)
		{
			_windowMenu->removeAction(a);
			a->deleteLater();
		}
		if (!activeChatsActions.count())
			chatsAction->setVisible(false);
	}
}

void MacIntegrationPlugin::onMetaTabPageActivated()
{
	ITabPage * page = qobject_cast<ITabPage*>(sender());
	if (page)
	{
		Action * a = activeChatsActions.value(page);
		if (a)
		{
			foreach(Action * action, activeChatsActions.values())
				action->setChecked(false);
			a->setChecked(true);
		}
	}
}

void MacIntegrationPlugin::onStatusChanged(const Jid &AStreamJid, int AStatusId)
{
	Q_UNUSED(AStreamJid)
	Action * a = availableStatuses.value(AStatusId);
	if (a)
	{
		foreach(Action * action, availableStatuses.values())
		{
			action->setChecked(false);
		}
		a->setChecked(true);
	}
}

void MacIntegrationPlugin::onStatusItemAdded(int status)
{
	if (statusChanger)
	{
		Action * statusAction = new Action;
		statusAction->setCheckable(true);
		availableStatuses.insert(status, statusAction);
		connect(statusAction, SIGNAL(triggered()), SLOT(onStatusAction()));
		onStatusItemChanged(status);
		_statusMenu->addAction(statusAction, 100);

		if (optionsManager)
			statusAction->setEnabled(!optionsManager->currentProfile().isNull());
	}
}

void MacIntegrationPlugin::onStatusItemChanged(int status)
{
	Action * statusAction = availableStatuses.value(status);
	if (statusAction)
	{
		statusAction->setText(statusChanger->statusItemName(status));
		int show = statusChanger->statusItemShow(status);
		statusAction->setIcon(statusChanger->iconByShow(show));
	}
}

void MacIntegrationPlugin::onStatusItemRemoved(int status)
{
	Action * statusAction = availableStatuses.value(status);
	if (statusAction)
	{
		_statusMenu->removeAction(statusAction);
		availableStatuses.take(status);
		statusAction->deleteLater();
	}
}

void MacIntegrationPlugin::onCheckForUpdatesAction()
{
	MacIntegrationPrivate::checkForUpdates();
}

void MacIntegrationPlugin::onNewContactAction()
{
	if (accountManager && rosterChanger)
	{
		IAccount * acc = accountManager->accounts().isEmpty() ? NULL : accountManager->accounts().first();
		if (acc && acc->isActive())
		{
			rosterChanger->showAddContactDialog(acc->xmppStream()->streamJid());
		}
	}
}

void MacIntegrationPlugin::onNewGroupAction()
{
	// TODO
#ifdef DEBUG_ENABLED
	qDebug() << "New Grop Action: not implemented!";
#endif
}

void MacIntegrationPlugin::onNewAccountAction()
{
	if (optionsManager)
	{
		optionsManager->showOptionsDialog(OPN_GATEWAYS);
	}
}

void MacIntegrationPlugin::onCloseTabAction()
{
	ITabWindow * tw = findTabWindow(qApp->activeWindow());
	if (tw)
		tw->closeCurrentTab();
}

void MacIntegrationPlugin::onCloseAllTabsAction()
{
	ITabWindow * tw = findTabWindow(qApp->activeWindow());
	if (tw)
		tw->closeAllTabs();
}

void MacIntegrationPlugin::onFullViewAction(bool on)
{
	Q_UNUSED(on)
	fullViewAction->setChecked(false);
	Options::node(OPV_AVATARS_SHOW).setValue(true);
	Options::node(OPV_ROSTER_SHOWSTATUSTEXT).setValue(true);
}

void MacIntegrationPlugin::onSimpleViewAction(bool on)
{
	Q_UNUSED(on)
	simpleViewAction->setChecked(false);
	Options::node(OPV_AVATARS_SHOW).setValue(true);
	Options::node(OPV_ROSTER_SHOWSTATUSTEXT).setValue(false);
}

void MacIntegrationPlugin::onCompactViewAction(bool on)
{
	Q_UNUSED(on)
	compactViewAction->setChecked(false);
	Options::node(OPV_AVATARS_SHOW).setValue(false);
	Options::node(OPV_ROSTER_SHOWSTATUSTEXT).setValue(false);
}

void MacIntegrationPlugin::onSortByStatusAction(bool on)
{
	Q_UNUSED(on)
	Options::node(OPV_ROSTER_SORTBYSTATUS).setValue(true);
	Options::node(OPV_ROSTER_SORTBYNAME).setValue(false);
}

void MacIntegrationPlugin::onSortByNameAction(bool on)
{
	Q_UNUSED(on)
	Options::node(OPV_ROSTER_SORTBYNAME).setValue(true);
	Options::node(OPV_ROSTER_SORTBYSTATUS).setValue(false);
}

void MacIntegrationPlugin::onShowOfflineAction(bool on)
{
	Options::node(OPV_ROSTER_SHOWOFFLINE).setValue(on);
}

void MacIntegrationPlugin::onStayOnTopAction(bool on)
{
	Options::node(OPV_MAINWINDOW_STAYONTOP).setValue(on);
}

void MacIntegrationPlugin::onStatusAction()
{
	Action * a = qobject_cast<Action*>(sender());
	if (a && statusChanger && accountManager)
	{
		a->setChecked(false);
		IAccount * acc = accountManager->accounts().isEmpty() ? NULL : accountManager->accounts().first();
		if (acc && acc->isActive())
		{
			int statusId = availableStatuses.key(a);
			statusChanger->setStreamStatus(acc->xmppStream()->streamJid(), statusId);
		}
	}
}

void MacIntegrationPlugin::onManageAccountsAction()
{
	onNewAccountAction();
}

void MacIntegrationPlugin::onAutoStatusAction(bool on)
{
	Options::node(OPV_AUTOSTARTUS_AWAYONLOCK).setValue(on);
}

void MacIntegrationPlugin::onMinimizeAction()
{
	QWidget * activeWindow = QApplication::activeWindow();
	if (activeWindow)
		activeWindow->showMinimized();
}

void MacIntegrationPlugin::onZoomAction()
{
	QWidget * activeWindow = QApplication::activeWindow();
	if (activeWindow && isWindowGrowButtonEnabled(activeWindow))
	{
		if (activeWindow->isMaximized())
			activeWindow->showNormal();
		else
			activeWindow->showMaximized();
	}
}

void MacIntegrationPlugin::onCloseAction()
{
	ITabWindow * tw = findTabWindow(qApp->activeWindow());
	if (tw)
	{
		//tw->currentTabPage()->closeTabPage();
		// nothing to do, see onCloseTabAction()
	}
	else
	{
		QWidget * activeWindow = QApplication::activeWindow();
		if (CustomBorderContainer* border = qobject_cast<CustomBorderContainer*>(activeWindow))
		{
			border->closeWidget();
		}
		else if (activeWindow)
			activeWindow->close();
	}
}

void MacIntegrationPlugin::onNextTabAction()
{
	ITabWindow * tw = findTabWindow(qApp->activeWindow());
	if (tw)
		tw->nextTab();
}

void MacIntegrationPlugin::onPrevTabAction()
{
	ITabWindow * tw = findTabWindow(qApp->activeWindow());
	if (tw)
		tw->previousTab();
}

void MacIntegrationPlugin::onBringAllToTopAction()
{
	QWidget * oldFocusedWidget = lastFocusedWidget;
	QWidget * oldActiveWindow = qApp->activeWindow();
	foreach (QWidget * w, qApp->topLevelWidgets())
	{
		if (!w->isHidden())
			w->raise();
	}
	if (oldActiveWindow)
		oldActiveWindow->activateWindow();
	if (oldFocusedWidget)
		oldFocusedWidget->setFocus();
}

void MacIntegrationPlugin::onShowMainWindowAction()
{
	if (mainWindow)
		mainWindow->showMainWindow();
}

void MacIntegrationPlugin::onContactAction()
{
	Action * a = qobject_cast<Action*>(sender());
	if (a)
	{
		ITabPage * page = activeChatsActions.key(a);
		if (page)
		{
			a->setChecked(true);
			page->showTabPage();
		}
	}
}

void MacIntegrationPlugin::onOnlineHelpAction()
{
	QDesktopServices::openUrl(QUrl("http://contacts.rambler.ru/faq.html"));
}

void MacIntegrationPlugin::onFeedbackAction()
{
	if (pluginManager)
	{
		pluginManager->showFeedbackDialog();
	}
}

void MacIntegrationPlugin::onFacebookAction()
{
	QDesktopServices::openUrl(QUrl("http://www.facebook.com/groups/185080491548525/"));
}

void MacIntegrationPlugin::onRulesAction()
{
	QDesktopServices::openUrl(QUrl("http://help.rambler.ru/legal/?s=101705"));
}

void MacIntegrationPlugin::onFunAction()
{
	QDesktopServices::openUrl(QUrl(currentFunLink));
}

void MacIntegrationPlugin::onHelpMenuAboutToShow()
{
	if (funLinks.keys().count())
	{
		int index = qrand() % funLinks.keys().count();
		currentFunLink = funLinks.keys().at(index);
		QString funText = funLinks.value(currentFunLink);
		funAction->setText(funText);
		if (!funAction->isEnabled())
			funAction->setEnabled(true);
	}
	else
		funAction->setEnabled(false);
}

// copy/cut/paste/undo/redo/selectall are handled by widgets, these slots do nothing for now
void MacIntegrationPlugin::onCopyAction()
{
	if (QWidget * w = QApplication::focusWidget())
	{
		if (QLineEdit * le = qobject_cast<QLineEdit*>(w))
			le->copy();
		else if (QPlainTextEdit * pte = qobject_cast<QPlainTextEdit*>(w))
			pte->copy();
		else if (QTextEdit * te = qobject_cast<QTextEdit*>(w))
			te->copy();
		else if (QWebView * wv = qobject_cast<QWebView*>(w))
			wv->page()->triggerAction(QWebPage::Copy);
	}
}

void MacIntegrationPlugin::onPasteAction()
{
	if (QWidget * w = QApplication::focusWidget())
	{
		if (QLineEdit * le = qobject_cast<QLineEdit*>(w))
			le->paste();
		else if (QPlainTextEdit * pte = qobject_cast<QPlainTextEdit*>(w))
			pte->paste();
		else if (QTextEdit * te = qobject_cast<QTextEdit*>(w))
			te->paste();
		else if (QWebView * wv = qobject_cast<QWebView*>(w))
			wv->page()->triggerAction(QWebPage::Paste);
	}
}

void MacIntegrationPlugin::onCutAction()
{
	if (QWidget * w = QApplication::focusWidget())
	{
		if (QLineEdit * le = qobject_cast<QLineEdit*>(w))
			le->cut();
		else if (QPlainTextEdit * pte = qobject_cast<QPlainTextEdit*>(w))
			pte->cut();
		else if (QTextEdit * te = qobject_cast<QTextEdit*>(w))
			te->cut();
		else if (QWebView * wv = qobject_cast<QWebView*>(w))
			wv->page()->triggerAction(QWebPage::Cut);
	}
}

void MacIntegrationPlugin::onUndoAction()
{
	if (QWidget * w = QApplication::focusWidget())
	{
		if (QLineEdit * le = qobject_cast<QLineEdit*>(w))
			le->undo();
		else if (QPlainTextEdit * pte = qobject_cast<QPlainTextEdit*>(w))
			pte->undo();
		else if (QTextEdit * te = qobject_cast<QTextEdit*>(w))
			te->undo();
		else if (QWebView * wv = qobject_cast<QWebView*>(w))
			wv->page()->triggerAction(QWebPage::Undo);
	}
}

void MacIntegrationPlugin::onRedoAction()
{
	if (QWidget * w = QApplication::focusWidget())
	{
		if (QLineEdit * le = qobject_cast<QLineEdit*>(w))
			le->redo();
		else if (QPlainTextEdit * pte = qobject_cast<QPlainTextEdit*>(w))
			pte->redo();
		else if (QTextEdit * te = qobject_cast<QTextEdit*>(w))
			te->redo();
		else if (QWebView * wv = qobject_cast<QWebView*>(w))
			wv->page()->triggerAction(QWebPage::Redo);
	}
}

void MacIntegrationPlugin::onSelectAllAction()
{
	if (QWidget * w = QApplication::focusWidget())
	{
		if (QLineEdit * le = qobject_cast<QLineEdit*>(w))
			le->selectAll();
		else if (QPlainTextEdit * pte = qobject_cast<QPlainTextEdit*>(w))
			pte->selectAll();
		else if (QTextEdit * te = qobject_cast<QTextEdit*>(w))
			te->selectAll();
		else if (QWebView * wv = qobject_cast<QWebView*>(w))
			wv->page()->triggerAction(QWebPage::SelectAll);
	}
}

void MacIntegrationPlugin::onFindAction()
{
	if (rosterSearch)
		rosterSearch->startSearch();
}

void MacIntegrationPlugin::onSelectionChanged()
{
	updateActions();
}

void MacIntegrationPlugin::onTextChanged()
{
	updateActions();
}

void MacIntegrationPlugin::onCopyAvailableChange(bool available)
{
	copyAction->setEnabled(available);
}

void MacIntegrationPlugin::onShowMainWindowDockAction()
{
	if (mainWindow)
		mainWindow->showMainWindow();
}

void MacIntegrationPlugin::onRecentContactAction()
{

}

void MacIntegrationPlugin::onDockMenuAboutToShow()
{
	foreach (Action * a, recentContactsActions)
	{
		_dockMenu->removeAction(a);
		a->deleteLater();
	}
	if (messageWidgets)
		recentContactsActions = messageWidgets->createLastTabPagesActions(NULL);
	foreach (Action * action, recentContactsActions)
	{
		_dockMenu->addAction(action);
	}
}

Q_EXPORT_PLUGIN2(plg_macintegration, MacIntegrationPlugin)

