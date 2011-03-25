#include "fullscreencontrols.h"
#include <utils/iconstorage.h>
#include <definitions/resources.h>
#include <definitions/menuicons.h>


FullScreenControls::FullScreenControls( QWidget *parent)
	: QWidget(parent)
{
	ui.setupUi(this);

	IconStorage* iconStorage = IconStorage::staticStorage(RSR_STORAGE_MENUICONS);

	ui.wgtAVControl->setDark(false);

	connect(ui.wgtAVControl, SIGNAL(camStateChange(bool)), this, SIGNAL(camStateChange(bool)));
	connect(ui.wgtAVControl, SIGNAL(micStateChange(bool)), this, SIGNAL(micStateChange(bool)));
	connect(ui.wgtAVControl, SIGNAL(micVolumeChange(int)), this, SIGNAL(micVolumeChange(int)));


	QImage imgHangup = iconStorage->getImage(MNI_SIPPHONE_WHITE_HANGUP);
	QIcon iconHangup;
	iconHangup.addPixmap(QPixmap::fromImage(imgHangup), QIcon::Normal, QIcon::On);
	ui.btnHangup->setIcon(iconHangup);
	connect(ui.btnHangup, SIGNAL(clicked()), this, SIGNAL(hangup()));


	QImage imgOn = iconStorage->getImage(MNI_SIPPHONE_WHITE_FULLSCREEN_ON);
	QImage imgOff = iconStorage->getImage(MNI_SIPPHONE_WHITE_FULLSCREEN_OFF);

	QIcon iconFS;
	iconFS.addPixmap(QPixmap::fromImage(imgOn), QIcon::Normal, QIcon::On);
	iconFS.addPixmap(QPixmap::fromImage(imgOff), QIcon::Normal, QIcon::Off);
	ui.btnFullScreen->setIcon(iconFS);
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
