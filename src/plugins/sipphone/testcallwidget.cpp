/*
 * This file will be removed. I mean it.
 */

#include "testcallwidget.h"
#include "ui_testcallwidget.h"

TestCallWidget::TestCallWidget(SipManager * manager, const Jid &stream) :
	QWidget(NULL),
	ui(new Ui::TestCallWidget)
{
	call = NULL;
	sipManager = manager;
	streamJid = stream;
	ui->setupUi(this);
	ui->contact->setAttribute(Qt::WA_MacShowFocusRect, false);
}

TestCallWidget::~TestCallWidget()
{
	delete ui;
}

void TestCallWidget::on_call_clicked()
{
	Jid contact = ui->contact->text();
	call = sipManager->newCall(streamJid, QList<Jid>() << contact);
	call->startCall();
}

void TestCallWidget::on_reject_clicked()
{
	if (call)
	{
		call->rejectCall();
	}
}

void TestCallWidget::on_videoOn_stateChanged(int state)
{
	if (call)
	{
			call->setDeviceState(ISipDevice::DT_REMOTE_CAMERA, (state == Qt::Checked) ? ISipDevice::DS_ENABLED : ISipDevice::DS_DISABLED);
	}
}

void TestCallWidget::on_soundOn_stateChanged(int state)
{
	if (call)
	{
			call->setDeviceState(ISipDevice::DT_REMOTE_MICROPHONE, (state == Qt::Checked) ? ISipDevice::DS_ENABLED : ISipDevice::DS_DISABLED);
	}
}

void TestCallWidget::on_micOn_stateChanged(int state)
{
	if (call)
	{
			call->setDeviceState(ISipDevice::DT_LOCAL_MICROPHONE, (state == Qt::Checked) ? ISipDevice::DS_ENABLED : ISipDevice::DS_DISABLED);
	}
}

void TestCallWidget::on_cameraOn_stateChanged(int state)
{
	if (call)
	{
			call->setDeviceState(ISipDevice::DT_LOCAL_CAMERA, (state == Qt::Checked) ? ISipDevice::DS_ENABLED : ISipDevice::DS_DISABLED);
	}
}
