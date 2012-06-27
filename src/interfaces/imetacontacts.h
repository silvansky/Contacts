#ifndef IMETACONTACTS_H
#define IMETACONTACTS_H

#include <QSet>
#include <QMainWindow>
#include <interfaces/iroster.h>
#include <interfaces/ipresence.h>
#include <interfaces/imessagewidgets.h>

#define METACONTACTS_UUID "{D2E1D146-F98F-4868-89C0-308F72062BFA}"

struct IMetaContact
{
	QString id;
	QString name;
	QString ask;
	QString subscription;
	QSet<Jid> items;
	QSet<QString> groups;
	bool operator==(const IMetaContact &AOther) const {
		return id==AOther.id && name==AOther.name && ask==AOther.ask && subscription==AOther.subscription && items==AOther.items && groups==AOther.groups;
	}
	bool operator!=(const IMetaContact &AOther) const {
		return !operator==(AOther);
	}
};

struct IMetaItemDescriptor
{
	IMetaItemDescriptor() {
		detach = true;
		combine = false;
		service = false;
		persistent = false;
		metaOrder = -1;
	}
	QString name;
	QString icon;
	bool detach;
	bool combine;
	bool service;
	bool persistent;
	bool hidden;
	int metaOrder;
	QString gateId;
	QString gatePrefix;
	QList<QString> domains;
};

class IMetaRoster
{
public:
	virtual QObject *instance() =0;
	virtual bool isEnabled() const =0;
	virtual Jid streamJid() const =0;
	virtual IRoster *roster() const =0;
	virtual bool isOpen() const =0;
	virtual QList<QString> metaContacts() const =0;
	virtual IMetaContact metaContact(const QString &AMetaId) const =0;
	virtual QString itemMetaContact(const Jid &AItemJid) const =0;
	virtual IPresenceItem metaPresenceItem(const QString &AMetaId) const =0;
	virtual QList<IPresenceItem> itemPresences(const Jid &AItemJid) const =0;
	virtual QString metaAvatarHash(const QString &AMetaId) const =0;
	virtual QImage metaAvatarImage(const QString &AMetaId, bool AAllowNull = true, bool AAllowGray = true) const =0;
	virtual QSet<QString> groups() const =0;
	virtual QList<IMetaContact> groupContacts(const QString &AGroup) const =0;
	virtual void saveMetaContacts(const QString &AFileName) const =0;
	virtual void loadMetaContacts(const QString &AFileName) =0;
	//Operations on contacts
	virtual QString createContact(const IMetaContact &AContact) =0;
	virtual QString renameContact(const QString &AMetaId, const QString &ANewName) =0;
	virtual QString deleteContact(const QString &AMetaId) =0;
	virtual QString mergeContacts(const QString &ADestId, const QList<QString> &AMetaIds) =0;
	virtual QString setContactGroups(const QString &AMetaId, const QSet<QString> &AGroups) =0;
	//Operations on contact items
	virtual QString detachContactItem(const QString &AMetaId, const Jid &AItemJid) =0;
	virtual QString deleteContactItem(const QString &AMetaId, const Jid &AItemJid) =0;
	//Operations on groups
	virtual QString renameGroup(const QString &AGroup, const QString &ANewName) =0;
protected:
	virtual void metaRosterOpened() =0;
	virtual void metaAvatarChanged(const QString &AMetaId) =0;
	virtual void metaPresenceChanged(const QString &AMetaId) =0;
	virtual void metaContactReceived(const IMetaContact &AContact, const IMetaContact &ABefore) =0;
	virtual void metaActionResult(const QString &AActionId, const QString &AErrCond, const QString &AErrMessage) =0;
	virtual void metaRosterClosed() =0;
	virtual void metaRosterEnabled(bool AEnabled) =0;
	virtual void metaRosterStreamJidAboutToBeChanged(const Jid &AAfter) =0;
	virtual void metaRosterStreamJidChanged(const Jid &ABefore) =0;
};

