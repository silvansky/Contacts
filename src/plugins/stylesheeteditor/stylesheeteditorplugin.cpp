#include "stylesheeteditorplugin.h"
#include "stylesheeteditor.h"
#include <utils/filestorage.h>
#include <definations/stylesheets.h>
#include <definations/resources.h>
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
	pluginManager = APluginManager;
	return (pluginManager);
}

bool StyleSheetEditorPlugin::initObjects()
{
	editor = new StyleSheetEditorDialog(0);
	connect(editor, SIGNAL(styleSheetChanged(const QString&)), SLOT(styleSheetChanged(const QString&)));
	connect(editor, SIGNAL(resetStyleSheet()), SLOT(resetStyleSheet()));
	return true;
}

bool StyleSheetEditorPlugin::startPlugin()
{
	resetStyleSheet();
	editor->show();
	return true;
}

void StyleSheetEditorPlugin::styleSheetChanged(const QString& newSheet)
{
	if (pluginManager)
		pluginManager->setStyleSheet(newSheet);
}

void StyleSheetEditorPlugin::resetStyleSheet()
{
	if (pluginManager)
		editor->setText(pluginManager->styleSheet());
}

Q_EXPORT_PLUGIN2(plg_stylesheeteditor, StyleSheetEditorPlugin)
