#ifndef MACINTEGRATIONPLUGIN_H
#define MACINTEGRATIONPLUGIN_H

#include <QObject>
#include <interfaces/ipluginmanager.h>
#include <interfaces/imacintegration.h>

#define MACINTEGRATION_UUID "{c936122e-cbf3-442d-b01f-8ecc5b0ad530}"

class MacIntegrationPrivate;
class ITabWindow;

class MacIntegrationPlugin :
		public QObject,
		public IPlugin,
		public IMacIntegration
{
	Q_OBJECT
	Q_INTERFACES(IPlugin IMacIntegration)
public:
	MacIntegrationPlugin();
	~MacIntegrationPlugin();
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
	Menu * fileMenu();
	Menu * editMenu();
	Menu * contactsMenu();
	Menu * windowMenu();
	void setDockBadge(const QString & badgeText);
	void postGrowlNotify(const QImage & icon, const QString & title, const QString & text, const QString & type, int id);
	void showGrowlPreferencePane();
	void setCustomBorderColor(const QColor & color);
	void setCustomTitleColor(const QColor & color);
	void setWindowMovableByBackground(QWidget * window, bool movable);
signals:
	void dockClicked();
	void growlNotifyClicked(int);
private:
	void initMenus();
	void updateActions();
private slots:
	void onFocusChanged(QWidget * old, QWidget * now);
	void onMinimizeAction();
	void onCloseAction();
	void onCopyAction();
	void onPasteAction();
	void onCutAction();
	void onUndoAction();
	void onRedoAction();
	void onSelectAllAction();
	// slots for handling selection change events
	void onSelectionChanged();
	void onTextChanged();
	void onCopyAvailableChange(bool);
private:
	QWidget * lastFocusedWidget;
	Menu * _dockMenu;
	QMenuBar * _menuBar;
	Menu * _fileMenu;
	Menu * _editMenu;
	Menu * _contactsMenu;
	Menu * _windowMenu;
	MacIntegrationPrivate * p;
	// actions
	Action * copyAction, * cutAction, * pasteAction, * undoAction, * redoAction, * selectallAction;
	Action * closeAction;

};

#endif // MACINTEGRATIONPLUGIN_H
