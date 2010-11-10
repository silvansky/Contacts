#ifndef PROXYOPTIONSWIDGET_H
#define PROXYOPTIONSWIDGET_H

#include <definitions/optionvalues.h>
#include <interfaces/iconnectionmanager.h>
#include <interfaces/ioptionsmanager.h>
#include <utils/options.h>
#include "ui_proxyoptionswidget.h"

class ProxyOptionsWidget :
		public QWidget,
		public IOptionsWidget
{
	Q_OBJECT;
	Q_INTERFACES(IOptionsWidget);
public:
	ProxyOptionsWidget(IConnectionManager *AManager, OptionsNode ANode, QWidget *AParent = NULL);
	virtual QWidget* instance() { return this; }
public slots:
	void apply();
	void reset();
signals:
	void modified();
	void childApply();
	void childReset();
private:
	Ui::ProxyOptionsWidget ui;
private:
	IConnectionManager *FManager;
private:
	OptionsNode FConnectionNode;
};

#endif // PROXYOPTIONSWIDGET_H
