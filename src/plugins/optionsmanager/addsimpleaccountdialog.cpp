#include "addsimpleaccountdialog.h"
#include "ui_addsimpleaccountdialog.h"

#include <definitions/resources.h>
#include <definitions/customborder.h>
#include <utils/customborderstorage.h>
#include <utils/log.h>

#ifdef Q_WS_MAC
# include <utils/macwidgets.h>
#endif

#include <QTimer>

AddSimpleAccountDialog::AddSimpleAccountDialog() :
	QDialog(NULL),
	ui(new Ui::AddSimpleAccountDialog)
{
	ui->setupUi(this);

	_serverApiHandler = new ServerApiHandler;
	connect(_serverApiHandler, SIGNAL(checkAuthRequestSucceeded(const QString &, const QString &)), SLOT(onCheckAuthRequestSucceeded(const QString &, const QString &)));
	connect(_serverApiHandler, SIGNAL(checkAuthRequestFailed(const QString &, const QString &)), SLOT(onCheckAuthRequestFailed(const QString &, const QString &)));
	connect(_serverApiHandler, SIGNAL(requestFailed(const QString &)), SLOT(onCheckAuthRequestRequestFailed(const QString &)));
	_succeeded = false;

	connect(ui->acceptButton, SIGNAL(clicked()), SLOT(onAcceptClicked()));
	connect(ui->rejectButton, SIGNAL(clicked()), SLOT(reject()));

	ui->login->setAttribute(Qt::WA_MacShowFocusRect, false);
	ui->password->setAttribute(Qt::WA_MacShowFocusRect, false);

#ifdef Q_WS_MAC
	ui->buttonsLayout->setSpacing(16);
	ui->buttonsLayout->addWidget(ui->acceptButton);
#endif

	domainsMenu = NULL;

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

AddSimpleAccountDialog::~AddSimpleAccountDialog()
{
	delete _serverApiHandler;
	delete ui;
}

void AddSimpleAccountDialog::showDialog()
{
	window()->setWindowModality(Qt::ApplicationModal);
	window()->show();
	QTimer::singleShot(10, this, SLOT(onAdjustWindowSize()));
}

QString AddSimpleAccountDialog::service() const
{
	return _service;
}

void AddSimpleAccountDialog::setService(const QString &newService)
{
	_service = newService;
}

QString AddSimpleAccountDialog::caption() const
{
	return _caption;
}

void AddSimpleAccountDialog::setCaption(const QString &newCaption)
{
	_caption = newCaption;
	ui->caption->setText(_caption);
}

QImage AddSimpleAccountDialog::icon() const
{
	return _icon;
}

void AddSimpleAccountDialog::setIcon(const QImage &newIcon)
{
	_icon = newIcon;
}

QString AddSimpleAccountDialog::loginPlaceholder() const
{
	return _loginPlaceholder;
}

void AddSimpleAccountDialog::setLoginPlaceholder(const QString &newPlaceholder)
{
	_loginPlaceholder = newPlaceholder;
	ui->login->setPlaceholderText(_loginPlaceholder);
}

QString AddSimpleAccountDialog::passwordPlaceholder() const
{
	return _passwordPlaceholder;
}

void AddSimpleAccountDialog::setPasswordPlaceholder(const QString &newPlaceholder)
{
	_passwordPlaceholder = newPlaceholder;
	ui->password->setPlaceholderText(_passwordPlaceholder);
}

QStringList AddSimpleAccountDialog::domainList() const
{
	return _domainList;
}

void AddSimpleAccountDialog::setDomainList(const QStringList &newList)
{
	_domainList = newList;
	int c = _domainList.count();
	if (c)
	{
		if (c == 1)
		{
			ui->singleDomain->setVisible(true);
			ui->domains->setVisible(false);
			ui->singleDomain->setText("@" + _domainList.at(0));
		}
		else
		{
			ui->singleDomain->setVisible(false);
			ui->domains->setVisible(true);
			if (domainsMenu)
				domainsMenu->deleteLater();
			domainsMenu = new Menu;
			Action *firstAction = NULL;
			foreach (QString domain, _domainList)
			{
				Action *domainAction = new Action(domainsMenu);
				domainAction->setText("@" + domain);
				domainAction->setData(Action::DR_UserDefined + 1, domain);
				connect(domainAction, SIGNAL(triggered()), SLOT(onDomainActionTriggered()));
				domainsMenu->addAction(domainAction);
				if (!firstAction)
					firstAction = domainAction;
			}
			ui->domains->setMenu(domainsMenu);
			firstAction->trigger();
		}
	}
	else
	{
		ui->domains->setVisible(false);
		ui->singleDomain->setVisible(false);
	}
}

bool AddSimpleAccountDialog::succeeded() const
{
	return _succeeded;
}

QString AddSimpleAccountDialog::selectedUserId() const
{
	return _selectedUserId;
}

QString AddSimpleAccountDialog::selectedUserDisplayName() const
{
	return _selectedUserDisplayName;
}

QString AddSimpleAccountDialog::authToken() const
{
	return _authToken;
}

void AddSimpleAccountDialog::onDomainActionTriggered()
{
	Action *a = qobject_cast<Action *>(sender());
	if (a)
	{
		ui->domains->setText(a->text());
		ui->domains->setProperty("selectedDomain", a->data(Action::DR_UserDefined + 1).toString());
	}
}

void AddSimpleAccountDialog::onAdjustWindowSize()
{
	window()->adjustSize();
}

void AddSimpleAccountDialog::onAcceptClicked()
{
	QString user = ui->login->text();
	if (ui->domains->isVisible())
		user += ui->domains->text();
	else if (ui->singleDomain->isVisible())
		user += ui->singleDomain->text();
	_serverApiHandler->sendCheckAuthRequest(service(), user, ui->password->text());
}

void AddSimpleAccountDialog::onCheckAuthRequestSucceeded(const QString &user, const QString &displayName, const QString &authToken_)
{
	_selectedUserId = user;
	_selectedUserDisplayName = displayName;
	_authToken = authToken_;
	_succeeded = true;
	accept();
}

void AddSimpleAccountDialog::onCheckAuthRequestFailed(const QString &user, const QString &reason)
{
	LogError(QString("[AddSimpleAccountDialog]: Check auth request failed for %1: %2").arg(user, reason));
}

void AddSimpleAccountDialog::onCheckAuthRequestRequestFailed(const QString &error)
{
	LogError(QString("[AddSimpleAccountDialog]: Check auth request failed: %1").arg(error));
}
