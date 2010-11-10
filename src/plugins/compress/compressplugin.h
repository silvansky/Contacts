#ifndef COMPRESSPLUGIN_H
#define COMPRESSPLUGIN_H

#include <QObject>
#include <QObjectCleanupHandler>
#include <definitions/namespaces.h>
#include <definitions/xmppfeatureorders.h>
#include <interfaces/ipluginmanager.h>
#include <interfaces/ixmppstreams.h>
#include "compression.h"

#define COMPRESS_UUID "{e34791d9-be7a-4ab1-917f-6897d11a5116}"

class CompressPlugin :
			public QObject,
			public IPlugin,
			public IXmppFeaturesPlugin
{
	Q_OBJECT;
	Q_INTERFACES(IPlugin IXmppFeaturesPlugin);
public:
	CompressPlugin();
	~CompressPlugin();
	//IPlugin
	virtual QObject *instance() { return this; }
	virtual QUuid pluginUuid() const { return COMPRESS_UUID; }
	virtual void pluginInfo(IPluginInfo *APluginInfo);
	virtual bool initConnections(IPluginManager *APluginManager, int &AInitOrder);
	virtual bool initObjects();
	virtual bool initSettings() { return true; }
	virtual bool startPlugin() { return true; }
	//IXmppFeaturesPlugin
	virtual QList<QString> xmppFeatures() const { return QList<QString>() << NS_FEATURE_COMPRESS; }
	virtual IXmppFeature *newXmppFeature(const QString &AFeatureNS, IXmppStream *AXmppStream);
signals:
	void featureCreated(IXmppFeature *AFeature);
	void featureDestroyed(IXmppFeature *AFeature);
protected slots:
	void onFeatureDestroyed();
private:
	IXmppStreams *FXmppStreams;
};

#endif // COMPRESSPLUGIN_H
