/*
 * This file will be removed. I mean it.
 */


#ifndef TESTCALLWIDGET_H
#define TESTCALLWIDGET_H

#include "sipmanager.h"
#include "sipcall.h"

#include <QWidget>
#include <QPixmap>

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
	void setPreview(const QPixmap & p);
	void setRemoteImage(const QPixmap & p);
	void setStatusText(const QString & status);
private slots:
	void on_call_clicked();
	void on_reject_clicked();
	void on_videoOn_stateChanged(int state);
	void on_soundOn_stateChanged(int state);
	void on_micOn_stateChanged(int state);
	void on_cameraOn_stateChanged(int state);
	// call
	void onCallStateChanged(int AState);
	void onCallActiveDeviceChanged(ISipDevice::Type ADeviceType);
	void onCallDeviceStateChanged(ISipDevice::Type AType, ISipDevice::State AState);
	void onCallDevicePropertyChanged(ISipDevice::Type AType, int AProperty, const QVariant & AValue);
private:
	Ui::TestCallWidget *ui;
	SipManager * sipManager;
	ISipCall * call;
	Jid streamJid;
};

#endif // TESTCALLWIDGET_H
