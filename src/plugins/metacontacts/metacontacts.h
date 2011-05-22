#ifndef METACONTACTS_H
#define METACONTACTS_H

#include <QMultiMap>
#include <QObjectCleanupHandler>
#include <definitions/resources.h>
#include <definitions/menuicons.h>
#include <definitions/soundfiles.h>
#include <definitions/actiongroups.h>
#include <definitions/metaitemorders.h>
#include <definitions/gateserviceidentifiers.h>
#include <definitions/rosterproxyorders.h>
#include <definitions/rosterindextyperole.h>
#include <definitions/rosterclickhookerorders.h>
#include <definitions/rosterdragdropmimetypes.h>
#include <definitions/toolbargroups.h>
#include <definitions/customborder.h>
#include <definitions/notificators.h>
#include <definitions/optionwidgetorders.h>
#include <definitions/notificationdataroles.h>
#include <interfaces/ipluginmanager.h>
#include <interfaces/imetacontacts.h>
#include <interfaces/irosterchanger.h>
#include <interfaces/irostersview.h>
#include <interfaces/imessageprocessor.h>
#include <interfaces/istatusicons.h>
#include <interfaces/irostersearch.h>
#include <interfaces/igateways.h>
#include <interfaces/inotifications.h>
#include <utils/widgetmanager.h>
#include <utils/customborderstorage.h>
#include "metaroster.h"
#include "metaproxymodel.h"
#include "metatabwindow.h"
#include "mergecontactsdialog.h"
#include "metacontextmenu.h"
#include "metaprofiledialog.h"

struct TabPageInfo
{
	Jid streamJid;
	QString metaId;
	ITabPage *page;
};

class GroupMenu :
	public Menu
{
	Q_OBJECT
public:
	GroupMenu(QWidget* AParent = NULL) : Menu(AParent) { }
	virtual ~GroupMenu() {}
protected:
	void mouseReleaseEvent(QMouseEvent *AEvent);
};

