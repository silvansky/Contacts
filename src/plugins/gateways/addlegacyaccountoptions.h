#ifndef ADDLEGACYACCOUNTOPTIONS_H
#define ADDLEGACYACCOUNTOPTIONS_H

#include <QWidget>
#include <interfaces/ioptionsmanager.h>
#include "ui_addlegacyaccountoptions.h"

class AddLegacyAccountOptions : 
			public QWidget,
			public IOptionsWidget
{
	Q_OBJECT;
	Q_INTERFACES(IOptionsWidget);
public:
	AddLegacyAccountOptions(QWidget *AParent = NULL);
	~AddLegacyAccountOptions();
	virtual QWidget* instance() { return this; }
public slots:
	virtual void apply();
	virtual void reset();
signals:
	void modified();
	void childApply();
	void childReset();
private:
	Ui::AddLegacyAccountOptionsClass ui;
};

#endif // ADDLEGACYACCOUNTOPTIONS_H
