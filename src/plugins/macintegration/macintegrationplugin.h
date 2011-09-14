#ifndef MACINTEGRATIONPLUGIN_H
#define MACINTEGRATIONPLUGIN_H

#include <QObject>
#include <interfaces/ipluginmanager.h>
#include <interfaces/imacintegration.h>

#define MACINTEGRATION_UUID "{c936122e-cbf3-442d-b01f-8ecc5b0ad530}"

class MacIntegrationPrivate;

class MacIntegrationPlugin :
		public QObject,
		public IPlugin,
		public IMacIntegration
{
	Q_OBJECT
	Q_INTERFACES(IPlugin IMacIntegration)
public:
	explicit MacIntegrationPlugin();
	//IPlugin
	virtual QObject *instance() { return this; }
	virtual QUuid pluginUuid() const { return MACINTEGRATION_UUID; }
	virtual void pluginInfo(IPluginInfo *APluginInfo);
	virtual bool initConnections(IPluginManager *APluginManager, int &AInitOrder);
	virtual bool initObjects();
	virtual bool initSettings() { return true; }
	virtual bool startPlugin() { return true; }
	//IMacIntegration
	Menu * dockMenu();
	QMenuBar * menuBar();
signals:
	void dockClicked();

public slots:
private:
	Menu * _dockMenu;
	QMenuBar * _menuBar;
	MacIntegrationPrivate * p;

};

#endif // MACINTEGRATIONPLUGIN_H
