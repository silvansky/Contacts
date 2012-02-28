#ifndef ISYSTEMINTEGRATION_H
#define ISYSTEMINTEGRATION_H

#include <QMenuBar>
#include <utils/menu.h>

#define SYSTEMINTEGRATION_UUID "{c1bab057-ff28-418e-9654-faf24538d237}"

// interface for real plugins
class ISystemIntegrationImplementation
{
public:
	virtual QObject * instance() = 0;

	// initialization
	virtual void init() = 0;

	// finalization
	virtual void finalize() = 0;

	// information
	virtual bool isGlobalMenuPresent() const = 0;
	virtual bool isDockMenuPresent() const = 0;
	virtual bool isDockPresent() const = 0;
	virtual bool isSystemNotificationsAccessible() const = 0;
	virtual QString systemNotificationsSystemName() const = 0;
	virtual bool isSystemNotificationsSettingsAccessible() const = 0;

	// menus
	virtual Menu * dockMenu() = 0;
	virtual QMenuBar * menuBar() = 0;
	virtual Menu * fileMenu() = 0;
	virtual Menu * editMenu() = 0;
	virtual Menu * viewMenu() = 0;
	virtual Menu * statusMenu() = 0;
	virtual Menu * windowMenu() = 0;
	virtual Menu * helpMenu() = 0;

	// dock
	virtual void setDockBadge(const QString & badgeText) = 0;
	virtual void setDockOverlayImage(const QImage & image, Qt::Alignment alignment = Qt::AlignCenter, bool showAppIcon = true) = 0;
	virtual bool isRequestUserAttentionPresent() const = 0;
	virtual void requestUserAttention() = 0;

	// notifications
	virtual void postSystemNotify(const QImage & icon, const QString & title, const QString & text, const QString & type, int id) = 0;
	virtual void showSystemNotificationsSettings() = 0;

protected:
	// signals
	virtual void dockClicked() = 0;
	virtual void systemNotificationClicked(int) = 0;
};

// system integration plugin
class ISystemIntegration
{
public:
	// action roles
	enum MenuActionRole
	{
		ApplicationRole,
		SettingsRole,
		FileRole,
		EditRole,
		ViewRole,
		StatusRole,
		WindowRole,
		HelpRole,
		DockRole
	};
	virtual QObject * instance() = 0;
	// system implementation
	virtual ISystemIntegrationImplementation * implementation() = 0;
	// menus
	virtual bool isGlobalMenuPresent() const = 0;
	virtual bool isDockMenuPresent() const = 0;
	virtual void addAction(MenuActionRole role, Action * action, int group = AG_DEFAULT) = 0;
	virtual void removeAction(MenuActionRole role, Action * action) = 0;
	// dock
	virtual bool isDockPresent() const = 0;
	virtual void setDockBadge(const QString & badge) = 0;
	virtual void setDockOverlayImage(const QImage & image, Qt::Alignment alignment = Qt::AlignCenter, bool showAppIcon = true) = 0;
	virtual bool isRequestUserAttentionPresent() const = 0;
	virtual void requestUserAttention() = 0;
	// notifications
	virtual bool isSystemNotificationsAccessible() const = 0;
	virtual QString systemNotificationsSystemName() const = 0;
	virtual bool isSystemNotificationsSettingsAccessible() const = 0;
	virtual void postSystemNotify(const QImage & icon, const QString & title, const QString & text, const QString & type, int id) = 0;
	virtual void showSystemNotificationsSettings() = 0;
protected:
	// signals
	virtual void dockClicked() = 0;
	virtual void systemNotificationClicked(int) = 0;
};

Q_DECLARE_INTERFACE(ISystemIntegration,"Virtus.Core.ISystemIntegration/1.0")
Q_DECLARE_INTERFACE(ISystemIntegrationImplementation,"Virtus.Core.ISystemIntegrationImplementation/1.0")

#endif // ISYSTEMINTEGRATION_H