class IMetaTabWindow :
	public ITabPage
{
public:
	virtual QMainWindow *instance() =0;
	virtual QString metaId() const =0;
	virtual IMetaRoster *metaRoster() const =0;
	virtual ToolBarChanger *toolBarChanger() const =0;
	virtual void insertTopWidget(int AOrder, QWidget *AWidget) =0;
	virtual void removeTopWidget(QWidget *AWidget) =0;
	//Common pages
	virtual void createFirstPage() =0;
	virtual QList<QString> pages() const =0;
	virtual QString currentPage() const =0;
	virtual void setCurrentPage(const QString &APageId) =0;
	virtual QString insertPage(int AOrder, bool ACombine = false) =0;
	virtual QIcon pageIcon(const QString &APageId) const =0;
	virtual void setPageIcon(const QString &APageId, const QIcon &AIcon) =0;
	virtual void setPageIcon(const QString &APageId, const QString &AMetaIcon) =0;
	virtual QString pageName(const QString &APageId) const =0;
	virtual void setPageName(const QString &APageId, const QString &AName) =0;
	virtual QString widgetPage(ITabPage *APage) const =0;
	virtual ITabPage *pageWidget(const QString &APageId) const =0;
	virtual void setPageWidget(const QString &APageId, ITabPage *AWidget) =0;
	virtual void removePage(const QString &APageId) =0;
	//Item pages
	virtual bool isContactPage() const =0;
	virtual Jid currentItem() const =0;
	virtual void setCurrentItem(const Jid &AItemJid) =0;
	virtual Jid pageItem(const QString &APageId) const =0;
	virtual QString itemPage(const Jid &AItemJid) const =0;
	virtual ITabPage *itemWidget(const Jid &AItemJid) const =0;
	virtual void setItemWidget(const Jid &AItemJid, ITabPage *AWidget) =0;
protected:
	virtual void currentPageChanged(const QString &APageId) =0;
	virtual void pageInserted(const QString &APageId, int AOrder, bool ACombined) =0;
	virtual void pageChanged(const QString &APageId) =0;
	virtual void pageWidgetRequested(const QString &APageId) =0;
	virtual void pageContextMenuRequested(const QString &APageId, Menu *AMenu) =0;
	virtual void pageRemoved(const QString &APageId) =0;
	virtual void topWidgetInserted(int AOrder, QWidget *AWidget) =0;
	virtual void topWidgetRemoved(QWidget* AWidget) =0;
};

class IMetaContacts
{
public:
	virtual QObject *instance() =0;
	virtual QList<IMetaItemDescriptor> metaDescriptors() const =0;
	virtual IMetaItemDescriptor metaDescriptorByOrder(int APageOrder) const =0;
	virtual IMetaItemDescriptor metaDescriptorByItem(const Jid &AItemJid) const =0;
	virtual QString itemFormattedLogin(const Jid &AItemJid) const =0;
	virtual QMultiMap<int, Jid> itemOrders(QList<Jid> AItems) const =0;
	virtual QString metaContactName(const IMetaContact &AContact) const =0;
	virtual IMetaRoster *getMetaRoster(IRoster *ARoster) =0;
	virtual IMetaRoster *findMetaRoster(const Jid &AStreamJid) const =0;
	virtual void removeMetaRoster(IRoster *ARoster) =0;
	virtual QString metaRosterFileName(const Jid &AStreamJid) const =0;
	virtual QList<IMetaTabWindow *> metaTabWindows() const =0;
	virtual IMetaTabWindow *getMetaTabWindow(const Jid &AStreamJid, const QString &AMetaId) =0;
	virtual IMetaTabWindow *findMetaTabWindow(const Jid &AStreamJid, const QString &AMetaId) const =0;
	virtual quint32 editMetaContactRestrictions(const Jid &AStreamJid, const QString &AMetaId, const Jid &AItemJid = Jid::null) const =0;
	virtual QString deleteContactWithNotify(const Jid &AStreamJid, const QString &AMetaId, const Jid &AItemJid = Jid::null) =0;
	virtual QDialog *showMetaProfileDialog(const Jid &AStreamJid, const QString &AMetaId) =0;
	virtual QDialog *showRenameContactDialog(const Jid &AStreamJid, const QString &AMetaId) =0;
protected:
	virtual void metaRosterAdded(IMetaRoster *AMetaRoster) =0;
	virtual void metaRosterOpened(IMetaRoster *AMetaRoster) =0;
	virtual void metaAvatarChanged(IMetaRoster *AMetaRoster, const QString &AMetaId) =0;
	virtual void metaPresenceChanged(IMetaRoster *AMetaRoster, const QString &AMetaId) =0;
	virtual void metaContactReceived(IMetaRoster *AMetaRoster, const IMetaContact &AContact, const IMetaContact &ABefore) =0;
	virtual void metaActionResult(IMetaRoster *AMetaRoster, const QString &AActionId, const QString &AErrCond, const QString &AErrMessage) =0;
	virtual void metaRosterClosed(IMetaRoster *AMetaRoster) =0;
	virtual void metaRosterEnabled(IMetaRoster *AMetaRoster, bool AEnabled) =0;
	virtual void metaRosterRemoved(IMetaRoster *AMetaRoster) =0;
	virtual void metaTabWindowCreated(IMetaTabWindow *AWindow) =0;
	virtual void metaTabWindowDestroyed(IMetaTabWindow *AWindow) =0;
};

Q_DECLARE_INTERFACE(IMetaRoster,"Virtus.Plugin.IMetaRoster/1.0")
Q_DECLARE_INTERFACE(IMetaTabWindow,"Virtus.Plugin.IMetaTabWindow/1.0")
Q_DECLARE_INTERFACE(IMetaContacts,"Virtus.Plugin.IMetaContacts/1.0")

#endif
