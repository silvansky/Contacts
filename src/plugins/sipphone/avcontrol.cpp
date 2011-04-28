#include "avcontrol.h"
#include <utils/iconstorage.h>
#include <definitions/resources.h>
#include <definitions/menuicons.h>
#include <QPainter>

//#include <QMessageBox>

BtnSynchro* AVControl::__bSyncCamera = NULL;
BtnSynchro* AVControl::__bSyncMic = NULL;

void BtnSynchro::onStateChange(bool state)
{
	foreach(QAbstractButton* btn, _buttons)
	{
		if(btn == sender())
			continue;
		btn->setChecked(state);
	}
}


AVControl::AVControl(QWidget *parent)
	: QWidget(parent), _isDark(true)
{
	ui.setupUi(this);

	if(__bSyncCamera == NULL)
	{
		__bSyncCamera = new BtnSynchro(ui.chkbtnCameraOn);
	}
	else
	{
		__bSyncCamera->AddRef(ui.chkbtnCameraOn);
	}

	if(__bSyncMic == NULL)
	{
		__bSyncMic = new BtnSynchro(ui.chkbtnMicOn);
	}
	else
	{
		__bSyncMic->AddRef(ui.chkbtnMicOn);
	}

	StyleStorage::staticStorage(RSR_STORAGE_STYLESHEETS)->insertAutoStyle(this, STS_SIPPHONE);

	IconStorage* iconStorage = IconStorage::staticStorage(RSR_STORAGE_MENUICONS);

	QIcon iconOn = iconStorage->getIcon(MNI_SIPPHONE_CAM_ON);
	QIcon iconOff = iconStorage->getIcon(MNI_SIPPHONE_CAM_OFF);
	QIcon iconDisabled = iconStorage->getIcon(MNI_SIPPHONE_CAM_DISABLED);

	QImage imgOn = iconStorage->getImage(MNI_SIPPHONE_CAM_ON);
	QImage imgOff = iconStorage->getImage(MNI_SIPPHONE_CAM_OFF);
	QImage imgDisabled = iconStorage->getImage(MNI_SIPPHONE_CAM_DISABLED);

		//if(!currentIcon.isNull())
			//_pixList.append(currentIcon.pixmap(42, 24, QIcon::Normal, QIcon::On));

	QIcon icon;
	icon.addPixmap(QPixmap::fromImage(imgOn), QIcon::Normal, QIcon::On);
	icon.addPixmap(QPixmap::fromImage(imgOff), QIcon::Normal, QIcon::Off);
	icon.addPixmap(QPixmap::fromImage(imgDisabled), QIcon::Disabled, QIcon::On);
	icon.addPixmap(QPixmap::fromImage(imgDisabled), QIcon::Disabled, QIcon::Off);
	ui.chkbtnCameraOn->setIcon(icon);


	QImage imgAOn = iconStorage->getImage(MNI_SIPPHONE_MIC_ON);
	QImage imgAOff = iconStorage->getImage(MNI_SIPPHONE_MIC_OFF);
	QImage imgADisabled = iconStorage->getImage(MNI_SIPPHONE_MIC_DISABLED);

	QIcon iconAudio;
	iconAudio.addPixmap(QPixmap::fromImage(imgAOn), QIcon::Normal, QIcon::On);
	iconAudio.addPixmap(QPixmap::fromImage(imgAOff), QIcon::Normal, QIcon::Off);
	iconAudio.addPixmap(QPixmap::fromImage(imgADisabled), QIcon::Disabled, QIcon::On);
	iconAudio.addPixmap(QPixmap::fromImage(imgADisabled), QIcon::Disabled, QIcon::Off);
	ui.chkbtnMicOn->setIcon(iconAudio);

	//ui.chkbtnCameraOn->setEnabled(false);
	//ui.chkbtnMicOn->setEnabled(false);
	ui.chkbtnMicOn->setChecked(true);


	//connect(ui.chkbtnCameraOn, SIGNAL(clicked(bool)), this, SIGNAL(camStateChange(bool)));
	connect(ui.chkbtnCameraOn, SIGNAL(toggled(bool)), this, SIGNAL(camStateChange(bool)));

	connect(ui.chkbtnMicOn, SIGNAL(toggled(bool)), this, SIGNAL(micStateChange(bool)));
	//connect(ui.chkbtnMicOn, SIGNAL(toggled(bool)), this, SLOT(onMicStateChange(bool)));

	connect(ui.hslSoundVolume, SIGNAL(valueChanged(int)), this, SIGNAL(micVolumeChange(int)));


	connect(ui.chkbtnCameraOn, SIGNAL(toggled(bool)), __bSyncCamera, SLOT(onStateChange(bool)));
	connect(ui.chkbtnMicOn, SIGNAL(toggled(bool)), __bSyncMic, SLOT(onStateChange(bool)));
}



