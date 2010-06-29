#ifndef ROSTEROPTIONS_H
#define ROSTEROPTIONS_H

#include <QWidget>
#include <definations/optionvalues.h>
#include <definations/resources.h>
#include <definations/menuicons.h>
#include <interfaces/ioptionsmanager.h>
#include <utils/options.h>
#include <utils/iconstorage.h>
#include "ui_rosteroptionswidget.h"

class RosterOptionsWidget : 
			public QWidget,
			public IOptionsWidget
{
	Q_OBJECT;
	Q_INTERFACES(IOptionsWidget);
public:
	RosterOptionsWidget(QWidget *AParent = NULL);
	~RosterOptionsWidget();
	virtual QWidget* instance() { return this; }
public slots:
	virtual void apply();
	virtual void reset();
signals:
	void modified();
	void childApply();
	void childReset();
private:
	Ui::RosterOptionsWidgetClass ui;
};

#endif // ROSTEROPTIONS_H
