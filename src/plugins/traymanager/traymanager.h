#ifndef TRAYMANAGER_H
#define TRAYMANAGER_H

#include <QMap>
#include <QTimer>
#include <definations/resources.h>
#include <definations/menuicons.h>
#include <definations/actiongroups.h>
#include <interfaces/ipluginmanager.h>
#include <interfaces/itraymanager.h>
#include <utils/iconstorage.h>

class TrayManager :
			public QObject,
			public IPlugin,
			public ITrayManager
{
	Q_OBJECT;
	Q_INTERFACES(IPlugin ITrayManager);
	Q_PROPERTY(QIcon icon READ icon WRITE setIcon);
public:
	TrayManager();
	~TrayManager();
	virtual QObject *instance() { return this; }
	//IPlugin
	virtual QUuid pluginUuid() const { return TRAYMANAGER_UUID; }
	virtual void pluginInfo(IPluginInfo *APluginInfo);
	virtual bool initConnections(IPluginManager *APluginManager, int &AInitOrder);
	virtual bool initObjects();
	virtual bool initSettings() { return true; }
	virtual bool startPlugin();
	//ITrayManager
	virtual Menu *contextMenu() const;
	virtual QIcon icon() const;
	virtual void setIcon(const QIcon &AIcon);
	virtual QString toolTip() const;
	virtual void setToolTip(const QString &AToolTip);
	virtual int activeNotify() const;
	virtual QList<int> notifies() const;
	virtual ITrayNotify notifyById(int ANotifyId) const;
	virtual int appendNotify(const ITrayNotify &ANotify);
	virtual void removeNotify(int ANotifyId);
	virtual void showMessage(const QString &ATitle, const QString &AMessage,QSystemTrayIcon::MessageIcon AIcon = QSystemTrayIcon::Information, int ATimeout = 10000);
signals:
	void notifyAppended(int ANotifyId);
	void notifyRemoved(int ANotifyId);
	void activeNotifyChanged(int ANotifyId);
	void notifyActivated(int ANotifyId, QSystemTrayIcon::ActivationReason AReason);
	void messageClicked();
	void messageShown(const QString &ATitle, const QString &AMessage,QSystemTrayIcon::MessageIcon AIcon, int ATimeout);
protected:
	void updateTray();
protected slots:
	void onTrayIconActivated(QSystemTrayIcon::ActivationReason AReason);
	void onBlinkTimerTimeout();
	void onTriggerTimerTimeout();
private:
	IPluginManager *FPluginManager;
private:
	Menu *FContextMenu;
private:
	QTimer FBlinkTimer;
	QSystemTrayIcon FSystemIcon;
private:
	bool FIconHidden;
	int FActiveNotify;
	QIcon FIcon;
	QString FToolTip;
	QList<int> FNotifyOrder;
	QTimer FTriggerTimer;
	QMap<int, ITrayNotify> FNotifyItems;
};

#endif // TRAYMANAGER_H
