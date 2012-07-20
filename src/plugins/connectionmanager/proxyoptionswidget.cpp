#include "proxyoptionswidget.h"
#include <QIntValidator>

ProxyOptionsWidget::ProxyOptionsWidget(IConnectionManager *AManager, OptionsNode ANode, QWidget *AParent) : QWidget(AParent)
{
	ui.setupUi(this);

	ui.lneProxyHost->setAttribute(Qt::WA_MacShowFocusRect, false);
	ui.lneProxyPassword->setAttribute(Qt::WA_MacShowFocusRect, false);
	ui.lneProxyUser->setAttribute(Qt::WA_MacShowFocusRect, false);

	ui.gbManualProxySettings->setEnabled(false);

	FManager = AManager;
	FConnectionNode = ANode;

	ui.lneProxyPort->setValidator(new QIntValidator(1, 65535, ui.lneProxyPort));

	connect(ui.rdbAutoProxy,SIGNAL(toggled(bool)),SIGNAL(modified()));
	connect(ui.rdbExplorerProxy,SIGNAL(toggled(bool)),SIGNAL(modified()));
	connect(ui.rdbFireFoxProxy,SIGNAL(toggled(bool)),SIGNAL(modified()));
	connect(ui.rdbManualProxy,SIGNAL(toggled(bool)),SIGNAL(modified()));
	connect(ui.lneProxyHost,SIGNAL(textChanged(const QString &)),SIGNAL(modified()));
	connect(ui.lneProxyPort,SIGNAL(textChanged(const QString&)),SIGNAL(modified()));
	connect(ui.chbProxyUserPassword,SIGNAL(toggled(bool)),SIGNAL(modified()));
	connect(ui.lneProxyUser,SIGNAL(textChanged(const QString &)),SIGNAL(modified()));
	connect(ui.lneProxyPassword,SIGNAL(textChanged(const QString &)),SIGNAL(modified()));

	reset();
}

void ProxyOptionsWidget::apply()
{
	if (ui.rdbManualProxy->isChecked() && (ui.lneProxyHost->text().isEmpty() || ui.lneProxyPort->text().toInt()==0))
		ui.rdbAutoProxy->setChecked(true);

	if (ui.rdbAutoProxy->isChecked())
		FConnectionNode.setValue(APPLICATION_PROXY_REF_UUID, "proxy");
	else if (ui.rdbExplorerProxy->isChecked())
		FConnectionNode.setValue(IEXPLORER_PROXY_REF_UUID, "proxy");
	else if (ui.rdbFireFoxProxy->isChecked())
		FConnectionNode.setValue(FIREFOX_PROXY_REF_UUID, "proxy");
	else
		FConnectionNode.setValue(MANUAL_PROXY_REF_UUID, "proxy");

	IConnectionProxy proxy;
	proxy.name = tr("Manual Proxy");
	proxy.proxy.setType(QNetworkProxy::HttpProxy);
	proxy.proxy.setHostName(ui.lneProxyHost->text());
	proxy.proxy.setPort(ui.lneProxyPort->text().toInt());
	proxy.proxy.setUser(ui.chbProxyUserPassword->isChecked() ? ui.lneProxyUser->text() : QString::null);
	proxy.proxy.setPassword(ui.chbProxyUserPassword->isChecked() ? ui.lneProxyPassword->text() : QString::null);
	FManager->setProxy(MANUAL_PROXY_REF_UUID, proxy);

	emit childApply();
}

void ProxyOptionsWidget::reset()
{
	QList<QUuid> proxies = FManager->proxyList();
	ui.rdbExplorerProxy->setVisible(proxies.contains(IEXPLORER_PROXY_REF_UUID));
	ui.rdbFireFoxProxy->setVisible(proxies.contains(FIREFOX_PROXY_REF_UUID));

	QString proxyId = FConnectionNode.node("proxy").value().toString();
	if (proxyId == APPLICATION_PROXY_REF_UUID)
		ui.rdbAutoProxy->setChecked(true);
	else if (proxyId==IEXPLORER_PROXY_REF_UUID && proxies.contains(IEXPLORER_PROXY_REF_UUID))
		ui.rdbExplorerProxy->setChecked(true);
	else if (proxyId==FIREFOX_PROXY_REF_UUID && proxies.contains(FIREFOX_PROXY_REF_UUID))
		ui.rdbFireFoxProxy->setChecked(true);
	else
		ui.rdbManualProxy->setChecked(true);

	IConnectionProxy proxy = FManager->proxyById(MANUAL_PROXY_REF_UUID);
	ui.lneProxyHost->setText(proxy.proxy.hostName());
	ui.lneProxyPort->setText(proxy.proxy.port()>0 ? QString::number(proxy.proxy.port()) : QString::null);
	ui.lneProxyUser->setText(proxy.proxy.user());
	ui.lneProxyPassword->setText(proxy.proxy.password());
	ui.chbProxyUserPassword->setChecked(!ui.lneProxyUser->text().isEmpty());

	emit childReset();
}
