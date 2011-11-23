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
#include <interfaces/istatuschanger.h>
#include <interfaces/iemoticons.h>

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
    virtual bool initSettings();
    virtual bool startPlugin() { return true; }
    //IMacIntegration
    virtual Menu * dockMenu();
    virtual QMenuBar * menuBar();
    virtual Menu * fileMenu();
    virtual Menu * editMenu();
    virtual Menu * viewMenu();
    virtual Menu * statusMenu();
    virtual Menu * windowMenu();
    virtual Menu * helpMenu();
    virtual void setDockBadge(const QString & badgeText);
    virtual void postGrowlNotify(const QImage & icon, const QString & title, const QString & text, const QString & type, int id);
    virtual void showGrowlPreferencePane();
    virtual void setCustomBorderColor(const QColor & color);
    virtual void setCustomTitleColor(const QColor & color);
    virtual void setWindowMovableByBackground(QWidget * window, bool movable);
signals:
    void dockClicked();
    void growlNotifyClicked(int);
private:
    void initMenus();
    void updateActions();
    void updateContactActions();
private slots:
    void onAboutToQuit();
    void onOptionsChanged(const OptionsNode &ANode);
    void onFocusChanged(QWidget * old, QWidget * now);
    // profiles
    void onProfileOpened(const QString & name);
    void onProfileClosed(const QString & name);
    // tabs
    void onMetaTabWindowCreated(IMetaTabWindow *AWindow);
    void onMetaTabPageClosed();
    void onMetaTabPageActivated();
    // statuses
    void onStatusChanged(const Jid &AStreamJid, int AStatusId);
    void onStatusItemAdded(int status);
    void onStatusItemChanged(int status);
    void onStatusItemRemoved(int status);
    // file menu
    void onNewContactAction();
    void onNewGroupAction();
    void onNewAccountAction();
    void onCloseTabAction();
    void onCloseAllTabsAction();
    // status menu
    void onStatusAction();
    void onManageAccountsAction();
    void onAutoStatusAction(bool on);
    // window menu
    void onMinimizeAction();
    void onZoomAction();
    void onCloseAction();
    void onNextTabAction();
    void onPrevTabAction();
    void onBringAllToTopAction();
    void onShowMainWindowAction();
    void onContactAction();
    // help menu
    void onOnlineHelpAction();
    void onFeedbackAction();
    void onFacebookAction();
    void onRulesAction();
    void onFunAction();
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
    Action * autoStatusAction;
    Action * manageAccountsAction;
    Action * findAction;
    Menu * emoticonsMenu;
    QMap<ITabPage*, Action*> activeChatsActions;
    QMap<int, Action *> availableStatuses;
    Action * onlineHelpAction;
    Action * feedbackAction;
    Action * facebookAction;
    Action * rulesAction;
    Action * funAction;
    QMap<QString, QString> funLinks;
    // other plugins
    IPluginManager * pluginManager;
    IAccountManager * accountManager;
    IRosterChanger * rosterChanger;
    IOptionsManager * optionsManager;
    IRosterSearch * rosterSearch;
    IMainWindowPlugin * mainWindow;
    IMetaContacts * metaContacts;
    IStatusChanger * statusChanger;
    IEmoticons * emoticons;
};

#endif // MACINTEGRATIONPLUGIN_H
