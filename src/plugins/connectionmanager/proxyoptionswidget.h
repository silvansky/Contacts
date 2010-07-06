#ifndef PROXYOPTIONSWIDGET_H
#define PROXYOPTIONSWIDGET_H

#include <interfaces/ioptionsmanager.h>
#include "ui_proxyoptionswidget.h"

class ProxyOptionsWidget :
		public QWidget,
		public IOptionsWidget
{
	Q_OBJECT;
	Q_INTERFACES(IOptionsWidget);
public:
	ProxyOptionsWidget(QWidget *AParent = NULL);
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
};

#endif // PROXYOPTIONSWIDGET_H
