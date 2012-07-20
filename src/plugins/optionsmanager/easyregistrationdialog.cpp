#include "easyregistrationdialog.h"
#include "ui_easyregistrationdialog.h"

#include <definitions/resources.h>
#include <definitions/menuicons.h>
#include <definitions/htmlpages.h>
#include <definitions/stylesheets.h>
#include <definitions/stylevalues.h>
#include <definitions/customborder.h>

#include <utils/log.h>
#include <utils/iconstorage.h>
#include <utils/stylestorage.h>
#include <utils/customborderstorage.h>

#ifdef Q_OS_DARWIN
# include <utils/macwidgets.h>
#endif

#include <QFile>
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
#define ERROR_HTML           "<html><body bgcolor=\"%1\" link=\"%2\" vlink=\"%3\" alink=\"%4\"><p align=center><span style=\"font-family: \'%5\'; color: %6; font-size: %7px;\">%8</span></p></body></html>"
#define FULL_REGISTER_URL    "http://id.rambler.ru/profile/create/"

EasyRegistrationDialog::EasyRegistrationDialog(QWidget *parent) :
	QDialog(parent),
	ui(new Ui::EasyRegistrationDialog)
{
	ui->setupUi(this);
	ui->errWidget->setVisible(false);
	errorSet = false;
	loaderShown = false;

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
		connect(this, SIGNAL(registered(const Jid &user)), border, SLOT(close()));
		border->setResizable(false);
		//border->setMaximumSize(size() + QSize(border->leftBorderWidth() + border->rightBorderWidth(), border->topBorderWidth() + border->bottomBorderWidth()));
	}
	else
	{
		setAttribute(Qt::WA_DeleteOnClose, true);
		ui->easyRegWebView->setAttribute(Qt::WA_TranslucentBackground, true);
		//ui->caption->setVisible(false);
		layout()->setContentsMargins(0, 0, 0, 0);
#ifdef Q_OS_DARWIN
		setWindowGrowButtonEnabled(this, false);
#endif

	}

	ui->easyRegWebView->page()->mainFrame()->setScrollBarPolicy(Qt::Vertical, Qt::ScrollBarAlwaysOff);
	ui->easyRegWebView->page()->mainFrame()->setScrollBarPolicy(Qt::Horizontal, Qt::ScrollBarAlwaysOff);
	connect(ui->easyRegWebView, SIGNAL(loadFinished(bool)), SLOT(onLoaded(bool)));
	connect(ui->easyRegWebView->page(), SIGNAL(linkClicked(const QUrl &)),SLOT(onWebPageLinkClicked(const QUrl &)));

	int recommendedWebViewWidth = StyleStorage::staticStorage(RSR_STORAGE_STYLESHEETS)->getStyleInt(SV_EASYREG_WV_DEF_WIDTH, 365);
	int recommendedWebViewHeight = StyleStorage::staticStorage(RSR_STORAGE_STYLESHEETS)->getStyleInt(SV_EASYREG_WV_DEF_HEIGHT, 335);
	recommendedWebViewSize = QSize(recommendedWebViewWidth, recommendedWebViewHeight);

	window()->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
	window()->setWindowModality(Qt::ApplicationModal);

	window()->resize(neededSize());

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
		close();
	QDialog::keyPressEvent(ke);
}

void EasyRegistrationDialog::startLoading()
{
	static QString loader;
	if (loader.isEmpty())
	{
		QString fName = FileStorage::staticStorage(RSR_STORAGE_HTML_PAGES)->fileFullName(HTML_OPTIONS_EASYREG_LOADING);
		QFile f(fName);
		if (f.open(QFile::ReadOnly))
		{
			loader = QString::fromUtf8(f.readAll().data());
			loader = loader.arg(tr("Connecting..."));
		}
		if (loader.isEmpty())
			loader = QString("<html><body><p align=center><h1><font color=white>%1</font></h1></p></body></html>").arg(tr("Connecting..."));
	}
	ui->easyRegWebView->setHtml(loader);
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
	//qDebug() << "*** recommendedWebViewSize: " << recommendedWebViewSize;
	//qDebug() << "*** size hint: " << window()->sizeHint();
	qDebug() << "*** updating window size, error: " << errorSet;
#endif
	ui->easyRegWebView->setFixedSize(recommendedWebViewSize);
	//if (errorSet && !ui->errWidget->isVisible())
	{
		ui->errWidget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Ignored);
	}
	if (errorSet)
	{
		ui->errWidget->setVisible(true);
	}
	//window()->setMaximumSize(neededSize);
	QRect neededGeom = window()->geometry();
	neededGeom.setSize(neededSize());
	//ui->errWidget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Ignored);
	//ui->errWidget->setMinimumSize(0, 0);
	if (neededGeom != window()->geometry())
	{
#ifdef DEBUG_ENABLED
		qDebug() << "*** old geometry:" << window()->geometry() << "new geometry:" << neededGeom;
#endif
		// TODO: move code for window animation to utils' WidgetManager
		QPropertyAnimation *animation = new QPropertyAnimation(window(), "geometry");
		animation->setStartValue(window()->geometry());
		animation->setEndValue(neededGeom);
		int animationDuration = StyleStorage::staticStorage(RSR_STORAGE_STYLESHEETS)->getStyleInt(SV_GLOBAL_ANIMATION_DURATION, 300);
		animation->setDuration(animationDuration);
		connect(animation, SIGNAL(finished()), SLOT(onWindowAnimationComplete()));
		animation->start(QAbstractAnimation::DeleteWhenStopped);
	}
	else
		onWindowAnimationComplete();
}

