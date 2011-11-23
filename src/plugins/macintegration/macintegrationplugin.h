#ifndef MACINTEGRATIONPLUGIN_H
#define MACINTEGRATIONPLUGIN_H

#include <QObject>
#include <interfaces/ipluginmanager.h>
#include <interfaces/imacintegration.h>
#include <interfaces/iaccountmanager.h>
#include <interfaces/irosterchanger.h>
#include <interfaces/ioptionsmanager.h>
#include <interfaces/irostersearch.h>
#include <interfaces/imainwindow.h>
#include <interfaces/imetacontacts.h>

#define MACINTEGRATION_UUID "{c936122e-cbf3-442d-b01f-8ecc5b0ad530}"

class MacIntegrationPrivate;
class ITabWindow;

class MacIntegrationPlugin :
        public QObject,
        public IPlugin,
        public IMacIntegration
{
    Q_OBJECT
    Q_INTERFACES(IPlugin IMacIntegration)
public:
    MacIntegrationPlugin();
    ~MacIntegrationPlugin();
    //IPlugin
    virtual QObject *instance() { return this; }
    virtual QUuid pluginUuid() const { return MACINTEGRATION_UUID; }
    virtual void pluginInfo(IPluginInfo *APluginInfo);
    virtual bool initConnections(IPluginManager *APluginManager, int &AInitOrder);
    virtual bool initObjects();
    virtual bool initSettings() { return true; }
    virtual bool startPlugin() { return true; }
    //IMacIntegration
    Menu * dockMenu();
    QMenuBar * menuBar();
    Menu * fileMenu();
    Menu * editMenu();
    Menu * viewMenu();
    Menu * statusMenu();
    Menu * windowMenu();
    Menu * helpMenu();
    void setDockBadge(const QString & badgeText);
    void postGrowlNotify(const QImage & icon, const QString & title, const QString & text, const QString & type, int id);
    void showGrowlPreferencePane();
    void setCustomBorderColor(const QColor & color);
    void setCustomTitleColor(const QColor & color);
    void setWindowMovableByBackground(QWidget * window, bool movable);
signals:
    void dockClicked();
    void growlNotifyClicked(int);
private:
    void initMenus();
    void updateActions();
    void updateContactActions();
private slots:
    void onAboutToQuit();
    void onFocusChanged(QWidget * old, QWidget * now);
    void onMetaTabWindowCreated(IMetaTabWindow *AWindow);
    void onMetaTabPageClosed();
    void onMetaTabPageActivated();
    void onMetaTabPageDeactivated();
    // file menu
    void onNewContactAction();
    void onNewGroupAction();
    void onNewAccountAction();
    void onCloseTabAction();
    void onCloseAllTabsAction();
    // window menu
    void onMinimizeAction();
    void onZoomAction();
    void onCloseAction();
    void onNextTabAction();
    void onPrevTabAction();
    void onBringAllToTopAction();
    void onShowMainWindowAction();
    void onContactAction();
    // edit menu
    void onCopyAction();
    void onPasteAction();
    void onCutAction();
    void onUndoAction();
    void onRedoAction();
    void onSelectAllAction();
    void onFindAction();
    // slots for handling selection change events
    void onSelectionChanged();
    void onTextChanged();
    void onCopyAvailableChange(bool);
private:
    QWidget * lastFocusedWidget;
    Menu * _dockMenu;
    QMenuBar * _menuBar;
    Menu * _fileMenu;
    Menu * _editMenu;
    Menu * _viewMenu;
    Menu * _statusMenu;
    Menu * _windowMenu;
    Menu * _helpMenu;
    MacIntegrationPrivate * p;
    // actions
    Action * copyAction, * cutAction, * pasteAction, * undoAction, * redoAction, * selectallAction;
    Action * closeAction, * closeTabAction, * closeAllTabsAction;
    Action * minimizeAction, * zoomAction, * bringAllToTopAction;
    Action * nextTabAction, * prevTabAction;
    Action * chatsAction;
    Action * newContactAction;
    Action * newGroupAction;
    Action * newAccountAction;
    Action * findAction;
    QHash<ITabPage*, Action*> activeChatsActions;
    // other plugins
    IAccountManager * accountManager;
    IRosterChanger * rosterChanger;
    IOptionsManager * optionsManager;
    IRosterSearch * rosterSearch;
    IMainWindowPlugin * mainWindow;
    IMetaContacts * metaContacts;
};

#endif // MACINTEGRATIONPLUGIN_H
