#ifndef HISTORYOPTIONSWIDGET_H
#define HISTORYOPTIONSWIDGET_H

#include <QWidget>
#include <interfaces/iramblerhistory.h>
#include <interfaces/ioptionsmanager.h>
#include <interfaces/ixmppstreams.h>
#include "ui_historyoptionswidget.h"

class HistoryOptionsWidget : 
	public QWidget,
	public IOptionsWidget
{
	Q_OBJECT;
	Q_INTERFACES(IOptionsWidget);
public:
	HistoryOptionsWidget(IRamblerHistory *AHistory, IXmppStream *AXmppStream, QWidget *AParent = NULL);
	~HistoryOptionsWidget();
	virtual QWidget* instance() { return this; }
public slots:
	virtual void apply();
	virtual void reset();
signals:
	void modified();
	void updated();
	void childApply();
	void childReset();
protected slots:
	void onHistoryPrefsChanged(const Jid &AStreamJid, const IHistoryStreamPrefs &APrefs);
private:
	Ui::HistoryOptionsWidgetClass ui;
private:
	IXmppStream *FXmppStream;
	IRamblerHistory *FHistory;
};

#endif // HISTORYOPTIONSWIDGET_H
