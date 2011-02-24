#include "avcontrol.h"

AVControl::AVControl(QWidget *parent)
	: QWidget(parent)
{
	ui.setupUi(this);

	connect(ui.chkbtnCameraOn, SIGNAL(clicked(bool)), this, SIGNAL(camStateChange(bool)));

	connect(ui.chkbtnMicOn, SIGNAL(clicked(bool)), this, SIGNAL(micStateChange(bool)));
	connect(ui.hslMicVolume, SIGNAL(valueChanged(int)), this, SIGNAL(micVolumeChange(int)));
}

AVControl::~AVControl()
{

}

void AVControl::SetCameraOn(bool isOn)
{
	ui.chkbtnCameraOn->setChecked(isOn);
	emit camStateChange(isOn);
}