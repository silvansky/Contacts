#ifndef WIN7INTEGRATION_H
#define WIN7INTEGRATION_H

#include <interfaces/iwin7integration.h>
#include <interfaces/ipluginmanager.h>
#include <interfaces/isystemintegration.h>

class Win7Integration :
		public QObject,
		public IPlugin,
		public IWin7Integration,
		public ISystemIntegrationImplementation
{
	Q_OBJECT
	Q_INTERFACES(IPlugin IWin7Integration ISystemIntegrationImplementation)
public:
	Win7Integration();
	~Win7Integration();
	//IPlugin
	virtual QObject *instance() { return this; }
	virtual QUuid pluginUuid() const { return WIN7INTEGRATION_UUID; }
	virtual void pluginInfo(IPluginInfo *APluginInfo);
	virtual bool initConnections(IPluginManager *APluginManager, int &AInitOrder) { return true; }
	virtual bool initObjects() { return true; }
	virtual bool initSettings() { return true; }
	virtual bool startPlugin() { return true; }
	//IWin7Integration
	//ISystemIntegrationImplementation
	virtual void init() {;}
	virtual void finalize() {;}
	virtual bool isGlobalMenuPresent() const { return false; }
	virtual bool isDockMenuPresent() const { return false; }
	virtual bool isDockPresent() const { return false; }
	virtual bool isSystemNotificationsAccessible() const { return false; }
	virtual QString systemNotificationsSystemName() const { return QString::null; }
	virtual bool isSystemNotificationsSettingsAccessible() const { return false; }
	virtual Menu * dockMenu() { return NULL; }
	virtual QMenuBar * menuBar() { return NULL; }
	virtual Menu * fileMenu() { return NULL; }
	virtual Menu * editMenu() { return NULL; }
	virtual Menu * viewMenu() { return NULL; }
	virtual Menu * statusMenu() { return NULL; }
	virtual Menu * windowMenu() { return NULL; }
	virtual Menu * helpMenu() { return NULL; }
	virtual void setDockBadge(const QString & badgeText) {;}
	virtual void setDockOverlayImage(const QImage & image, Qt::Alignment alignment = Qt::AlignCenter, bool showAppIcon = true) {;}
	virtual bool isRequestUserAttentionPresent() const { return false; }
	virtual void requestUserAttention() {;}
	virtual void postSystemNotify(const QImage & icon, const QString & title, const QString & text, const QString & type, int id) {;}
	virtual void showSystemNotificationsSettings() {;}
signals:
	void dockClicked();
	void systemNotificationClicked(int);
};

#endif // WIN7INTEGRATION_H
