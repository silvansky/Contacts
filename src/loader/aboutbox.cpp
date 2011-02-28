#include "aboutbox.h"

#include <QDesktopServices>

AboutBox::AboutBox(IPluginManager *APluginManager, QWidget *AParent) : QDialog(AParent)
{
	ui.setupUi(this);
	setAttribute(Qt::WA_DeleteOnClose,true);

	ui.lblName->setText("VIRTUS IM");
	ui.lblVersion->setText(tr("Version: %1.%2 %3").arg(APluginManager->version()).arg(APluginManager->revision()).arg(CLIENT_VERSION_SUFIX).trimmed());
	ui.lblHomePage->setText(tr("Home page: %1").arg("<a href='http://virtus.rambler.ru'>http://virtus.rambler.ru</a>"));
	ui.lblCopyright->setText(tr("Copyright 2010, Ltd. 'Rambler Internet Holding'. All rights reserved. %1").arg(QString("<a href='http://virtus.rambler.ru'>%1</a>").arg(tr("Terms of Use"))));

	connect(ui.lblHomePage,SIGNAL(linkActivated(const QString &)),SLOT(onLabelLinkActivated(const QString &)));
	connect(ui.lblCopyright,SIGNAL(linkActivated(const QString &)),SLOT(onLabelLinkActivated(const QString &)));
	connect(ui.pbtSendComment, SIGNAL(clicked()), APluginManager->instance(), SLOT(onShowCommentsDialog()));
}

AboutBox::~AboutBox()
{

}

void AboutBox::onLabelLinkActivated(const QString &ALink)
{
	QDesktopServices::openUrl(ALink);
}
