#include "easyregistrationdialog.h"
#include "ui_easyregistrationdialog.h"

#include <definitions/resources.h>
#include <definitions/stylesheets.h>
#include <definitions/customborder.h>
#include <utils/log.h>
#include <utils/stylestorage.h>
#include <utils/customborderstorage.h>

#include <QWebFrame>
#include <QNetworkRequest>

#ifdef DEBUG_ENABLED
# include <QDebug>
#endif

#define EASY_REG_URL "http://reg.tx.xmpp.rambler.ru/"

EasyRegistrationDialog::EasyRegistrationDialog(QWidget *parent) :
	QWidget(parent),
	ui(new Ui::EasyRegistrationDialog)
{
	ui->setupUi(this);

	// style
	StyleStorage::staticStorage(RSR_STORAGE_STYLESHEETS)->insertAutoStyle(this, STS_OPTIONS_EASYREGISTRATIONDIALOG);

	// custom border
	CustomBorderContainer *border = CustomBorderStorage::staticStorage(RSR_STORAGE_CUSTOMBORDER)->addBorder(this, CBS_DIALOG);
	if (border)
	{
		border->setAttribute(Qt::WA_DeleteOnClose, true);
		border->setMaximizeButtonVisible(false);
		border->setMinimizeButtonVisible(false);
		connect(border, SIGNAL(closeClicked()), SIGNAL(aborted()));
		connect(this, SIGNAL(aborted()), border, SLOT(close()));
		connect(this, SIGNAL(registered(const QString &login)), border, SLOT(close()));
		border->setResizable(false);
	}
	else
	{
		setAttribute(Qt::WA_DeleteOnClose,true);
		ui->caption->setVisible(false);
		layout()->setContentsMargins(0, 0, 0, 0);
	}

	ui->easyRegWebView->page()->mainFrame()->setScrollBarPolicy(Qt::Vertical,Qt::ScrollBarAlwaysOff);
	connect(ui->easyRegWebView, SIGNAL(loadFinished(bool)), SLOT(onLoaded(bool)));

	window()->setWindowModality(Qt::ApplicationModal);

	startLoading();
}

EasyRegistrationDialog::~EasyRegistrationDialog()
{
	delete ui;
}

void EasyRegistrationDialog::closeEvent(QCloseEvent *ce)
{
	if (!userJid.isValid())
	{
		emit aborted();
	}
	QWidget::closeEvent(ce);
}

void EasyRegistrationDialog::startLoading()
{
	QNetworkRequest request(QUrl(EASY_REG_URL));
	request.setRawHeader("Accept-Encoding","identity");
	ui->easyRegWebView->load(request);
}

void EasyRegistrationDialog::onLoaded(bool ok)
{
	if (!ok)
	{
		// set error
	}
	else
	{
		QUrl url = ui->easyRegWebView->url();
#ifdef DEBUG_ENABLED
		qDebug() << "web view loaded! url:" << url;
#endif
		if (url.hasQueryItem("login") && url.hasQueryItem("domain") && url.hasQueryItem("registered"))
		{
			bool registrationOk = url.queryItemValue("registered").toInt() == 1;
			if (registrationOk)
			{
				userJid.setNode(url.queryItemValue("login"));
				userJid.setDomain(url.queryItemValue("domain"));
				emit registered(userJid);
				close();
			}
		}
	}
}

void EasyRegistrationDialog::showEvent(QShowEvent *se)
{
	QWidget::showEvent(se);
}
