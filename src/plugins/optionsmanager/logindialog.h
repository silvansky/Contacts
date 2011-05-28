#ifndef LOGINDIALOG_H
#define LOGINDIALOG_H

#include <QUuid>
#include <QEvent>
#include <QTimer>
#include <QDialog>
#include <definitions/version.h>
#include <definitions/resources.h>
#include <definitions/menuicons.h>
#include <definitions/stylesheets.h>
#include <definitions/optionvalues.h>
#include <definitions/optionnodes.h>
#include <definitions/optionwidgetorders.h>
#include <interfaces/ipluginmanager.h>
#include <interfaces/ioptionsmanager.h>
#include <interfaces/iaccountmanager.h>
#include <interfaces/ipresence.h>
#include <interfaces/istatuschanger.h>
#include <interfaces/ixmppstreams.h>
#include <interfaces/iconnectionmanager.h>
#include <interfaces/imainwindow.h>
#include <interfaces/iconnectionmanager.h>
#include <interfaces/idefaultconnection.h>
#include <interfaces/itraymanager.h>
#include <interfaces/inotifications.h>
#include <utils/options.h>
#include <utils/balloontip.h>
#include <utils/iconstorage.h>
#include <utils/stylestorage.h>
#include <utils/widgetmanager.h>
#include "ui_logindialog.h"

class LoginDialog :
	public QDialog
{
	Q_OBJECT
public:
	LoginDialog(IPluginManager *APluginManager, QWidget *AParent = NULL);
	~LoginDialog();
public:
	void loadLastProfile();
	void connectIfReady();
	Jid currentStreamJid() const;
public slots:
	virtual void reject();
protected:
	virtual void showEvent(QShowEvent *AEvent);
	virtual void keyPressEvent(QKeyEvent *AEvent);
	virtual bool eventFilter(QObject *AWatched, QEvent *AEvent);
	void moveEvent(QMoveEvent *);
	void mousePressEvent(QMouseEvent *);
protected:
	void initialize(IPluginManager *APluginManager);
	bool isCapsLockOn() const;
	void closeCurrentProfile();
	bool tryNextConnectionSettings();
	void setConnectEnabled(bool AEnabled);
	void stopReconnection();
	void showConnectionSettings();
	void hideConnectionError();
	void showConnectionError(const QString &ACaption, const QString &AError);
	void hideXmppStreamError();
	void showXmppStreamError(const QString &ACaption, const QString &AError, const QString &AHint, bool showPasswordEnabled = true);
	void saveCurrentProfileSettings();
	void loadCurrentProfileSettings();
	bool readyToConnect() const;
protected slots:
	void onConnectClicked();
	void onXmppStreamOpened();
	void onXmppStreamClosed();
	void onReconnectTimerTimeout();
	void onCompleterHighLighted(const QString &AText);
	void onCompleterActivated(const QString &AText);
	void onDomainCurrentIntexChanged(int AIndex);
	void onNewDomainSelected(const QString & newDomain);
	void onNewDomainRejected();
	void onLabelLinkActivated(const QString &ALink);
	void onLoginOrPasswordTextChanged();
	void onShowConnectingAnimation();
	void onAdjustDialogSize();
	void onNotificationAppend(int ANotifyId, INotification &ANotification);
	void onNotificationAppended(int ANotifyId, const INotification &ANotification);
	void onTrayNotifyActivated(int ANotifyId, QSystemTrayIcon::ActivationReason AReason);
	void onShowPasswordToggled(int state);
	void onStylePreviewReset();
private:
	Ui::LoginDialogClass ui;
private:
	IAccountManager *FAccountManager;
	IOptionsManager *FOptionsManager;
	IStatusChanger *FStatusChanger;
	IMainWindowPlugin *FMainWindowPlugin;
	IConnectionManager *FConnectionManager;
	INotifications *FNotifications;
	ITrayManager *FTrayManager;
private:
	bool FNewProfile;
	bool FFirstConnect;
	bool FMainWindowVisible;
	bool FSavedPasswordCleared;
	QUuid FAccountId;
	QTimer FReconnectTimer;
	int FConnectionSettings;
	int domainPrevIndex;
	QWidget *FConnectionErrorWidget;
};

#endif // LOGINDIALOG_H
