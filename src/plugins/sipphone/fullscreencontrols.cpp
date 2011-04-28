#include "fullscreencontrols.h"
#include <utils/iconstorage.h>
#include <utils/stylestorage.h>
#include <definitions/resources.h>
#include <definitions/menuicons.h>
#include <definitions/stylesheets.h>
#include <QPainter>


FullScreenControls::FullScreenControls( QWidget *parent)
	: QWidget(parent)
{
	ui.setupUi(this);

	IconStorage* iconStorage = IconStorage::staticStorage(RSR_STORAGE_MENUICONS);

	ui.wgtAVControl->setDark(false);

	setAutoFillBackground(true);

	connect(ui.wgtAVControl, SIGNAL(camStateChange(bool)), this, SIGNAL(camStateChange(bool)));
	connect(ui.wgtAVControl, SIGNAL(micStateChange(bool)), this, SIGNAL(micStateChange(bool)));
	connect(ui.wgtAVControl, SIGNAL(micVolumeChange(int)), this, SIGNAL(micVolumeChange(int)));


	QImage imgHangup = iconStorage->getImage(MNI_SIPPHONE_WHITE_HANGUP);
	QIcon iconHangup;
	iconHangup.addPixmap(QPixmap::fromImage(imgHangup), QIcon::Normal, QIcon::On);
	ui.btnHangup->setIcon(iconHangup);
	connect(ui.btnHangup, SIGNAL(clicked()), this, SIGNAL(hangup()));


	QImage imgOn = iconStorage->getImage(MNI_SIPPHONE_WHITE_FULLSCREEN_OFF);
	QImage imgOff = iconStorage->getImage(MNI_SIPPHONE_WHITE_FULLSCREEN_ON);

	QIcon iconFS;
	iconFS.addPixmap(QPixmap::fromImage(imgOn), QIcon::Normal, QIcon::On);
	iconFS.addPixmap(QPixmap::fromImage(imgOff), QIcon::Normal, QIcon::Off);
	ui.btnFullScreen->setIcon(iconFS);
	connect(ui.btnFullScreen, SIGNAL(clicked(bool)), this, SIGNAL(fullScreenState(bool)));
	StyleStorage::staticStorage(RSR_STORAGE_STYLESHEETS)->insertAutoStyle(this, STS_SIPPHONE);
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

void FullScreenControls::paintEvent(QPaintEvent *)
{
	QStyleOption opt;
	opt.init(this);
	QPainter p(this);
	style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
}
