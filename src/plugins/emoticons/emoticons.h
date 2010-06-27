#ifndef EMOTICONS_H
#define EMOTICONS_H

#include <QMap>
#include <QHash>
#include <QStringList>
#include <definations/actiongroups.h>
#include <definations/toolbargroups.h>
#include <definations/messagewriterorders.h>
#include <definations/resourceloaderorders.h>
#include <definations/optionvalues.h>
#include <definations/optionnodes.h>
#include <definations/optionnodeorders.h>
#include <definations/optionwidgetorders.h>
#include <definations/menuicons.h>
#include <interfaces/ipluginmanager.h>
#include <interfaces/iemoticons.h>
#include <interfaces/imessageprocessor.h>
#include <interfaces/imessagewidgets.h>
#include <interfaces/ioptionsmanager.h>
#include <utils/iconstorage.h>
#include <utils/options.h>
#include <utils/menu.h>
#include "selecticonmenu.h"
#include "emoticonsoptions.h"

class EmoticonsContainer;

class Emoticons :
			public QObject,
			public IPlugin,
			public IEmoticons,
			public IMessageWriter,
			public IOptionsHolder
{
	Q_OBJECT;
	Q_INTERFACES(IPlugin IEmoticons IMessageWriter IOptionsHolder);
public:
	Emoticons();
	~Emoticons();
	//IPlugin
	virtual QObject *instance() { return this; }
	virtual QUuid pluginUuid() const { return EMOTICONS_UUID; }
	virtual void pluginInfo(IPluginInfo *APluginInfo);
	virtual bool initConnections(IPluginManager *APluginManager, int &AInitOrder);
	virtual bool initObjects();
	virtual bool initSettings();
	virtual bool startPlugin() { return true; }
	//IMessageWriter
	virtual void writeMessage(int AOrder, Message &AMessage, QTextDocument *ADocument, const QString &ALang);
	virtual void writeText(int AOrder, Message &AMessage, QTextDocument *ADocument, const QString &ALang);
	//IOptionsHolder
	virtual QMultiMap<int, IOptionsWidget *> optionsWidgets(const QString &ANodeId, QWidget *AParent);
	//IEmoticons
	virtual QList<QString> activeIconsets() const;
	virtual QUrl urlByKey(const QString &AKey) const;
	virtual QString keyByUrl(const QUrl &AUrl) const;
protected:
	void createIconsetUrls();
	void replaceTextToImage(QTextDocument *ADocument) const;
	void replaceImageToText(QTextDocument *ADocument) const;
	SelectIconMenu *createSelectIconMenu(const QString &ASubStorage, QWidget *AParent);
	void insertSelectIconMenu(const QString &ASubStorage);
	void removeSelectIconMenu(const QString &ASubStorage);
protected slots:
	void onEditWidgetCreated(IEditWidget *AEditWidget);
	void onEditWidgetContentsChanged(int APosition, int ARemoved, int AAdded);
	void onEmoticonsContainerDestroyed(QObject *AObject);
	void onSelectIconMenuDestroyed(QObject *AObject);
	void onIconSelected(const QString &ASubStorage, const QString &AIconKey);
	void onOptionsOpened();
	void onOptionsChanged(const OptionsNode &ANode);
private:
	IMessageWidgets *FMessageWidgets;
	IMessageProcessor *FMessageProcessor;
	IOptionsManager *FOptionsManager;
private:
	QHash<QString, QUrl> FUrlByKey;
	QMap<QString, IconStorage *> FStorages;
	QList<EmoticonsContainer *> FContainers;
	QMap<SelectIconMenu *, EmoticonsContainer *> FContainerByMenu;
};

#endif // EMOTICONS_H
