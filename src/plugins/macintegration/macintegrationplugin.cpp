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
#include <interfaces/imessagewidgets.h>
#include <interfaces/imainwindow.h>
#include <utils/custombordercontainer.h>
#include <utils/macwidgets.h>
#include <utils/imagemanager.h>
#include <definitions/optionnodes.h>

#include <QDebug>

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
    Q_UNUSED(APluginManager)
    Q_UNUSED(AInitOrder)

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
    }
    return true;
}

bool MacIntegrationPlugin::initObjects()
{
    // menus
    initMenus();

    // dock
    _dockMenu = new Menu();

    qt_mac_set_dock_menu(_dockMenu); // setting dock menu

    connect(MacIntegrationPrivate::instance(), SIGNAL(dockClicked()), SIGNAL(dockClicked()));
    connect(MacIntegrationPrivate::instance(), SIGNAL(growlNotifyClicked(int)), SIGNAL(growlNotifyClicked(int)));
    connect(qApp, SIGNAL(focusChanged(QWidget*,QWidget*)), SLOT(onFocusChanged(QWidget*,QWidget*)));
    //postGrowlNotify(QApplication::style()->standardPixmap(QStyle::SP_MessageBoxCritical).toImage(), "Done!", "Growl notifications work ok.", "Error", 1);

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

void MacIntegrationPlugin::postGrowlNotify(const QImage & icon, const QString & title, const QString & text, const QString & type, int id)
{
    MacIntegrationPrivate::postGrowlNotify(icon, title, text, type, id);
}

void MacIntegrationPlugin::showGrowlPreferencePane()
{
    MacIntegrationPrivate::showGrowlPrefPane();
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
    newGroupAction->setText(tr("New Gorup..."));
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
    _fileMenu->addAction(closeTabAction, 501);

    closeAllTabsAction = new Action;
    closeAllTabsAction->setText(tr("Close All Tabs"));
    closeAllTabsAction->setShortcut(QKeySequence("Shift+Ctrl+W"));
    closeAllTabsAction->setEnabled(false);
    connect(closeAllTabsAction, SIGNAL(triggered()), SLOT(onCloseAllTabsAction()));
    _fileMenu->addAction(closeAllTabsAction, 501);

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

    // Status
    _statusMenu = new Menu;
    _statusMenu->setTitle(tr("Status"));
    _menuBar->addMenu(_statusMenu);

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

void MacIntegrationPlugin::onAboutToQuit()
{

}

void MacIntegrationPlugin::onFocusChanged(QWidget * old, QWidget * now)
{
    Q_UNUSED(old)
    if (lastFocusedWidget)
        lastFocusedWidget->disconnect(this);
    lastFocusedWidget = now;
    if (now)
    {
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
            //connect(pte, SIGNAL(copyAvailable(bool)), SLOT(onCopyAvailableChange(bool)));

        }
        else if (QTextEdit * te = qobject_cast<QTextEdit*>(now))
        {
            connect(te, SIGNAL(selectionChanged()), SLOT(onSelectionChanged()));
            connect(te, SIGNAL(textChanged()), SLOT(onTextChanged()));
            //connect(te, SIGNAL(copyAvailable(bool)), SLOT(onCopyAvailableChange(bool)));
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

void MacIntegrationPlugin::onMetaTabPageDeactivated()
{
//    qDebug() << "tab page deactivated!";
//    ITabPage * page = qobject_cast<ITabPage*>(sender());
//    if (page)
//    {
//        Action * a = activeChatsActions.value(page);
//        if (a)
//            a->setChecked(false);
//    }
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
    qDebug() << "New Grop Action: not implemented!";
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
    // TODO: activate tabwindow and open tab with metacontact
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

Q_EXPORT_PLUGIN2(plg_macintegration, MacIntegrationPlugin)