AVControl::~AVControl()
{
	if(__bSyncCamera)
		if(__bSyncCamera->Release(ui.chkbtnCameraOn) == 0)
			__bSyncCamera = NULL;

	if(__bSyncMic)
		if(__bSyncMic->Release(ui.chkbtnMicOn) == 0)
			__bSyncMic = NULL;
}

void AVControl::SetCameraOn(bool isOn)
{
	ui.chkbtnCameraOn->setChecked(isOn);
	emit camStateChange(isOn);
}

void AVControl::setDark(bool isDark)
{
	if(_isDark == isDark)
		return;
	_isDark = isDark;

	IconStorage* iconStorage = IconStorage::staticStorage(RSR_STORAGE_MENUICONS);

	QImage imgOn = iconStorage->getImage(MNI_SIPPHONE_CAM_ON);
	QImage imgOff = iconStorage->getImage(MNI_SIPPHONE_CAM_OFF);
	QImage imgDisabled = iconStorage->getImage(MNI_SIPPHONE_CAM_DISABLED);

	QImage imgAOn = iconStorage->getImage(MNI_SIPPHONE_MIC_ON);
	QImage imgAOff = iconStorage->getImage(MNI_SIPPHONE_MIC_OFF);
	QImage imgADisabled = iconStorage->getImage(MNI_SIPPHONE_MIC_DISABLED);

	if(!_isDark)
	{
		imgOn = iconStorage->getImage(MNI_SIPPHONE_WHITE_CAM_ON);
		imgOff = iconStorage->getImage(MNI_SIPPHONE_WHITE_CAM_OFF);
		imgDisabled = iconStorage->getImage(MNI_SIPPHONE_WHITE_CAM_DISABLED);

		imgAOn = iconStorage->getImage(MNI_SIPPHONE_WHITE_MIC_ON);
		imgAOff = iconStorage->getImage(MNI_SIPPHONE_WHITE_MIC_OFF);
		imgADisabled = iconStorage->getImage(MNI_SIPPHONE_WHITE_MIC_DISABLED);
	}

	QIcon icon;
	icon.addPixmap(QPixmap::fromImage(imgOn), QIcon::Normal, QIcon::On);
	icon.addPixmap(QPixmap::fromImage(imgOff), QIcon::Normal, QIcon::Off);
	icon.addPixmap(QPixmap::fromImage(imgDisabled), QIcon::Disabled, QIcon::On);
	icon.addPixmap(QPixmap::fromImage(imgDisabled), QIcon::Disabled, QIcon::Off);
	ui.chkbtnCameraOn->setIcon(icon);

	QIcon iconAudio;
	iconAudio.addPixmap(QPixmap::fromImage(imgAOn), QIcon::Normal, QIcon::On);
	iconAudio.addPixmap(QPixmap::fromImage(imgAOff), QIcon::Normal, QIcon::Off);
	iconAudio.addPixmap(QPixmap::fromImage(imgADisabled), QIcon::Disabled, QIcon::On);
	iconAudio.addPixmap(QPixmap::fromImage(imgADisabled), QIcon::Disabled, QIcon::Off);
	ui.chkbtnMicOn->setIcon(iconAudio);

	ui.hslSoundVolume->setDark(isDark);
}

void AVControl::paintEvent(QPaintEvent *)
{
	QStyleOption opt;
	opt.init(this);
	QPainter p(this);
	style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
}
