#include "aboutbox.h"

#include <QDesktopServices>

AboutBox::AboutBox(IPluginManager *APluginManager, Updater* updater, QWidget *AParent) : QDialog(AParent)
{
	ui.setupUi(this);
	setAttribute(Qt::WA_DeleteOnClose,true);

	ui.lblName->setText(CLIENT_NAME);
	ui.lblVersion->setText(tr("Version: %1.%2 %3").arg(APluginManager->version()).arg(APluginManager->revision()).arg(CLIENT_VERSION_SUFIX));

	// Пока проверку на версию не делаем. Т.к. не понятно как сравнивать номера версий.
	//if(APluginManager->version() == updater->getVersion())
	//{
	//	//setVersion(updater->getVersion());
	//	setDescription(tr("You use latest version"));
	//	setSize(0);
	//	ui.pbtUpdate->setVisible(false);
	//}
	//else
	{
		setVersion(updater->getVersion());
		setDescription(updater->getDescription());
		setSize(updater->getSize());
		ui.pbtUpdate->setVisible(true);
	}


	connect(ui.lblHomePage,SIGNAL(linkActivated(const QString &)),SLOT(onLabelLinkActivated(const QString &)));
	connect(ui.pbtSendComment, SIGNAL(clicked()), APluginManager->instance(), SLOT(onShowCommentsDialog()));

	connect(ui.pbtUpdate, SIGNAL(clicked()), this, SLOT(startUpdate()));
	connect(ui.pbtUpdate, SIGNAL(clicked()), updater, SLOT(update()));
	

	connect(updater, SIGNAL(updateFinished(QString, bool)), this, SLOT(updateFinish(QString, bool)));
	//connect(updater, SIGNAL(updateCanceled(QString)), this, SLOT(updateFinish(QString)));
	connect(updater, SIGNAL(updateFinished(QString, bool)), APluginManager->instance(), SLOT(updateMe(QString, bool)));
}

AboutBox::~AboutBox()
{

}

// Параметры обновления
void AboutBox::setVersion(QString version)
{
	ui.lblNewVersion->setText(tr("New version ") + CLIENT_NAME + "(" + version + ")");
}

void AboutBox::setSize(int size)
{
	QString strSize;
	strSize.setNum(size);
	ui.lblUpdateSize->setText(strSize);
	if(size > 0)
	{
		ui.lblUpdateSize->setVisible(true);
	}
	else
	{
		ui.lblUpdateSize->setVisible(false);
	}
}

//QUrl setUrl() const { return url; }
void AboutBox::setDescription(QString description)
{
	if(description.isNull() || description.isEmpty()) 
		return;

	ui.lblVersionInfo->setText(description);
}


void AboutBox::onLabelLinkActivated(const QString &ALink)
{
	QDesktopServices::openUrl(ALink);
}

void AboutBox::startUpdate()
{
	ui.pbtUpdate->setEnabled(false);
}

void AboutBox::updateFinish(QString reason, bool state)
{
	
	if(state == false)
		QMessageBox::information(this, tr("Update"), reason);
	ui.pbtUpdate->setEnabled(true);
}
