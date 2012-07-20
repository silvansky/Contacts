#include "addfacebookaccountdialog.h"
#include "ui_addfacebookaccountdialog.h"

#include <definitions/resources.h>
#include <definitions/stylesheets.h>
#include <definitions/customborder.h>
#include <utils/log.h>
#include <utils/stylestorage.h>
#include <utils/custominputdialog.h>
#include <utils/customborderstorage.h>

#include <QWebFrame>
#include <QWebHistory>
#include <QTextDocument>
#include <QNetworkRequest>
#include <QDesktopServices>

#ifdef DEBUG_ENABLED
# include <QDebug>
#endif

#define FB_AUTH_HOST "fb.tx.contacts.rambler.ru"
#define FB_AUTH_URL  "http://"FB_AUTH_HOST"/auth"


AddFacebookAccountDialog::AddFacebookAccountDialog(QWidget *parent) :
	QDialog(parent),
	ui(new Ui::AddFacebookAccountDialog)
{
	ui->setupUi(this);

	_succeeded = false;

	StyleStorage::staticStorage(RSR_STORAGE_STYLESHEETS)->insertAutoStyle(this,STS_GATEWAYS_ADDFACEBOOKACCOUNTDIALOG);

	setFixedSize(500, 500);

	CustomBorderContainer *border = CustomBorderStorage::staticStorage(RSR_STORAGE_CUSTOMBORDER)->addBorder(this, CBS_DIALOG);
	if (border)
	{
		border->setAttribute(Qt::WA_DeleteOnClose, true);
		border->setMaximizeButtonVisible(false);
		border->setMinimizeButtonVisible(false);
		connect(border, SIGNAL(closeClicked()), SLOT(reject()));
		connect(this, SIGNAL(rejected()), border, SLOT(close()));
		connect(this, SIGNAL(accepted()), border, SLOT(close()));
		border->setResizable(false);

	}
	else
	{
		setAttribute(Qt::WA_DeleteOnClose,true);
		ui->caption->setVisible(false);
		layout()->setContentsMargins(0, 0, 0, 0);
	}

	ui->webView->page()->setLinkDelegationPolicy(QWebPage::DelegateAllLinks);
	ui->webView->page()->mainFrame()->setScrollBarPolicy(Qt::Vertical,Qt::ScrollBarAlwaysOff);
	connect(ui->webView, SIGNAL(loadStarted()), SLOT(onWebViewLoadStarted()));
	connect(ui->webView, SIGNAL(loadFinished(bool)), SLOT(onWebViewLoadFinished(bool)));
	connect(ui->webView->page(),SIGNAL(linkClicked(const QUrl &)), SLOT(onWebPageLinkClicked(const QUrl &)));

	setWaitMode(true, tr("Waiting for host response..."));

	QNetworkRequest request(QUrl(FB_AUTH_URL));
	request.setRawHeader("Accept-Encoding", "identity");
	ui->webView->load(request);
}

AddFacebookAccountDialog::~AddFacebookAccountDialog()
{
	delete ui;
}

void AddFacebookAccountDialog::showDialog()
{
	window()->setWindowModality(Qt::ApplicationModal);
	window()->show();
	QTimer::singleShot(10, this, SLOT(onAdjustWindowSize()));
}

bool AddFacebookAccountDialog::succeeded() const
{
	return _succeeded;
}

QString AddFacebookAccountDialog::selectedUserId() const
{
	return _selectedUserId;
}

QString AddFacebookAccountDialog::authToken() const
{
	return _authToken;
}

void AddFacebookAccountDialog::abort(const QString &message)
{
	CustomInputDialog *dialog = new CustomInputDialog(CustomInputDialog::Info);
	dialog->setCaptionText(tr("Error"));
	dialog->setInfoText(message);
	dialog->setAcceptButtonText(tr("Ok"));
	dialog->setDeleteOnClose(true);
	dialog->show();
	QTimer::singleShot(0, this, SLOT(reject()));
	hide();
}

void AddFacebookAccountDialog::setWaitMode(bool wait, const QString &message)
{
	ui->caption->setText(tr("Facebook authorization"));
	if (wait && !message.isEmpty())
		ui->caption->setText(ui->caption->text() + " - " + message);

	window()->setWindowTitle(ui->caption->text());
	ui->webView->setEnabled(!wait);
}

void AddFacebookAccountDialog::onAdjustWindowSize()
{
	window()->adjustSize();
}

void AddFacebookAccountDialog::onWebViewLoadStarted()
{
	setWaitMode(true, tr("Loading..."));
}

void AddFacebookAccountDialog::onWebViewLoadFinished(bool ok)
{
	ui->webView->history()->clear();
	setWaitMode(false);
	if (ok)
	{
		QUrl result = ui->webView->url();
#ifdef DEBUG_ENABLED
		qDebug() << "loaded " << result;
#endif
		if (result.host() == FB_AUTH_HOST)
		{
			if (result.hasQueryItem("access_token") && result.hasQueryItem("username"))
			{
				ui->webView->setHtml(Qt::escape(tr("Facebook has confirmed your authorization")));

				_selectedUserId = result.queryItemValue("username");
				_authToken = result.queryItemValue("access_token").split("|").value(1);
#ifdef DEBUG_ENABLED
				qDebug() << "FB auth ok! " << _selectedUserId << _authToken;
#endif
				_succeeded = true;
				accept();
			}
			else if (result.hasQueryItem("error"))
			{
				if (result.queryItemValue("error_reason") != "user_denied")
				{
					LogError(QString("[AddFacebookAccountDialog]: Registration failed: %1").arg(result.queryItemValue("error_description")));
					//abort(FAbortMessage/*result.queryItemValue("error_description").replace('+',' ')*/);
#ifdef DEBUG_ENABLED
					qDebug() << "FB auth failed! " << result.queryItemValue("error_reason") << " | " << result.queryItemValue("error_description");
#endif
					reject();
				}
				else
				{
					LogDetail(QString("[AddFacebookAccountDialog]: Registration canceled by user"));
					reject();
				}
			}
			else
			{
				LogError(QString("[AddFacebookAccountDialog]: Incorrect result: %1").arg(result.toString()));
			}
		}
	}
	else
	{
		// error
		abort(tr("The service is temporarily unavailable, please try to connect later."));
	}
}

void AddFacebookAccountDialog::onWebPageLinkClicked(const QUrl &url)
{
	QDesktopServices::openUrl(url);
}
