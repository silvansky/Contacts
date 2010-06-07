#ifndef STYLESHEETEDITORPLUGIN_H
#define STYLESHEETEDITORPLUGIN_H

#include <interfaces/ipluginmanager.h>
#include <interfaces/istylesheeteditor.h>
#include "stylesheeteditor.h"

class StyleSheetEditorPlugin :
		public QObject,
		public IPlugin,
		public IStyleSheetEditorPlugin
{
	Q_OBJECT;
	Q_INTERFACES(IPlugin IStyleSheetEditorPlugin);
public:
	StyleSheetEditorPlugin();
	~StyleSheetEditorPlugin();
	// IPlugin
	virtual QObject* instance() { return this; }
	virtual QUuid pluginUuid() const { return STYLESHEETEDITOR_UUID; }
	virtual void pluginInfo(IPluginInfo *APluginInfo);
	virtual bool initConnections(IPluginManager *APluginManager, int &AInitOrder);
	virtual bool initObjects();
	virtual bool initSettings() { return true; }
	virtual bool startPlugin();
private:
	IPluginManager * pluginManager;
	qdesigner_internal::StyleSheetEditorDialog * editor;
protected slots:
	void styleSheetChanged(const QString&);
	void resetStyleSheet();
};

#endif // STYLESHEETEDITORPLUGIN_H
