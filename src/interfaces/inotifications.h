#ifndef INOTIFICATIONS_H
#define INOTIFICATIONS_H

#include <QMap>
#include <QIcon>
#include <QImage>
#include <utils/jid.h>
#include <utils/action.h>

#define NOTIFICATIONS_UUID  "{57208da0-40a7-4aa3-9428-3056115d2ef8}"

struct INotification
{
	enum NotifyKinds {
		RosterNotify          = 0x0001,
		PopupWindow           = 0x0002,
		TrayNotify            = 0x0004,
		SoundPlay             = 0x0008,
		AlertWidget           = 0x0010,
		ShowMinimized         = 0x0020,
		TabPageNotify         = 0x0040,
		AutoActivate          = 0x8000
	};
	enum NotifyFlags {
		RemoveInvisible       = 0x0001,
		TestNotify            = 0x8000
	};
	INotification() { 
		kinds = 0;
		flags = RemoveInvisible;
	}
	QString typeId;
	ushort kinds;
	ushort flags;
	QList<Action *> actions;
	QMap<int, QVariant> data;
};

struct INotificationType
{
	INotificationType() {
		order = 0;
		kindMask = 0;
		kindDefs = 0;
	}
	int order;
	QString title;
	ushort kindMask;
	ushort kindDefs;
};

class INotifications
{
public:
	virtual QObject *instance() =0;
	virtual QList<int> notifications() const =0;
	virtual INotification notificationById(int ANotifyId) const =0;
	virtual int appendNotification(const INotification &ANotification) =0;
	virtual void activateNotification(int ANotifyId) =0;
	virtual void removeNotification(int ANotifyId) =0;
	virtual void registerNotificationType(const QString &ATypeId, const INotificationType &AType) =0;
	virtual QList<QString> notificationTypes() const =0;
	virtual INotificationType notificationType(const QString &ATypeId) const =0;
	virtual ushort notificationKinds(const QString &ATypeId) const =0;
	virtual void setNotificationKinds(const QString &ATypeId, ushort AKinds) =0;
	virtual void removeNotificationType(const QString &ATypeId) =0;
	virtual QImage contactAvatar(const Jid &AStreamJid,const Jid &AContactJid) const =0;
	virtual QIcon contactIcon(const Jid &AStreamJid, const Jid &AContactJid) const =0;
	virtual QString contactName(const Jid &AStreamJId, const Jid &AContactJid) const =0;
protected:
	virtual void notificationActivated(int ANotifyId) =0;
	virtual void notificationRemoved(int ANotifyId) =0;
	virtual void notificationAppend(int ANotifyId, INotification &ANotification) =0;
	virtual void notificationAppended(int ANotifyId, const INotification &ANotification) =0;
	virtual void notificationTest(const QString &ATypeId, ushort AKinds) =0;
};

Q_DECLARE_INTERFACE(INotifications,"Virtus.Plugin.INotifications/1.0")

#endif
