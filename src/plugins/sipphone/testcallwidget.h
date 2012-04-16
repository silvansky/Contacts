/*
 * This file will be removed. I mean it.
 */


#ifndef TESTCALLWIDGET_H
#define TESTCALLWIDGET_H

#include <QWidget>
#include "sipmanager.h"
#include "sipcall.h"

namespace Ui
{
class TestCallWidget;
}

class TestCallWidget : public QWidget
{
	Q_OBJECT
	
public:
	explicit TestCallWidget(SipManager * manager, const Jid & stream);
	~TestCallWidget();
	
private slots:
	void on_call_clicked();
	void on_reject_clicked();
	void on_videoOn_stateChanged(int state);
	void on_soundOn_stateChanged(int state);
	void on_micOn_stateChanged(int state);
	void on_cameraOn_stateChanged(int state);
private:
	Ui::TestCallWidget *ui;
	SipManager * sipManager;
	ISipCall * call;
	Jid streamJid;
};

#endif // TESTCALLWIDGET_H
