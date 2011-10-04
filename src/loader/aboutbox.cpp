#include "aboutbox.h"

#include <QShowEvent>
#include <QDesktopServices>
#include "aboutqtdialog.h"
#include <utils/customborderstorage.h>
#include <utils/stylestorage.h>
#include <utils/iconstorage.h>
#include <utils/graphicseffectsstorage.h>
#include <definitions/resources.h>
#include <definitions/customborder.h>
#include <definitions/stylesheets.h>
#include <definitions/graphicseffects.h>
#include <definitions/menuicons.h>

AboutBox::AboutBox(IPluginManager *APluginManager, QWidget *AParent) : QDialog(AParent)
{
	ui.setupUi(this);
	setAttribute(Qt::WA_DeleteOnClose,true);

#ifdef Q_WS_MAC
	ui.buttonsLayout->setSpacing(16);
	ui.buttonsLayout->addWidget(ui.pbtSendComment);
#endif

	QString styleBegin = "<html><style>a { color: #acacac; }</style><body><font color=#acacac>";
	QString styleEnd = "</font></body></html>";

	//ui.lblName->setText(tr("Contacts"));
	IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->insertAutoIcon(ui.lblName, MNI_OPTIONS_LOGIN_LOGO, 0, 0, "pixmap");
	ui.lblVersion->setText(styleBegin + tr("Version %1.%2 %3").arg(APluginManager->version()).arg(APluginManager->revision()).arg(CLIENT_VERSION_SUFIX).trimmed() + styleEnd);
	ui.lblHomePage->setText(styleBegin + tr("Official site: %1").arg("<a href='http://contacts.rambler.ru'>contacts.rambler.ru</a>") + "</font>");
	ui.lblCopyright->setText(styleBegin + tr("© 2011, \"Rambler Internet Holding LLC\".<br>%1").arg(QString("<a href='http://help.rambler.ru/legal/?s=44761'>%1</a>").arg(tr("Terms of Use"))) + styleEnd);
	ui.lblFontInfo->setText(styleBegin + tr("The program uses the Segoe UI font on the license granted Monotype Imaging Inc. %1").arg("<a href=\'http://www.fonts.com\'>www.fonts.com</a>") + styleEnd);
	ui.lblAboutQt->setText(styleBegin + tr("The program is developed with %1.").arg("<a href=\'about:qt\'>Qt</a>") + styleEnd);

	connect(ui.lblAboutQt, SIGNAL(linkActivated(const QString &)), SLOT(onLabelLinkActivated(const QString &)));
	connect(ui.lblHomePage,SIGNAL(linkActivated(const QString &)),SLOT(onLabelLinkActivated(const QString &)));
	connect(ui.lblCopyright,SIGNAL(linkActivated(const QString &)),SLOT(onLabelLinkActivated(const QString &)));
	connect(ui.pbtSendComment, SIGNAL(clicked()), APluginManager->instance(), SLOT(onShowCommentsDialog()));

	border = CustomBorderStorage::staticStorage(RSR_STORAGE_CUSTOMBORDER)->addBorder(this, CBS_DIALOG);
	if (border)
	{
		border->setResizable(false);
		border->setMinimizeButtonVisible(false);
		border->setMaximizeButtonVisible(false);
		border->setAttribute(Qt::WA_DeleteOnClose, true);
		connect(border, SIGNAL(closeClicked()), SLOT(reject()));
		connect(this, SIGNAL(accepted()), border, SLOT(closeWidget()));
		connect(this, SIGNAL(rejected()), border, SLOT(closeWidget()));
	}
	StyleStorage::staticStorage(RSR_STORAGE_STYLESHEETS)->insertAutoStyle(this, STS_PLUGINMANAGER_ABOUTBOX);
	GraphicsEffectsStorage::staticStorage(RSR_STORAGE_GRAPHICSEFFECTS)->installGraphicsEffect(this, GFX_LABELS);
}

AboutBox::~AboutBox()
{
	if (border)
		border->deleteLater();
}

void AboutBox::onLabelLinkActivated(const QString &ALink)
{
	if (ALink == "about:qt")
	{
		AboutQtDialog::aboutQt();
	}
	else
		QDesktopServices::openUrl(ALink);
}