QSize EasyRegistrationDialog::neededSize()
{
	int neededWidth = layout()->contentsMargins().left() + layout()->contentsMargins().right() + recommendedWebViewSize.width();
	int neededHeight = layout()->contentsMargins().top() + layout()->contentsMargins().bottom() + recommendedWebViewSize.height() + ui->caption->sizeHint().height();
	QSize ns(neededWidth, neededHeight);
	CustomBorderContainer *border = CustomBorderStorage::widgetBorder(this);
	if (border)
	{
		ns += QSize(border->leftBorderWidth() + border->rightBorderWidth(), border->topBorderWidth() + border->bottomBorderWidth());
	}
	if (errorSet)
	{
		int errWidgetHeight = StyleStorage::staticStorage(RSR_STORAGE_STYLESHEETS)->getStyleInt(SV_EASYREG_ERROR_WIDGET_HEIGHT, 60);
		ns += QSize(0, errWidgetHeight);
	}
#ifdef DEBUG_ENABLED
	qDebug() << "*** needed size:" << ns;
#endif
	return ns;
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
	window()->adjustSize();
}

void EasyRegistrationDialog::onLoaded(bool ok)
{
	static QString errorHtml;

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

			if (errorHtml.isEmpty())
			{

				QString fName = FileStorage::staticStorage(RSR_STORAGE_HTML_PAGES)->fileFullName(HTML_OPTIONS_EASYREG_ERROR);
				QFile f(fName);
				if (f.open(QFile::ReadOnly))
				{
					errorHtml = QString::fromUtf8(f.readAll().data());
					QString errText = tr("Check internet connection and reload page, or register at <a href=\"%1\" class=\"b-link\">website</a>.").arg(FULL_REGISTER_URL);
					errorHtml = errorHtml.arg(tr("Could not connect")).arg(errText).arg("http://"EASY_REG_URL"/").arg(tr("Reload"));
				}
				if (errorHtml.isEmpty())
				{
					// style
					QString bgcolor = StyleStorage::staticStorage(RSR_STORAGE_STYLESHEETS)->getStyleColor(SV_GLOBAL_BACKGROUND_COLOR).name();
					QString linkcolor = StyleStorage::staticStorage(RSR_STORAGE_STYLESHEETS)->getStyleColor(SV_GLOBAL_LINK_COLOR).name();
					QString vlinkcolor = linkcolor;
					QString alinkcolor = StyleStorage::staticStorage(RSR_STORAGE_STYLESHEETS)->getStyleColor(SV_GLOBAL_DARK_LINK_COLOR).name();
					QString fontface = StyleStorage::staticStorage(RSR_STORAGE_STYLESHEETS)->getStyleValue(SV_GLOBAL_FONT_FACE).toString();
					QString fontcolor = StyleStorage::staticStorage(RSR_STORAGE_STYLESHEETS)->getStyleColor(SV_GLOBAL_TEXT_COLOR).name();
					QString fontsize = QString::number(StyleStorage::staticStorage(RSR_STORAGE_STYLESHEETS)->getStyleInt(SV_EASYREG_ERROR_FONT_SIZE, 18));
					// link
					QString link = QString("<a href=\"%1\" target=_blank>http://rambler.ru</a>").arg(FULL_REGISTER_URL);
					// error html
					QString errorText = tr("Registration is tempoprarily unavailable. Please, register online at %1").arg(link);
					errorHtml = QString(ERROR_HTML).arg(bgcolor, linkcolor, vlinkcolor, alinkcolor, fontface, fontcolor, fontsize, errorText);
				}
			}
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
		else if (!loaderShown)
		{
			loaderShown = true;
			QNetworkRequest request(QUrl("http://"EASY_REG_URL"/"));
			request.setRawHeader("Accept-Encoding", "identity");
			ui->easyRegWebView->load(request);
		}
	}
}

void EasyRegistrationDialog::onWebPageLinkClicked(const QUrl &url)
{
	if (url.toString() == EASY_REG_URL)
	{
		loaderShown = false;
		startLoading();
	}
	else
	{
		QDesktopServices::openUrl(url);
		close();
	}
}

void EasyRegistrationDialog::showEvent(QShowEvent *se)
{
	QDialog::showEvent(se);
	updateWindowSize();
}
