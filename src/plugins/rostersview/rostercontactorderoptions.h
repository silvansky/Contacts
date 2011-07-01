#ifndef ROSTERCONTACTORDEROPTIONS_H
#define ROSTERCONTACTORDEROPTIONS_H

#include <QWidget>
#include <definitions/optionvalues.h>
#include <interfaces/ioptionsmanager.h>
#include <utils/options.h>
#include "ui_rostercontactorderoptions.h"

class RosterContactOrderOptions :
	public QWidget,
	public IOptionsWidget
{
	Q_OBJECT
	Q_INTERFACES(IOptionsWidget)
public:
	RosterContactOrderOptions(QWidget *AParent = NULL);
	~RosterContactOrderOptions();
	virtual QWidget* instance() { return this; }
public slots:
	virtual void apply();
	virtual void reset();
signals:
	void modified();
	void updated();
	void childApply();
	void childReset();
private:
	Ui::RosterContactOrderOptionsClass ui;
};

#endif // ROSTERCONTACTORDEROPTIONS_H
