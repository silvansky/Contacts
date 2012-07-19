#ifndef LOGINDIALOG_H
#define LOGINDIALOG_H

#include "ui_logindialog.h"
#include "serverapihandler.h"
#include "addaccountwidget.h"

#include <definitions/version.h>
#include <definitions/resources.h>
#include <definitions/menuicons.h>
#include <definitions/optionnodes.h>
#include <definitions/stylesheets.h>
#include <definitions/optionvalues.h>
#include <definitions/optionwidgetorders.h>

#include <interfaces/iavatars.h>
#include <interfaces/ipresence.h>
#include <interfaces/imainwindow.h>
#include <interfaces/ixmppstreams.h>
#include <interfaces/itraymanager.h>
#include <interfaces/istatuschanger.h>
#include <interfaces/ipluginmanager.h>
#include <interfaces/inotifications.h>
#include <interfaces/ioptionsmanager.h>
#include <interfaces/iaccountmanager.h>
#include <interfaces/iconnectionmanager.h>
#include <interfaces/iconnectionmanager.h>
#include <interfaces/idefaultconnection.h>

#include <utils/menu.h>
#include <utils/options.h>
#include <utils/balloontip.h>
#include <utils/iconstorage.h>
#include <utils/stylestorage.h>
#include <utils/widgetmanager.h>

#include <QUuid>
#include <QEvent>
#include <QTimer>
#include <QDialog>

class LoginDialog :
	public QDialog
{
	Q_OBJECT
private:
	enum Mode
	{
		LogIn,
		Registration,
		SelectProfile,
		CreateAccounts
	};

	enum RegistrationState
	{
		NotStarted,
		RequestSent,
		Success
	};

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
	bool event(QEvent *AEvent);
	void showEvent(QShowEvent *AEvent);
	void keyPressEvent(QKeyEvent *AEvent);
	bool eventFilter(QObject *AWatched, QEvent *AEvent);
protected:
	void initialize(IPluginManager *APluginManager);
	bool isCapsLockOn() const;
	void showCapsLockBalloon(const QPoint &APoint);
	void showErrorBalloon();
	void hideErrorBallon();
	void closeCurrentProfile();
	bool tryNextConnectionSettings();
	void setControlsEnabled(bool AEnabled);
	void setRegControlsEnabled(bool AEnabled);
	void setConnectEnabled(bool AEnabled);
	void stopReconnection();
	void showConnectionSettings();
	// connection errors
	void hideConnectionError();
	void showConnectionError(const QString &ACaption, const QString &AError);
	void hideXmppStreamError();
	void showXmppStreamError(const QString &ACaption, const QString &AError, const QString &AHint, bool showPasswordEnabled = true);
	// reg errors
	void showRegConnectionError(const QString &text = QString::null);
	void showRegLoginError(const QString &text);
	void showRegPasswordError(const QString &text);
	void showRegConfirmPasswordError(const QString &text);
	// ...
	void saveCurrentProfileSettings();
	void loadCurrentProfileSettings();
	bool readyToConnect() const;
	bool readyToRegister() const;
	// validators
	int checkPassword(const QString &password) const;
	int checkLogin(const QString &login) const;
protected slots:
	// old easyreg dialog
	void askUserIfHeHasAccount();
	void showEasyRegDialog();
	void onAskDialogRejected();

	// connect / register buttons
	void onConnectClicked();
	void onRegisterClicked();

	// abort timer
	void onAbortTimerTimeout();

	// xmpp stream
	void onXmppStreamOpened();
	void onXmppStreamClosed();

	// reconnect timer
	void onReconnectTimerTimeout();

	// completers
	void onCompleterHighLighted(const QString &AText);
	void onCompleterActivated(const QString &AText);
	void onSuggestCompleterActivated(const QString &AText);

	// domain menu
	void onDomainCurrentIntexChanged(int AIndex);
	void onDomainActionTriggered();

	// custom domain
	void onNewDomainSelected(const QString &newDomain);
	void onNewDomainRejected();

	// handling links
	void onLabelLinkActivated(const QString &ALink);

	// login or reg data changed - verify it
	void onLoginOrPasswordTextChanged();
	void onRegistrationDataChanged();

	// cancel btn
	void onShowCancelButton();

	// adjust size
	void onAdjustDialogSize();

	// notifications
	void onNotificationAppend(int ANotifyId, INotification &ANotification);
	void onNotificationAppended(int ANotifyId, const INotification &ANotification);
	void onTrayNotifyActivated(int ANotifyId, QSystemTrayIcon::ActivationReason AReason);

	// show pass check
	void onShowPasswordToggled(int state);

	// style reset
	void onStylePreviewReset();

	// old easyreg dlg
	void onEasyRegDialogAborted();
	void onEasyRegDialogRegistered(const Jid &user);

	// validation timer
	void onRegDataVerifyTimer();
	void startRegDataVerifyTimer();
	void stopRegDataVerifyTimer();

	// rotation timer
	void onRotateTimer();
	void startLoadAnimation();
	void stopLoadAnimation();
protected slots:
	// for http requests
	void onRequestFinished(const QUrl &url, const QString &result);
	void onRequestFailed(const QUrl &url, const QString &error);
private:
	Mode mode() const;
	void setMode(Mode newMode);
	RegistrationState regState() const;
	void setRegState(RegistrationState state);
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
	IAvatars *FAvatars;
private:
	Mode mode_;
	RegistrationState regState_;
private:
	bool FNewProfile;
	bool FFirstConnect;
	bool FMainWindowVisible;
	bool FSavedPasswordCleared;
	int FDomainPrevIndex;
	int FConnectionSettings;
	int FActiveErrorType;
	QUuid FAccountId;
	Menu *FDomainsMenu;
	Menu *FRegDomainsMenu;
	QTimer FAbortTimer;
	QTimer FReconnectTimer;
	QTimer FRegDataVerifyTimer;
	QTimer FRotateTimer;
	QWidget *FConnectionErrorWidget;
	ServerApiHandler *serverApiHandler;
	QList<AddAccountWidget *> addAccountWidgets;
};

#endif // LOGINDIALOG_H
