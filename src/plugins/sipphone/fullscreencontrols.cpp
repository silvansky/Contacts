#include "fullscreencontrols.h"

FullScreenControls::FullScreenControls( QWidget *parent)
	: QWidget(parent)
{
	ui.setupUi(this);


	connect(ui.wgtAVControl, SIGNAL(camStateChange(bool)), this, SIGNAL(camStateChange(bool)));
	connect(ui.wgtAVControl, SIGNAL(micStateChange(bool)), this, SIGNAL(micStateChange(bool)));
	connect(ui.wgtAVControl, SIGNAL(micVolumeChange(int)), this, SIGNAL(micVolumeChange(int)));

	connect(ui.btnHangup, SIGNAL(clicked()), this, SIGNAL(hangup()));

	connect(ui.btnFullScreen, SIGNAL(clicked(bool)), this, SIGNAL(fullScreenState(bool)));
}

FullScreenControls::~FullScreenControls()
{

}

void FullScreenControls::SetCameraOn(bool isOn)
{
	ui.wgtAVControl->SetCameraOn(isOn);
	//emit camStateChange(isOn);
}
void FullScreenControls::setFullScreen(bool state)
{
	ui.btnFullScreen->setChecked(state);
	emit fullScreenState(state);
}
