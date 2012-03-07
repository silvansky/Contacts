#include "avcontrol.h"
#include <utils/iconstorage.h>
#include <definitions/resources.h>
#include <definitions/menuicons.h>
#include <QPainter>
#include <QProcess>

BtnSynchro* AVControl::__bSyncCamera = NULL;
BtnSynchro* AVControl::__bSyncMic = NULL;
BtnSynchro* AVControl::__bSyncHQ = NULL;

void BtnSynchro::setCheckState(bool state)
{
	foreach(QAbstractButton* btn, _buttons)
		btn->setChecked(state);
}

void BtnSynchro::setEnabledState(bool state)
{
	foreach(QAbstractButton* btn, _buttons)
		btn->setEnabled(state);
}

void BtnSynchro::setToolTipState(const QString &state)
{
	foreach(QAbstractButton* btn, _buttons)
		btn->setToolTip(state);
}

BtnSynchro::BtnSynchro( QAbstractButton* btn ) : _refCount(1)
{
	_buttons.append(btn);
}

int BtnSynchro::AddRef( QAbstractButton* btn )
{
	if(!_buttons.contains(btn)) // Не реально что такое может случиться, но тем не менее
	{
		QAbstractButton *curButton = _buttons.value(0);
		if (curButton)
		{
			btn->setChecked(curButton->isChecked());
			btn->setEnabled(curButton->isEnabled());
			btn->setToolTip(curButton->toolTip());
		}
		_buttons.append(btn);
	}
	return _refCount++;
}

int BtnSynchro::Release( QAbstractButton* btn )
{
	_refCount--;
	_buttons.removeOne(btn);
	if (_refCount == 0)
	{
		delete this;
		return 0;
	}
	return _refCount;
}


AVControl::AVControl(QWidget *parent) : QWidget(parent)
{
	ui.setupUi(this);
	StyleStorage::staticStorage(RSR_STORAGE_STYLESHEETS)->insertAutoStyle(this, STS_SIPPHONE);

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

	if(__bSyncHQ == NULL)
	{
		__bSyncHQ = new BtnSynchro(ui.chkbtnHQ);
	}
	else
	{
		__bSyncHQ->AddRef(ui.chkbtnHQ);
	}

	// ОТКЛЮЧИЛ HQ
	ui.chkbtnHQ->setEnabled(false);
	ui.chkbtnHQ->setVisible(false);
	connect(ui.chkbtnHQ, SIGNAL(toggled(bool)), SLOT(setResolutionHigh(bool)));
	connect(ui.chkbtnCameraOn, SIGNAL(toggled(bool)), SLOT(setCameraOn(bool)));
	connect(ui.chkbtnMicOn, SIGNAL(toggled(bool)), SLOT(setMicOn(bool)));
	connect(ui.hslSoundVolume, SIGNAL(valueChanged(int)), this, SIGNAL(micVolumeChange(int)));
	connect(ui.btnAudioSettings, SIGNAL(clicked()), this, SLOT(onAudioSettings()));

	ui.btnAudioSettings->setVisible(false);

	setDark(true);
	setMicOn(true);
	setCameraEnabled(false);
}

AVControl::~AVControl()
{
	if(__bSyncCamera)
		if(__bSyncCamera->Release(ui.chkbtnCameraOn) == 0)
			__bSyncCamera = NULL;

	if(__bSyncMic)
		if(__bSyncMic->Release(ui.chkbtnMicOn) == 0)
			__bSyncMic = NULL;

	if(__bSyncHQ)
		if(__bSyncHQ->Release(ui.chkbtnHQ) == 0)
			__bSyncHQ = NULL;
}

