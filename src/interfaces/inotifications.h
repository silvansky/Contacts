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
		RosterIcon    = 0x01,
		PopupWindow   = 0x02,
		TrayIcon      = 0x04,
		TrayAction    = 0x08,
		PlaySound     = 0x10,
		AutoActivate  = 0x20,
		ChatWindow    = 0x40,
		TestNotify    = 0x80
	};
	INotification() { kinds = 0; }
	uchar kinds;
	QList<Action *> actions;
	QMap<int, QVariant> data;
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
	virtual void insertNotificator(const QString &ANotificatorId, int AWidgetOrder, const QString &ATitle, uchar AKindMask, uchar ADefault) =0;
	virtual uchar notificatorKinds(const QString &ANotificatorId) const =0;
	virtual void setNotificatorKinds(const QString &ANotificatorId, uchar AKinds) =0;
	virtual void removeNotificator(const QString &ANotificatorId) =0;
	virtual QImage contactAvatar(const Jid &AContactJid) const =0;
	virtual QIcon contactIcon(const Jid &AStreamJid, const Jid &AContactJid) const =0;
	virtual QString contactName(const Jid &AStreamJId, const Jid &AContactJid) const =0;
protected:
	virtual void notificationAppend(int ANotifyId, INotification &ANotification) =0;
	virtual void notificationAppended(int ANotifyId, const INotification &ANotification) =0;
	virtual void notificationActivated(int ANotifyId) =0;
	virtual void notificationRemoved(int ANotifyId) =0;
	virtual void notificationTest(const QString &ANotificatorId, uchar AKinds) =0;
};

Q_DECLARE_INTERFACE(INotifications,"Virtus.Plugin.INotifications/1.0")

#endif
