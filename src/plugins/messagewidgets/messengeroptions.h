#ifndef MESSENGEROPTIONS_H
#define MESSENGEROPTIONS_H

#include <QKeySequence>
#include <definitions/optionvalues.h>
#include <interfaces/ioptionsmanager.h>
#include <utils/options.h>
#include "ui_messengeroptions.h"

class MessengerOptions :
	public QWidget,
	public IOptionsWidget
{
	Q_OBJECT
	Q_INTERFACES(IOptionsWidget)
public:
	MessengerOptions(QWidget *AParent);
	~MessengerOptions();
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
	Ui::MessengerOptionsClass ui;
};

#endif // MESSENGEROPTIONS_H
