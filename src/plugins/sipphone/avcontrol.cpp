#include "avcontrol.h"
#include <utils/iconstorage.h>
#include <definitions/resources.h>
#include <definitions/menuicons.h>


BtnSynchro* AVControl::__bSync = NULL;

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
	: QWidget(parent)
{
	ui.setupUi(this);

	if(__bSync == NULL)
	{
		__bSync = new BtnSynchro(ui.chkbtnCameraOn);
	}
	else
	{
		__bSync->AddRef(ui.chkbtnCameraOn);
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


	//connect(ui.chkbtnCameraOn, SIGNAL(clicked(bool)), this, SIGNAL(camStateChange(bool)));
	connect(ui.chkbtnCameraOn, SIGNAL(toggled(bool)), this, SIGNAL(camStateChange(bool)));
	//toggled

	connect(ui.chkbtnMicOn, SIGNAL(clicked(bool)), this, SIGNAL(micStateChange(bool)));
	connect(ui.hslMicVolume, SIGNAL(valueChanged(int)), this, SIGNAL(micVolumeChange(int)));


	connect(ui.chkbtnCameraOn, SIGNAL(toggled(bool)), __bSync, SLOT(onStateChange(bool)));




}

AVControl::~AVControl()
{
	if(__bSync)
		__bSync->Release(ui.chkbtnCameraOn);
}

void AVControl::SetCameraOn(bool isOn)
{
	ui.chkbtnCameraOn->setChecked(isOn);
	emit camStateChange(isOn);
}