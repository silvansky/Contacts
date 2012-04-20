/*
 * This file will be removed. I mean it.
 */

#include "testcallwidget.h"
#include "ui_testcallwidget.h"

#ifdef DEBUG_ENABLED
# include <QDebug>
#endif

TestCallWidget::TestCallWidget(SipManager * manager, const Jid &stream) :
	QWidget(NULL),
	ui(new Ui::TestCallWidget)
{
	call = NULL;
	sipManager = manager;
	streamJid = stream;
	ui->setupUi(this);
	ui->contact->setAttribute(Qt::WA_MacShowFocusRect, false);
	setStatusText("Ready.");
}

TestCallWidget::~TestCallWidget()
{
	delete ui;
}

void TestCallWidget::setPreview(const QPixmap &p)
{
	ui->preview->setPixmap(p);
}

void TestCallWidget::setRemoteImage(const QPixmap &p)
{
	ui->videoIn->setPixmap(p);
}

void TestCallWidget::setStatusText(const QString &status)
{
	ui->status->setText(status);
}

void TestCallWidget::on_call_clicked()
{
	Jid contact = ui->contact->text();
	call = sipManager->newCall(streamJid, QList<Jid>() << contact);
	connect(call->instance(), SIGNAL(stateChanged(int)), SLOT(onCallStateChanged(int)));
	connect(call->instance(), SIGNAL(activeDeviceChanged(ISipDevice::Type)), SLOT(onCallActiveDeviceChanged(ISipDevice::Type)));
	connect(call->instance(), SIGNAL(deviceStateChanged(ISipDevice::Type, ISipDevice::State)), SLOT(onCallDeviceStateChanged(ISipDevice::Type, ISipDevice::State)));
	connect(call->instance(), SIGNAL(devicePropertyChanged(ISipDevice::Type, int, const QVariant &)), SLOT(onCallDevicePropertyChanged(ISipDevice::Type, int, const QVariant &)));
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

void TestCallWidget::onCallStateChanged(int AState)
{
	switch (AState)
	{
	case ISipCall::CS_FINISHED:
		setStatusText("Call finished.");
		break;
	case ISipCall::CS_TALKING:
		setStatusText("Talking...");
		break;
	case ISipCall::CS_ERROR:
	{
		setStatusText(call->errorString());
#ifdef DEBUG_ENABLED
		qDebug() << "Call error: " + call->errorString();
#endif
		break;
	}
	default:
		setStatusText(QString("Call state: %1").arg(AState));
		break;
	}
}

void TestCallWidget::onCallActiveDeviceChanged(ISipDevice::Type ADeviceType)
{
	Q_UNUSED(ADeviceType)
}

void TestCallWidget::onCallDeviceStateChanged(ISipDevice::Type AType, ISipDevice::State AState)
{
	Q_UNUSED(AType)
	Q_UNUSED(AState)
}

void TestCallWidget::onCallDevicePropertyChanged(ISipDevice::Type AType, int AProperty, const QVariant &AValue)
{
	if (AType == ISipDevice::DT_LOCAL_CAMERA)
	{
		if (AProperty == ISipDevice::CP_CURRENTFRAME)
		{
			setPreview(AValue.value<QPixmap>());
		}
	}
	else if (AType == ISipDevice::DT_REMOTE_CAMERA)
	{
		if (AProperty == ISipDevice::VP_CURRENTFRAME)
		{
			setRemoteImage(AValue.value<QPixmap>());
		}
	}
}
