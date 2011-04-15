#include "aboutbox.h"

#include <QShowEvent>
#include <QDesktopServices>
#include <utils/customborderstorage.h>
#include <definitions/resources.h>
#include <definitions/customborder.h>

AboutBox::AboutBox(IPluginManager *APluginManager, QWidget *AParent) : QDialog(AParent)
{
	ui.setupUi(this);
	setAttribute(Qt::WA_DeleteOnClose,true);

	ui.lblName->setText("<font size=+4>friends</font>");
	ui.lblVersion->setText(tr("Version %1.%2 %3").arg(APluginManager->version()).arg(APluginManager->revision()).arg(CLIENT_VERSION_SUFIX).trimmed());
	ui.lblHomePage->setText(tr("Official site: %1").arg("<a href='http://virtus.rambler.ru'>http://virtus.rambler.ru</a>"));
	ui.lblCopyright->setText(tr("Copyright 2010-2011, \"Rambler Internet Holding Ltd\". All rights reserved.<br>%1").arg(QString("<a href='http://virtus.rambler.ru'>%1</a>").arg(tr("Terms of Use"))));

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
}

AboutBox::~AboutBox()
{
	if (border)
		border->deleteLater();
}

void AboutBox::onLabelLinkActivated(const QString &ALink)
{
	QDesktopServices::openUrl(ALink);
}

