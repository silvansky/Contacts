#include "stylesheeteditorplugin.h"
#include "stylesheeteditor.h"
#include <utils/filestorage.h>
#include <definitions/stylesheets.h>
#include <definitions/resources.h>
#include <QApplication>

StyleSheetEditorPlugin::StyleSheetEditorPlugin()
{
	pluginManager = 0;
}

StyleSheetEditorPlugin::~StyleSheetEditorPlugin()
{
	delete editor;
}

void StyleSheetEditorPlugin::pluginInfo(IPluginInfo *APluginInfo)
{
	APluginInfo->name = tr("Style sheet editor");
	APluginInfo->description = tr("Allows to edit and preview the application stylesheet");
	APluginInfo->version = "1.0";
	APluginInfo->author = "V.Gorshkov";
	APluginInfo->homePage = "http://virtus.rambler.ru";
}

bool StyleSheetEditorPlugin::initConnections(IPluginManager *APluginManager, int &AInitOrder)
{
	Q_UNUSED(AInitOrder);
	pluginManager = APluginManager;
	IPlugin * plugin = APluginManager->pluginInterface("IMainWindowPlugin").value(0,NULL);
	if (plugin)
	{
		mainWindowPlugin = qobject_cast<IMainWindowPlugin *>(plugin->instance());
	}
	return (pluginManager);
}

bool StyleSheetEditorPlugin::initObjects()
{
	editor = new StyleSheetEditorDialog(0);
	connect(editor, SIGNAL(styleSheetChanged(const QString&)), SLOT(styleSheetChanged(const QString&)));
	connect(editor, SIGNAL(resetStyleSheet()), SLOT(resetStyleSheet()));
	showDialog = new Action;
	showDialog->setText(tr("Show Style Sheet Editor"));
	connect(showDialog, SIGNAL(triggered()), SLOT(showEditDialog()));
	if (mainWindowPlugin)
		mainWindowPlugin->mainWindow()->mainMenu()->addAction(showDialog);
	return true;
}

bool StyleSheetEditorPlugin::startPlugin()
{
	resetStyleSheet();
	return true;
}

void StyleSheetEditorPlugin::showEditDialog()
{
	editor->show();
}

void StyleSheetEditorPlugin::styleSheetChanged(const QString& newSheet)
{
	Q_UNUSED(newSheet)
}

void StyleSheetEditorPlugin::resetStyleSheet()
{
}

Q_EXPORT_PLUGIN2(plg_stylesheeteditor, StyleSheetEditorPlugin)
