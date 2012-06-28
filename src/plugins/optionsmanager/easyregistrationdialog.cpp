#include "easyregistrationdialog.h"
#include "ui_easyregistrationdialog.h"

#include <definitions/resources.h>
#include <definitions/stylesheets.h>
#include <definitions/menuicons.h>
#include <definitions/customborder.h>

#include <utils/log.h>
#include <utils/iconstorage.h>
#include <utils/stylestorage.h>
#include <utils/customborderstorage.h>

#include <QKeyEvent>
#include <QWebFrame>
#include <QWebHistory>
#include <QNetworkRequest>
#include <QDesktopServices>

#ifdef DEBUG_ENABLED
# include <QDebug>
#endif

#define EASY_REG_URL         "reg.tx.xmpp.rambler.ru"
#define ERROR_HTML           "<html><body bgcolor=\"%1\" link=\"%2\" vlink=\"%3\" alink=\"%4\"><span style=\"font-family: \'%5\'; color: %6; font-size: %7px;\">%8</span></body></html>"
#define FULL_REGISTER_URL    "http://id.rambler.ru/profile/create/"

EasyRegistrationDialog::EasyRegistrationDialog(QWidget *parent) :
	QDialog(parent),
	ui(new Ui::EasyRegistrationDialog)
{
	ui->setupUi(this);

	// logo
	IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->insertAutoIcon(ui->caption, MNI_OPTIONS_LOGIN_LOGO, 0, 0, "pixmap");

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
		//ui->caption->setVisible(false);
		layout()->setContentsMargins(0, 0, 0, 0);
	}

	ui->easyRegWebView->page()->mainFrame()->setScrollBarPolicy(Qt::Vertical, Qt::ScrollBarAlwaysOff);
	ui->easyRegWebView->page()->mainFrame()->setScrollBarPolicy(Qt::Horizontal, Qt::ScrollBarAlwaysOff);
	connect(ui->easyRegWebView, SIGNAL(loadFinished(bool)), SLOT(onLoaded(bool)));
	connect(ui->easyRegWebView->page(),SIGNAL(linkClicked(const QUrl &)),SLOT(onWebPageLinkClicked(const QUrl &)));

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
	else
	{
		emit registered(userJid);
	}

	if (parentWidget())
		parentWidget()->close();
	else
		QDialog::closeEvent(ce);
}

void EasyRegistrationDialog::keyPressEvent(QKeyEvent *ke)
{
	if (ke->key() == Qt::Key_Escape)
	{
		close();
	}
	QDialog::keyPressEvent(ke);
}

void EasyRegistrationDialog::startLoading()
{
	QNetworkRequest request(QUrl("http://"EASY_REG_URL"/"));
	request.setRawHeader("Accept-Encoding","identity");
	ui->easyRegWebView->load(request);
}

void EasyRegistrationDialog::onLoaded(bool ok)
{
	ui->easyRegWebView->history()->clear();
	QUrl url = ui->easyRegWebView->url();
	if (!ok)
	{
		// set error
#ifdef DEBUG_ENABLED
		qDebug() << "web view failed to load!";
#endif
		if (url.host() == EASY_REG_URL)
		{
			// we have some data loaded
			// TODO: show some error
		}
		else
		{
			// no data loaded
			ui->easyRegWebView->page()->setLinkDelegationPolicy(QWebPage::DelegateAllLinks);
			// style
			QString bgcolor = StyleStorage::staticStorage(RSR_STORAGE_STYLESHEETS)->getStyleColor("globalBackgroundColor").name();
			QString linkcolor = StyleStorage::staticStorage(RSR_STORAGE_STYLESHEETS)->getStyleColor("globalLinkColor").name();
			QString vlinkcolor = linkcolor;
			QString alinkcolor = StyleStorage::staticStorage(RSR_STORAGE_STYLESHEETS)->getStyleColor("globalDarkLinkColor").name();
			QString fontface = StyleStorage::staticStorage(RSR_STORAGE_STYLESHEETS)->getStyleValue("globalFontFace").toString();
			QString fontcolor = StyleStorage::staticStorage(RSR_STORAGE_STYLESHEETS)->getStyleColor("globalTextColor").name();
			QString fontsize = QString::number(StyleStorage::staticStorage(RSR_STORAGE_STYLESHEETS)->getStyleInt("easyRegisterDialogErrorFontSize"));
			// link
			QString link = QString("<a href=\"%1\" target=_blank>http://rambler.ru</a>").arg(FULL_REGISTER_URL);
			// error html
			QString errorText = tr("Registration is tempoprarily unavailable. Please, register online at %1").arg(link);
			QString errorHtml = QString(ERROR_HTML).arg(bgcolor, linkcolor, vlinkcolor, alinkcolor, fontface, fontcolor, fontsize, errorText);
#ifdef DEBUG_ENABLED
			qDebug() << errorHtml;
#endif
			ui->easyRegWebView->setHtml(errorHtml);
		}
	}
	else
	{
#ifdef DEBUG_ENABLED
		qDebug() << "web view loaded! url:" << url;
#endif
		if (url.host() == EASY_REG_URL)
		{
			if (url.hasQueryItem("login") && url.hasQueryItem("domain") && url.hasQueryItem("success"))
			{
				bool registrationOk = url.queryItemValue("success").toInt() == 1;
				if (registrationOk)
				{
					userJid.setNode(url.queryItemValue("login"));
					userJid.setDomain(url.queryItemValue("domain"));
				}
			}
			if ((url.hasQueryItem("close") && (url.queryItemValue("close").toInt() == 1)) || url.hasQueryItem("error"))
			{
				close();
			}
		}
	}
}

void EasyRegistrationDialog::onWebPageLinkClicked(const QUrl &url)
{
	QDesktopServices::openUrl(url);
	close();
}

void EasyRegistrationDialog::showEvent(QShowEvent *se)
{
	QDialog::showEvent(se);
}