class MetaContacts :
	public QObject,
	public IPlugin,
	public IMetaContacts,
	public ITabPageHandler,
	public IRostersClickHooker,
	public IRostersKeyPressHooker,
	public IRostersDragDropHandler,
	public IViewDropHandler
{
	Q_OBJECT
	Q_INTERFACES(IPlugin IMetaContacts ITabPageHandler IRostersClickHooker IRostersKeyPressHooker IRostersDragDropHandler IViewDropHandler)
public:
	MetaContacts();
	~MetaContacts();
	virtual QObject* instance() { return this; }
	//IPlugin
	virtual QUuid pluginUuid() const { return METACONTACTS_UUID; }
	virtual void pluginInfo(IPluginInfo *APluginInfo);
	virtual bool initConnections(IPluginManager *APluginManager, int &AInitOrder);
	virtual bool initObjects();
	virtual bool initSettings() { return true; }
	virtual bool startPlugin() { return true; }
	//ITabPageHandler
	virtual bool tabPageAvail(const QString &ATabPageId) const;
	virtual ITabPage *tabPageFind(const QString &ATabPageId) const;
	virtual ITabPage *tabPageCreate(const QString &ATabPageId);
	virtual Action *tabPageAction(const QString &ATabPageId, QObject *AParent);
	//IRostersClickHooker
	virtual bool rosterIndexClicked(IRosterIndex *AIndex, int AOrder);
	//IRostersKeyPressHooker
	virtual bool keyOnRosterIndexPressed(IRosterIndex *AIndex, int AOrder, Qt::Key key, Qt::KeyboardModifiers modifiers);
	virtual bool keyOnRosterIndexesPressed(IRosterIndex *AIndex, QList<IRosterIndex*> ASelected, int AOrder, Qt::Key key, Qt::KeyboardModifiers modifiers);
	//IRostersDragDropHandler
	virtual Qt::DropActions rosterDragStart(const QMouseEvent *AEvent, const QModelIndex &AIndex, QDrag *ADrag);
	virtual bool rosterDragEnter(const QDragEnterEvent *AEvent);
	virtual bool rosterDragMove(const QDragMoveEvent *AEvent, const QModelIndex &AHover);
	virtual void rosterDragLeave(const QDragLeaveEvent *AEvent);
	virtual bool rosterDropAction(const QDropEvent *AEvent, const QModelIndex &AIndex, Menu *AMenu);
	// IViewDropHandler
	virtual bool viewDragEnter(IViewWidget *AWidget, const QDragEnterEvent *AEvent);
	virtual bool viewDragMove(IViewWidget *AWidget, const QDragMoveEvent *AEvent);
	virtual void viewDragLeave(IViewWidget *AWidget, const QDragLeaveEvent *AEvent);
	virtual bool viewDropAction(IViewWidget *AWidget, const QDropEvent *AEvent, Menu *AMenu);
	//IMetaContacts
	virtual QList<IMetaItemDescriptor> metaDescriptors() const;
	virtual IMetaItemDescriptor metaDescriptorByOrder(int APageOrder) const;
	virtual IMetaItemDescriptor metaDescriptorByItem(const Jid &AItemJid) const;
	virtual QString itemHint(const Jid &AItemJid) const;
	virtual QMultiMap<int, Jid> itemOrders(QList<Jid> AItems) const;
	virtual QString metaContactName(const IMetaContact &AContact) const;
	virtual IMetaRoster *newMetaRoster(IRoster *ARoster);
	virtual IMetaRoster *findMetaRoster(const Jid &AStreamJid) const;
	virtual void removeMetaRoster(IRoster *ARoster);
	virtual QString metaRosterFileName(const Jid &AStreamJid) const;
	virtual QList<IMetaTabWindow *> metaTabWindows() const;
	virtual IMetaTabWindow *newMetaTabWindow(const Jid &AStreamJid, const QString &AMetaId);
	virtual IMetaTabWindow *findMetaTabWindow(const Jid &AStreamJid, const QString &AMetaId) const;
	virtual QString deleteContactWithNotify(IMetaRoster *ARoster, const QString &AMetaId, const Jid &ItemJid = Jid::null);
signals:
	void metaRosterAdded(IMetaRoster *AMetaRoster);
	void metaRosterOpened(IMetaRoster *AMetaRoster);
	void metaAvatarChanged(IMetaRoster *AMetaRoster, const QString &AMetaId);
	void metaPresenceChanged(IMetaRoster *AMetaRoster, const QString &AMetaId);
	void metaContactReceived(IMetaRoster *AMetaRoster, const IMetaContact &AContact, const IMetaContact &ABefore);
	void metaActionResult(IMetaRoster *AMetaRoster, const QString &AActionId, const QString &AErrCond, const QString &AErrMessage);
	void metaRosterClosed(IMetaRoster *AMetaRoster);
	void metaRosterEnabled(IMetaRoster *AMetaRoster, bool AEnabled);
	void metaRosterStreamJidAboutToBeChanged(IMetaRoster *AMetaRoster, const Jid &AAfter);
	void metaRosterStreamJidChanged(IMetaRoster *AMetaRoster, const Jid &ABefore);
	void metaRosterRemoved(IMetaRoster *AMetaRoster);
	void metaTabWindowCreated(IMetaTabWindow *AWindow);
	void metaTabWindowDestroyed(IMetaTabWindow *AWindow);
	//ITabPageHandler
	void tabPageCreated(ITabPage *ATabPage);
	void tabPageDestroyed(ITabPage *ATabPage);
protected:
	void initMetaItemDescriptors();
	void deleteMetaRosterWindows(IMetaRoster *AMetaRoster);
	IMetaRoster *findBareMetaRoster(const Jid &AStreamJid) const;
	MetaProfileDialog *findMetaProfileDialog(const Jid &AStreamJid, const QString &AMetaId) const;
	void hideMetaContact(IMetaRoster *AMetaRoster, const QString &AMetaId);
	void unhideMetaContact(IMetaRoster *AMetaRoster, const QString &AMetaId);
	void notifyContactDeleteFailed(IMetaRoster *ARoster, const QString &AActionId, const QString &AErrCond, const QString &AErrMessage);
	void updateContactChatWindows(IMetaRoster *AMetaRoster, const IMetaContact &AContact, const IMetaContact &ABefore);
protected slots:
	void onMetaRosterOpened();
	void onMetaAvatarChanged(const QString &AMetaId);
	void onMetaPresenceChanged(const QString &AMetaId);
	void onMetaContactReceived(const IMetaContact &AContact, const IMetaContact &ABefore);
	void onMetaActionResult(const QString &AActionId, const QString &AErrCond, const QString &AErrMessage);
	void onMetaRosterClosed();
	void onMetaRosterEnabled(bool AEnabled);
	void onMetaRosterStreamJidAboutToBeChanged(const Jid &AAfter);
	void onMetaRosterStreamJidChanged(const Jid &ABefour);
	void onMetaRosterDestroyed(QObject *AObject);
	void onRosterAdded(IRoster *ARoster);
	void onRosterRemoved(IRoster *ARoster);
protected slots:
	void onMetaTabWindowActivated();
	void onMetaTabWindowPageWidgetRequested(const QString &APageId);
	void omMetaTabWindowPageContextMenuRequested(const QString &APageId, Menu *AMenu);
	void onMetaTabWindowDestroyed();
protected slots:
	void onRenameContact(bool);
	void onNewNameSelected(const QString & newName);
	void onDeleteContact(bool);
	void onDeleteContactDialogAccepted();
	void onMergeContacts(bool);
	void onCopyToGroup(bool);
	void onMoveToGroup(bool);
	void onRemoveFromGroup(bool);
	void onDetachContactItems(bool);
	void onChangeContactGroups(bool AChecked);
	void onNewGroupNameSelected(const QString & group);
protected slots:
	void onContactSubscription(bool);
	void onContactItemSubscription(bool);
protected slots:
	void onLoadMetaRosters();
	void onOpenTabPageAction(bool);
	void onSendContactDataAction(bool);
	void onShowMetaTabWindowAction(bool);
	void onShowMetaProfileDialogAction(bool);
	void onMetaProfileDialogDestroyed();
	void onChatWindowCreated(IChatWindow *AWindow);
	void onRosterAcceptMultiSelection(QList<IRosterIndex *> ASelected, bool &AAccepted);
	void onRosterIndexContextMenu(IRosterIndex *AIndex, QList<IRosterIndex *> ASelected, Menu *AMenu);
	void onRosterLabelToolTips(IRosterIndex *AIndex, int ALabelId, QMultiMap<int,QString> &AToolTips, ToolBarChanger *AToolBarChanger);
	void onNotificationActivated(int ANotifyId);
	void onNotificationRemoved(int ANotifyId);
	void onOptionsOpened();
	void onOptionsClosed();
private:
	IPluginManager *FPluginManager;
	IRosterPlugin *FRosterPlugin;
	IRosterChanger *FRosterChanger;
	IRostersViewPlugin *FRostersViewPlugin;
	IMessageWidgets *FMessageWidgets;
	IMessageProcessor *FMessageProcessor;
	IStatusIcons *FStatusIcons;
	IRosterSearch *FRosterSearch;
	IGateways *FGateways;
	INotifications *FNotifications;
private:
	QList<IMetaRoster *> FLoadQueue;
	QList<IMetaRoster *> FMetaRosters;
	QObjectCleanupHandler FCleanupHandler;
private:
	MetaProxyModel *FMetaProxyModel;
	QHash<QString, TabPageInfo> FTabPages;
	QList<IMetaTabWindow *> FMetaTabWindows;
	IMetaItemDescriptor FDefaultItemDescriptor;
	QList<MetaProfileDialog *> FMetaProfileDialogs;
	QList<IMetaItemDescriptor> FMetaItemDescriptors;
private:
	QList<int> FFailDeleteNotifies;
	QMap<IMetaRoster *, QMap<QString, QString> > FDeleteActions;
private:
	mutable QHash<Jid, int> FItemDescrCache;
};

#endif // METACONTACTS_H
