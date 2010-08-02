#ifndef ROSTERCONTACTVIEWOPTIONS_H
#define ROSTERCONTACTVIEWOPTIONS_H

#include <QWidget>
#include <definations/optionvalues.h>
#include <definations/resources.h>
#include <definations/menuicons.h>
#include <interfaces/ioptionsmanager.h>
#include <utils/options.h>
#include <utils/iconstorage.h>
#include "ui_rostercontactviewoptions.h"

class RosterContactViewOptions : 
	public QWidget,
	public IOptionsWidget
{
	Q_OBJECT;
	Q_INTERFACES(IOptionsWidget);
public:
	RosterContactViewOptions(QWidget *AParent = NULL);
	~RosterContactViewOptions();
	virtual QWidget* instance() { return this; }
public slots:
	virtual void apply();
	virtual void reset();
signals:
	void modified();
	void childApply();
	void childReset();
private:
	Ui::RosterContactViewOptionsClass ui;
};

#endif // ROSTERCONTACTVIEWOPTIONS_H