void AVControl::setDark(bool isDark)
{
	IconStorage* iconStorage = IconStorage::staticStorage(RSR_STORAGE_MENUICONS);

	QIcon camIcon;
	camIcon.addPixmap(QPixmap::fromImage(iconStorage->getImage(isDark ? MNI_SIPPHONE_CAM_ON : MNI_SIPPHONE_WHITE_CAM_ON)), QIcon::Normal, QIcon::On);
	camIcon.addPixmap(QPixmap::fromImage(iconStorage->getImage(isDark ? MNI_SIPPHONE_CAM_OFF : MNI_SIPPHONE_WHITE_CAM_OFF)), QIcon::Normal, QIcon::Off);
	camIcon.addPixmap(QPixmap::fromImage(iconStorage->getImage(isDark ? MNI_SIPPHONE_CAM_DISABLED : MNI_SIPPHONE_WHITE_CAM_DISABLED)), QIcon::Disabled, QIcon::On);
	camIcon.addPixmap(QPixmap::fromImage(iconStorage->getImage(isDark ? MNI_SIPPHONE_CAM_DISABLED : MNI_SIPPHONE_WHITE_CAM_DISABLED)), QIcon::Disabled, QIcon::Off);
	ui.chkbtnCameraOn->setIcon(camIcon);

	QIcon micIcon;
	micIcon.addPixmap(QPixmap::fromImage(iconStorage->getImage(isDark ? MNI_SIPPHONE_MIC_ON : MNI_SIPPHONE_WHITE_MIC_ON)), QIcon::Normal, QIcon::On);
	micIcon.addPixmap(QPixmap::fromImage(iconStorage->getImage(isDark ? MNI_SIPPHONE_MIC_OFF : MNI_SIPPHONE_WHITE_MIC_OFF)), QIcon::Normal, QIcon::Off);
	micIcon.addPixmap(QPixmap::fromImage(iconStorage->getImage(isDark ? MNI_SIPPHONE_MIC_DISABLED : MNI_SIPPHONE_WHITE_MIC_DISABLED)), QIcon::Disabled, QIcon::On);
	micIcon.addPixmap(QPixmap::fromImage(iconStorage->getImage(isDark ? MNI_SIPPHONE_MIC_DISABLED : MNI_SIPPHONE_WHITE_MIC_DISABLED)), QIcon::Disabled, QIcon::Off);
	ui.chkbtnMicOn->setIcon(micIcon);

	ui.hslSoundVolume->setDark(isDark);
}

void AVControl::updateMicToolTip()
{
	if (!ui.chkbtnMicOn->isEnabled())
		__bSyncMic->setToolTipState(tr("Failed to find the microphone"));
	else if (ui.chkbtnMicOn->isChecked())
		__bSyncMic->setToolTipState(tr("Turn off microphone"));
	else
		__bSyncMic->setToolTipState(tr("Turn on microphone"));
}

void AVControl::updateCamToolTip()
{
	if (!ui.chkbtnCameraOn->isEnabled())
		__bSyncCamera->setToolTipState(tr("Failed to find a web camera on your computer"));
	else if (ui.chkbtnCameraOn->isChecked())
		__bSyncCamera->setToolTipState(tr("Turn off web camera"));
	else
		__bSyncCamera->setToolTipState(tr("Turn on web camera"));
}

void AVControl::updateHQToolTip()
{
	if (!ui.chkbtnHQ->isEnabled())
		__bSyncHQ->setToolTipState(tr("High quality video is not available"));
	else if (ui.chkbtnHQ->isChecked())
		__bSyncHQ->setToolTipState(tr("Turn off high quality video"));
	else
		__bSyncHQ->setToolTipState(tr("Turn on high quality video"));
}

void AVControl::setCameraOn(bool isOn)
{
	__bSyncCamera->setCheckState(isOn);
	updateCamToolTip();
	emit camStateChange(isOn);
}

void AVControl::setCameraEnabled(bool isEnabled)
{
	__bSyncCamera->setEnabledState(isEnabled);
	updateCamToolTip();
	__bSyncHQ->setEnabledState(isEnabled);
	updateHQToolTip();
	emit camPresentChanged(isEnabled);
}


void AVControl::setResolutionHigh(bool isHigh)
{
	__bSyncHQ->setCheckState(isHigh);
	updateHQToolTip();
	emit camResolutionChange(isHigh);
}

void AVControl::setMicOn(bool isOn)
{
	__bSyncMic->setCheckState(isOn);
	updateMicToolTip();
	emit micStateChange(isOn);
}

void AVControl::setMicEnabled( bool isEnabled )
{
	__bSyncMic->setEnabledState(isEnabled);
	updateMicToolTip();
	emit micPresentChanged(isEnabled);
}

void AVControl::setVolumeEnabled( bool isEnabled )
{
	ui.hslSoundVolume->setEnabled(isEnabled);
	emit volumePresentChanged(isEnabled);
}

void AVControl::paintEvent(QPaintEvent *)
{
	QStyleOption opt;
	opt.init(this);
	QPainter p(this);
	style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
}

void AVControl::onAudioSettings()
{
#ifdef Q_WS_WIN32
	// ONLY FOR WINDOWS
	OSVERSIONINFO m_osinfo;
	ZeroMemory(&m_osinfo, sizeof(m_osinfo));
	m_osinfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	if (GetVersionEx((LPOSVERSIONINFO) &m_osinfo))
	{
		if(m_osinfo.dwMajorVersion < 6)
		{
			QProcess::startDetached("sndvol32.exe");
		}
		else
		{
			QProcess::startDetached("sndvol.exe");
		}
	}
#elif defined (Q_WS_MAC)
	// TODO: open sound preferences pane
	QProcess::startDetached("open -W \"/Applications/System Preferences.app\"");
#endif
}
