#ifndef STYLESHEETEDITORPLUGIN_H
#define STYLESHEETEDITORPLUGIN_H

#include <interfaces/ipluginmanager.h>
#include <interfaces/imainwindow.h>
#include <utils/action.h>
#include "stylesheeteditor.h"
#ifdef Q_WS_MAC
# include <interfaces/imacintegration.h>
#endif

#define STYLESHEETEDITOR_UUID		"{dcdd7857-af0e-4ccd-bfb4-fb1b7cbbb955}"

class StyleSheetEditorPlugin :
	public QObject,
	public IPlugin
{
	Q_OBJECT
	Q_INTERFACES(IPlugin)
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
protected slots:
	void showEditDialog();
private:
	IPluginManager * pluginManager;
	IMainWindowPlugin * mainWindowPlugin;
#ifdef Q_WS_MAC
	IMacIntegration * macIntegration;
#endif
	StyleSheetEditorDialog * editor;
	Action * showDialog;
protected slots:
	void styleSheetChanged(const QString&);
	void resetStyleSheet();
};

#endif // STYLESHEETEDITORPLUGIN_H
