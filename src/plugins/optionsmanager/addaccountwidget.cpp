#include "addaccountwidget.h"
#include "ui_addaccountwidget.h"

#include "addsimpleaccountdialog.h"
#include "addfacebookaccountdialog.h"
#include "addrambleraccountdialog.h"

#include <utils/iconstorage.h>
#include <definitions/resources.h>
#include <definitions/menuicons.h>

#ifdef DEBUG_ENABLED
# include <QDebug>
#endif

AddAccountWidget::AddAccountWidget(AccountWidgetType accWidgetType, QWidget *parent) :
	QWidget(parent),
	ui(new Ui::AddAccountWidget)
{
	ui->setupUi(this);

	_type = accWidgetType;
	_authInfo.authorized = false;

	updateServiceName();

	//connect(ui->serviceButton, SIGNAL(toggled(bool)), SLOT(onServiceButtonToggled(bool)));
	connect(ui->serviceButton, SIGNAL(clicked()), SLOT(onServiceButtonClicked()));
	connect(ui->removeButton, SIGNAL(clicked()), SLOT(onRemoveButtonClicked()));
	ui->successIndicator->setVisible(false);
	ui->removeButton->setVisible(false);
}

AddAccountWidget::~AddAccountWidget()
{
	delete ui;
}

ServiceAuthInfo AddAccountWidget::authInfo() const
{
	return _authInfo;
}

QString AddAccountWidget::serviceName() const
{
	return _serviceName;
}

void AddAccountWidget::setServiceName(const QString &newName)
{
	_serviceName = newName;
	ui->serviceButton->setText(_serviceName);
}

QImage AddAccountWidget::serviceIcon() const
{
	return _serviceIcon;
}

void AddAccountWidget::setServiceIcon(const QImage &newIcon)
{
	_serviceIcon = newIcon;
	ui->serviceButton->setIcon(QIcon(QPixmap::fromImage(_serviceIcon)));
}

AccountWidgetType AddAccountWidget::type() const
{
	return _type;
}

void AddAccountWidget::updateServiceName()
{
	if (_authInfo.authorized)
	{
		ui->serviceButton->setText(_authInfo.displayName);
	}
	else
	{
		switch (type())
		{
		case AW_Facebook:
			setServiceName(tr("Facebook"));
			setServiceIcon(IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getImage(MNI_GATEWAYS_SERVICE_FACEBOOK));
			break;
		case AW_Vkontakte:
			setServiceName(tr("Vkontakte"));
			setServiceIcon(IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getImage(MNI_GATEWAYS_SERVICE_VKONTAKTE));
			break;
		case AW_ICQ:
			setServiceName(tr("ICQ"));
			setServiceIcon(IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getImage(MNI_GATEWAYS_SERVICE_ICQ));
			break;
		case AW_MRIM:
			setServiceName(tr("Mail.ru Agent"));
			setServiceIcon(IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getImage(MNI_GATEWAYS_SERVICE_MAGENT));
			break;
		case AW_Yandex:
			setServiceName(tr("Yandex"));
			setServiceIcon(IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getImage(MNI_GATEWAYS_SERVICE_YONLINE));
			break;
		case AW_Rambler:
			setServiceName(tr("Rambler"));
			setServiceIcon(IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getImage(MNI_GATEWAYS_SERVICE_RAMBLER));
			break;
		default:
			setServiceName(tr("Other"));
			setServiceIcon(IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getImage(MNI_GATEWAYS_ADD_ACCOUNT_ERROR));
			break;
		}
	}
}

void AddAccountWidget::onServiceButtonToggled(bool on)
{
	Q_UNUSED(on)
}

void AddAccountWidget::onServiceButtonClicked()
{
	QStringList domains;
	QString caption;
	QString loginPlaceholder = tr("Login");
	QString dialogService;
	switch (type())
	{
	case AW_Vkontakte:
		caption = tr("Enter your Vkontakte ID and password");
		loginPlaceholder = tr("ID");
		dialogService = "vk";
		break;
	case AW_ICQ:
		caption = tr("Enter your ICQ UIN or login and password");
		loginPlaceholder = tr("UIN or login");
		dialogService = "icq";
		break;
	case AW_MRIM:
		caption = tr("Enter your Mail.ru Agent login and password");
		domains <<
			   "mail.ru" <<
			   "list.ru" <<
			   "bk.ru" <<
			   "inbox.ru";
		dialogService = "mrim";

		break;
	case AW_Yandex:
		caption = tr("Enter your Yandex login and password");
		domains <<
			   "yandex.ru" <<
			   "ya.ru" <<
			   "narod.ru";
		dialogService = "ya";
		break;
	default:
		break;
	}
	if (type() == AW_Facebook)
	{
		AddFacebookAccountDialog *dialog = new AddFacebookAccountDialog;
		connect(dialog, SIGNAL(accepted()), SLOT(onDialogAccepted()));
		connect(dialog, SIGNAL(rejected()), SLOT(onDialogRejected()));
		dialog->showDialog();
	}
	else if (type() == AW_Rambler)
	{
		AddRamblerAccountDialog *dialog = new AddRamblerAccountDialog;
		connect(dialog, SIGNAL(accepted()), SLOT(onDialogAccepted()));
		connect(dialog, SIGNAL(rejected()), SLOT(onDialogRejected()));
		dialog->showDialog();
	}
	else
	{
		AddSimpleAccountDialog *dialog = new AddSimpleAccountDialog;
		dialog->setCaption(caption);
		dialog->setLoginPlaceholder(loginPlaceholder);
		dialog->setDomainList(domains);
		dialog->setService(dialogService);
		connect(dialog, SIGNAL(accepted()), SLOT(onDialogAccepted()));
		connect(dialog, SIGNAL(rejected()), SLOT(onDialogRejected()));
		dialog->showDialog();
	}
	ui->removeButton->setVisible(true);
}

void AddAccountWidget::onRemoveButtonClicked()
{
	ui->serviceButton->setEnabled(true);
	ui->removeButton->setVisible(false);
	_authInfo.authorized = false;
	updateServiceName();
	emit authRemoved();
}

void AddAccountWidget::onDialogAccepted()
{
	QDialog *addDialog = qobject_cast<QDialog *>(sender());
	if (addDialog)
	{
		_authInfo.authorized = addDialog->property("succeeded").toBool();
		_authInfo.authToken = addDialog->property("authToken").toString();
		_authInfo.user = addDialog->property("selectedUserId").toString();
		_authInfo.displayName = addDialog->property("selectedUserDisplayName").toString();
		updateServiceName();
		if (_authInfo.authorized)
		{
			ui->serviceButton->setEnabled(false);
			emit authChecked();
		}
		else
		{
			ui->serviceButton->setEnabled(true);
			emit authCheckFailed();
		}
		addDialog->deleteLater();
#ifdef DEBUG_ENABLED
		qDebug() << QString("got account %1 at %2 with token %3").arg(_authInfo.user, _authInfo.service, _authInfo.authToken);
#endif
		return;
	}
}

void AddAccountWidget::onDialogRejected()
{
	QDialog *addDialog = qobject_cast<QDialog *>(sender());
	if (addDialog)
		addDialog->deleteLater();
	ui->serviceButton->setChecked(false);
}
