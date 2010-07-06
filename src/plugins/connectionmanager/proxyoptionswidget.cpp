#include "proxyoptionswidget.h"

ProxyOptionsWidget::ProxyOptionsWidget(QWidget *AParent) : QWidget(AParent)
{
	ui.setupUi(this);
	connect(ui.rdbAutoProxy,SIGNAL(toggled(bool)),SIGNAL(modified()));
	connect(ui.rdbManualProxy,SIGNAL(toggled(bool)),SIGNAL(modified()));
	connect(ui.lneProxyHost,SIGNAL(textChanged(const QString &)),SIGNAL(modified()));
	connect(ui.spbProxyPort,SIGNAL(valueChanged(int)),SIGNAL(modified()));
	connect(ui.chbProxyUserPassword,SIGNAL(toggled(bool)),SIGNAL(modified()));
	connect(ui.lneProxyUser,SIGNAL(textChanged(const QString &)),SIGNAL(modified()));
	connect(ui.lneProxyPassword,SIGNAL(textChanged(const QString &)),SIGNAL(modified()));
}

void ProxyOptionsWidget::apply()
{
	emit childApply();
}

void ProxyOptionsWidget::reset()
{
	emit childReset();
}
