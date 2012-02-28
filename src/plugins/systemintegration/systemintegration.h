#ifndef SYSTEMINTEGRATION_H
#define SYSTEMINTEGRATION_H

#include <interfaces/isystemintegration.h>
#include <interfaces/ipluginmanager.h>

class SystemIntegration :
		public QObject,
		public IPlugin,
		public ISystemIntegration
{
	Q_OBJECT
	Q_INTERFACES(IPlugin ISystemIntegration)
public:
	SystemIntegration();
	~SystemIntegration();
	// IPlugin
	virtual QObject *instance() { return this; }
	virtual QUuid pluginUuid() const { return SYSTEMINTEGRATION_UUID; }
	virtual void pluginInfo(IPluginInfo *APluginInfo);
	virtual bool initConnections(IPluginManager *APluginManager, int &AInitOrder);
	virtual bool initObjects();
	virtual bool initSettings();
	virtual bool startPlugin();
	// ISystemIntegration
	// system implementation
	virtual ISystemIntegrationImplementation * implementation() { return impl; }
	// menus
	virtual bool isGlobalMenuPresent() const;
	virtual bool isDockMenuPresent() const;
	virtual void addAction(MenuActionRole role, Action * action, int group = AG_DEFAULT);
	virtual void removeAction(Action * action);
	// dock
	virtual bool isDockPresent() const;
	virtual void setDockBadge(const QString & badge);
	virtual void setDockOverlayImage(const QImage & image, Qt::Alignment alignment = Qt::AlignCenter, bool showAppIcon = true);
	virtual void requestUserAttention();
	// notifications
	virtual bool isSystemNotificationsSettingsAccessible() const;
	virtual void postSystemNotify(const QImage & icon, const QString & title, const QString & text, const QString & type, int id);
	virtual void showSystemNotificationsSettings();
signals:
	void dockClicked();
	void systemNotificationClicked(int);
private:
	ISystemIntegrationImplementation * impl;
};

#endif // SYSTEMINTEGRATION_H
