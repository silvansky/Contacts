#include "addaccountwidget.h"
#include "ui_addaccountwidget.h"

#include "addsimpleaccountdialog.h"

#include <utils/iconstorage.h>
#include <definitions/resources.h>
#include <definitions/menuicons.h>

AddAccountWidget::AddAccountWidget(AccountWidgetType accWidgetType, QWidget *parent) :
	QWidget(parent),
	ui(new Ui::AddAccountWidget)
{
	ui->setupUi(this);

	_type = accWidgetType;
	_authInfo.authorized = false;

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

	connect(ui->serviceButton, SIGNAL(toggled(bool)), SLOT(onServiceButtonToggled(bool)));
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

void AddAccountWidget::onServiceButtonToggled(bool on)
{
	if (on)
	{
		QStringList domains;
		QString caption;
		QString loginPlaceholder = tr("Login");
		switch (type())
		{
		case AW_Vkontakte:
			caption = tr("Enter your Vkontakte ID and password");
			loginPlaceholder = tr("ID");
			break;
		case AW_ICQ:
			caption = tr("Enter your ICQ UIN or login and password");
			loginPlaceholder = tr("UIN or login");
			break;
		case AW_MRIM:
			caption = tr("Enter your Mail.ru Agent login and password");
			domains <<
				   "mail.ru" <<
				   "list.ru" <<
				   "bk.ru" <<
				   "inbox.ru";

			break;
		case AW_Yandex:
			caption = tr("Enter your Yandex login and password");
			domains <<
				   "yandex.ru" <<
				   "ya.ru" <<
				   "narod.ru";
			break;
		default:
			break;
		}
		if (type() == AW_Facebook)
		{
			// TODO: show facebook auth dialog
		}
		else if (type() == AW_Rambler)
		{
			// TODO: show rambler auth/reg dialog
		}
		else
		{
			AddSimpleAccountDialog *dialog = new AddSimpleAccountDialog;
			dialog->setCaption(caption);
			dialog->setLoginPlaceholder(loginPlaceholder);
			dialog->setDomainList(domains);
			connect(dialog, SIGNAL(accepted()), SLOT(onDialogAccepted()));
			dialog->showDialog();
		}
	}
	ui->successIndicator->setPixmap(on ? style()->standardPixmap(QStyle::SP_DialogApplyButton) : QPixmap());
}

void AddAccountWidget::onDialogAccepted()
{
	AddSimpleAccountDialog *simpleDialog = qobject_cast<AddSimpleAccountDialog *>(sender());
	if (simpleDialog)
	{
		_authInfo.authorized = simpleDialog->succeeded();
		_authInfo.authToken = simpleDialog->authToken();
		_authInfo.user = simpleDialog->selectedUserId();
		if (simpleDialog->succeeded())
			emit authChecked();
		else
			emit authCheckFailed();
		return;
	}
}
