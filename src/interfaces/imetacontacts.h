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
	QSet<Jid> items;
	QSet<QString> groups;
};

struct IMetaItemDescriptor
{
	QString name;
	QString icon;
	bool detach;
	bool combine;
	bool service;
	int pageOrder;
	QString contactPattern;
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
	// Возвращает идентификатор метаконтакта metaId.
	// AItemJid - это ContactJid может быть как с ресурсом так и без. Ресурс игнорируется.
	virtual QString itemMetaContact(const Jid &AItemJid) const =0;
	virtual IPresenceItem metaPresence(const QString &AMetaId) const =0;
	virtual QList<IPresenceItem> itemPresences(const Jid &AItemJid) const =0;
	virtual QString metaAvatarHash(const QString &AMetaId) const =0;
	virtual QImage metaAvatarImage(const QString &AMetaId, bool ANullImage = true) const =0;
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
	virtual bool renameGroup(const QString &AGroup, const QString &ANewName) =0;
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
	virtual ITabPage *itemPage(const Jid &AItemJid) const =0;
	virtual void setItemPage(const Jid &AItemJid, ITabPage *APage) =0;
	virtual Jid currentItem() const =0;
	virtual void setCurrentItem(const Jid &AItemJid) =0;
	virtual void insertTopWidget(int AOrder, QWidget *AWidget) =0;
	virtual void removeTopWidget(QWidget *AWidget) =0;
protected:
	virtual void currentItemChanged(const Jid &AItemJid) =0;
	virtual void itemPageRequested(const Jid &AItemJid) =0;
	virtual void itemPageChanged(const Jid &AItemJid, ITabPage *APage) =0;
	virtual void itemContextMenuRequested(const Jid &AItemJid, Menu *AMenu) =0;
	virtual void topWidgetInserted(int AOrder, QWidget *AWidget) =0;
	virtual void topWidgetRemoved(QWidget* AWidget) =0;
};

class IMetaContacts
{
public:
	virtual QObject *instance() =0;
	virtual QString itemHint(const Jid &AItemJid) const =0;
	virtual IMetaItemDescriptor itemDescriptor(const Jid &AItemJid) const =0;
	virtual QMultiMap<int, Jid> itemOrders(const QList<Jid> &AItems) const =0;
	virtual QString metaContactName(const IMetaContact &AContact) const =0;
	virtual IMetaRoster *newMetaRoster(IRoster *ARoster) =0;
	virtual IMetaRoster *findMetaRoster(const Jid &AStreamJid) const =0;
	virtual void removeMetaRoster(IRoster *ARoster) =0;
	virtual QString metaRosterFileName(const Jid &AStreamJid) const =0;
	virtual QList<IMetaTabWindow *> metaTabWindows() const =0;
	virtual IMetaTabWindow *newMetaTabWindow(const Jid &AStreamJid, const QString &AMetaId) =0;
	virtual IMetaTabWindow *findMetaTabWindow(const Jid &AStreamJid, const QString &AMetaId) const =0;
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
