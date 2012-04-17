#include "callcontrolwidget.h"

CallControlWidget::CallControlWidget(IPluginManager *APluginManager, ISipCall *ASipCall, QWidget *AParent) : QWidget(AParent)
{
	ui.setupUi(this);
	StyleStorage::staticStorage(RSR_STORAGE_STYLESHEETS)->insertAutoStyle(this,STS_SIPPHONE_CALLCONTROLWIDGET);

	FSipCall = ASipCall;
	connect(FSipCall->instance(),SIGNAL(stateChanged(int)),SLOT(onCallStateChanged(int)));
	connect(FSipCall->instance(),SIGNAL(deviceStateChanged(ISipDevice::Type, ISipDevice::State)),SLOT(onCallDeviceStateChanged(ISipDevice::Type, ISipDevice::State)));

	initialize(APluginManager);
}

CallControlWidget::~CallControlWidget()
{

}

ISipCall *CallControlWidget::sipCall() const
{
	return FSipCall;
}

void CallControlWidget::initialize(IPluginManager *APluginManager)
{

}

void CallControlWidget::onCallStateChanged(int AState)
{

}

void CallControlWidget::onCallDeviceStateChanged(ISipDevice::Type AType, ISipDevice::State AState)
{

}
