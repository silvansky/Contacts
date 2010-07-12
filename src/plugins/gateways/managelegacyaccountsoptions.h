#ifndef MANAGELEGACYACCOUNTSOPTIONS_H
#define MANAGELEGACYACCOUNTSOPTIONS_H

#include <QWidget>
#include <interfaces/ioptionsmanager.h>
#include "ui_managelegacyaccountsoptions.h"

class ManageLegacyAccountsOptions : 
			public QWidget,
			public IOptionsWidget
{
	Q_OBJECT;
	Q_INTERFACES(IOptionsWidget);
public:
	ManageLegacyAccountsOptions(QWidget *AParent = NULL);
	~ManageLegacyAccountsOptions();
	virtual QWidget* instance() { return this; }
public slots:
	virtual void apply();
	virtual void reset();
signals:
	void modified();
	void childApply();
	void childReset();
private:
	Ui::ManageLegacyAccountsOptionsClass ui;
};

#endif // MANAGELEGACYACCOUNTSOPTIONS_H
