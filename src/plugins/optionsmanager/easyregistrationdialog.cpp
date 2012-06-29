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
#include <QPropertyAnimation>

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
	ui->errWidget->setVisible(false);
	errorSet = false;

	// logo
	IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->insertAutoIcon(ui->caption, MNI_OPTIONS_LOGIN_LOGO, 0, 0, "pixmap");

	// error
	IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->insertAutoIcon(ui->errIcon, MNI_OPTIONS_ERROR_ALERT, 0, 0, "pixmap");

	// style
	StyleStorage::staticStorage(RSR_STORAGE_STYLESHEETS)->insertAutoStyle(this, STS_OPTIONS_EASYREGISTRATIONDIALOG);

	// custom border
	CustomBorderContainer *border = CustomBorderStorage::staticStorage(RSR_STORAGE_CUSTOMBORDER)->addBorder(this, CBS_DIALOG);
	if (border)
	{
		border->setAttribute(Qt::WA_DeleteOnClose, true);
		border->setMaximizeButtonVisible(false);
		border->setMinimizeButtonVisible(false);
		connect(border, SIGNAL(closeClicked()), SLOT(close()));
		connect(this, SIGNAL(aborted()), border, SLOT(close()));
		connect(this, SIGNAL(registered(const QString &login)), border, SLOT(close()));
		border->setResizable(false);
		border->setMaximumSize(size() + QSize(border->leftBorderWidth() + border->rightBorderWidth(), border->topBorderWidth() + border->bottomBorderWidth()));
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

	window()->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
	window()->setWindowModality(Qt::ApplicationModal);

	recommendedWebViewSize = QSize(400, 500);

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

void EasyRegistrationDialog::setError(bool on)
{
	if (errorSet != on)
	{
		errorSet = on;
		updateWindowSize();
	}
}

void EasyRegistrationDialog::parseSize(const QString &sizeString)
{
	QStringList params = sizeString.split('=');
	if ((params.count() > 1) && (params.at(0) == "size"))
	{
		params = params.at(1).split('x');
		if (params.count() == 2)
		{
			bool ok = true;
			int rWidth = params.at(0).toInt(&ok);
			if (ok)
			{
				int rHeight = params.at(1).toInt(&ok);
				if (ok)
				{
					QSize parsedSize = QSize(rWidth, rHeight);
					if (recommendedWebViewSize != parsedSize)
					{
						recommendedWebViewSize = parsedSize;
						updateWindowSize();
					}
				}
			}
		}
	}
}

void EasyRegistrationDialog::updateWindowSize()
{
#ifdef DEBUG_ENABLED
	qDebug() << "*** recommendedWebViewSize: " << recommendedWebViewSize;
	qDebug() << "*** size hint: " << window()->sizeHint();
	qDebug() << "*** error: " << errorSet;
#endif
	ui->easyRegWebView->setFixedSize(recommendedWebViewSize);
	if (errorSet && !ui->errWidget->isVisible())
	{
		ui->errWidget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Ignored);
	}
	QSize neededSize = sizeHint();
	CustomBorderContainer *border = CustomBorderStorage::widgetBorder(this);
	if (border)
	{
		neededSize += QSize(border->leftBorderWidth() + border->rightBorderWidth(), border->topBorderWidth() + border->bottomBorderWidth());
	}
	if (errorSet && !ui->errWidget->isVisible())
	{
		neededSize += QSize(0, 65);
		ui->errWidget->setVisible(true);
	}
	window()->setMaximumSize(neededSize);
	QRect neededGeom = window()->geometry();
	neededGeom.setSize(neededSize);
	//ui->errWidget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Ignored);
	//ui->errWidget->setMinimumSize(0, 0);
	if (neededGeom != window()->geometry())
	{
		QPropertyAnimation *animation = new QPropertyAnimation(window(), "geometry");
		animation->setStartValue(window()->geometry());
		animation->setEndValue(neededGeom);
		animation->setDuration(3000);
		connect(animation, SIGNAL(finished()), SLOT(onWindowAnimationComplete()));
		animation->start(QAbstractAnimation::DeleteWhenStopped);
	}
	else
		onWindowAnimationComplete();
}

void EasyRegistrationDialog::onWindowAnimationComplete()
{
#ifdef DEBUG_ENABLED
	qDebug() << "*** animation complete! error: " << errorSet;
#endif
	if (errorSet)
		ui->errWidget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
	else
		ui->errWidget->setVisible(false);
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
			setError(true);
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
			setError(false);
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
			if (url.hasFragment())
				parseSize(url.fragment());
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
	updateWindowSize();
}
