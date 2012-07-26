#include "addrambleraccountdialog.h"
#include "ui_addrambleraccountdialog.h"

#include <definitions/resources.h>
#include <definitions/customborder.h>
#include <definitions/stylesheets.h>
#include <definitions/graphicseffects.h>
#include <utils/stylestorage.h>
#include <utils/graphicseffectsstorage.h>
#include <utils/customborderstorage.h>
#include <utils/log.h>

#ifdef Q_WS_MAC
# include <utils/macwidgets.h>
#endif

#include <QTimer>

AddRamblerAccountDialog::AddRamblerAccountDialog(QWidget *parent) :
	QDialog(parent),
	ui(new Ui::AddRamblerAccountDialog)
{
	ui->setupUi(this);

	StyleStorage::staticStorage(RSR_STORAGE_STYLESHEETS)->insertAutoStyle(this,STS_OPTIONS_LOGINDIALOG);
	GraphicsEffectsStorage::staticStorage(RSR_STORAGE_GRAPHICSEFFECTS)->installGraphicsEffect(this, GFX_LABELS);

	_serverApiHandler = new ServerApiHandler;
	connect(_serverApiHandler, SIGNAL(checkAuthRequestSucceeded(const QString &, const QString &, const QString &)), SLOT(onCheckAuthRequestSucceeded(const QString &, const QString &, const QString &)));
	connect(_serverApiHandler, SIGNAL(checkAuthRequestFailed(const QString &, const QString &)), SLOT(onCheckAuthRequestFailed(const QString &, const QString &)));
	connect(_serverApiHandler, SIGNAL(checkAuthRequestRequestFailed(const QString &)), SLOT(onCheckAuthRequestRequestFailed(const QString &)));
	_succeeded = false;

	CustomBorderContainer *border = CustomBorderStorage::staticStorage(RSR_STORAGE_CUSTOMBORDER)->addBorder(this, CBS_DIALOG);
	if (border)
	{
		border->setMaximizeButtonVisible(false);
		border->setMinimizeButtonVisible(false);
		border->setResizable(false);
		connect(border, SIGNAL(closeClicked()), SLOT(reject()));
		connect(this, SIGNAL(accepted()), border, SLOT(closeWidget()));
		connect(this, SIGNAL(rejected()), border, SLOT(closeWidget()));
	}
	else
	{
		setFixedSize(sizeHint());
#ifdef Q_WS_MAC
		setWindowGrowButtonEnabled(this, false);
#endif
	}
}

AddRamblerAccountDialog::~AddRamblerAccountDialog()
{
	delete ui;
}

void AddRamblerAccountDialog::showDialog()
{
	window()->setWindowModality(Qt::ApplicationModal);
	window()->show();
	QTimer::singleShot(10, this, SLOT(onAdjustWindowSize()));
}

bool AddRamblerAccountDialog::succeeded() const
{
	return _succeeded;
}

QString AddRamblerAccountDialog::selectedUserId() const
{
	return _selectedUserId;
}

QString AddRamblerAccountDialog::selectedUserDisplayName() const
{
	return _selectedUserDisplayName;
}

QString AddRamblerAccountDialog::authToken() const
{
	return _authToken;
}

void AddRamblerAccountDialog::onAdjustWindowSize()
{
	window()->adjustSize();
}

void AddRamblerAccountDialog::onRegistrationSucceeded(const Jid &user)
{
}

void AddRamblerAccountDialog::onRegistrationFailed(const QString &reason, const QString &loginError, const QString &passwordError, const QStringList &suggests)
{
}

void AddRamblerAccountDialog::onCheckAuthRequestSucceeded(const QString &user, const QString &displayName, const QString &authToken_)
{
}

void AddRamblerAccountDialog::onCheckAuthRequestFailed(const QString &user, const QString &reason)
{
}

void AddRamblerAccountDialog::onCheckAuthRequestRequestFailed(const QString &error)
{
}
