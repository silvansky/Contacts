#include "rvolumecontrol.h"



#include <QPainter>
#include <QPaintEvent>
#include <QMouseEvent>
#include <math.h>

#include "utils/iconstorage.h"
#include "definitions/resources.h"
#include "definitions/menuicons.h"


double round(double x)
{
	return ((x - floor(x)) >= 0.5) ? ceil(x) : floor(x);
}



STDMETHODIMP CVolumeNotification::OnNotify(PAUDIO_VOLUME_NOTIFICATION_DATA NotificationData) 
{
	int volumeValue = (int)round(NotificationData->fMasterVolume*100);

	//if(_currVolume < 0)
	//{
	//	_currVolume = volumeValue;
	//	_mute = NotificationData->bMuted;
	//	emit volumeChanged(volumeValue);
	//	emit volumeMuted(NotificationData->bMuted);
	//	return S_OK;
	//}

	//
	//if(abs(_currVolume - volumeValue) >= 1)
	//{
	//	emit volumeChanged(volumeValue);
	//}
	//if(_mute != NotificationData->bMuted)
	//{
	//	emit volumeMuted(NotificationData->bMuted);
	//}

	_currVolume = volumeValue;
	_mute = NotificationData->bMuted;


	//if(!InlineIsEqualGUID(NotificationData->guidEventContext, GUID_NULL))
	{
		emit volumeChanged(_currVolume);
		emit volumeMuted(_mute);
	}

	return S_OK; 
} 



RVolumeControl::RVolumeControl(QWidget *parent)
: QWidget(parent), _value(0), _min(0), _max(100), _isEnableSound(true), _isOn(true), _isDark(true), _isWinXP(false)
{
	endpointVolume = NULL;
	volumeNotification = NULL;

	// ХАК ОТ ВАЛЕНТИНА
	setProperty("ignoreFilter", true);

	BOOL mute = false;
	float currVolume = 0.;

	// Определяем версию ОС (xp или старше?)
	OSVERSIONINFO m_osinfo;
	ZeroMemory(&m_osinfo, sizeof(m_osinfo));
	m_osinfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);

	if (GetVersionEx((LPOSVERSIONINFO) &m_osinfo))
	{
		if(m_osinfo.dwMajorVersion < 6)
		{
			_isWinXP = true;
		}
	}

	if(!_isWinXP)
	{
		CoInitialize(NULL);

		HRESULT hr;

		IMMDeviceEnumerator *deviceEnumerator = NULL;
		hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_INPROC_SERVER, __uuidof(IMMDeviceEnumerator), (LPVOID *)&deviceEnumerator);

		if(hr == S_OK)
		{
			IMMDevice *defaultDevice = NULL; 

			hr = deviceEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &defaultDevice);
			if(hr == S_OK)
			{
				deviceEnumerator->Release();
				deviceEnumerator = NULL; 

				//endpointVolume = NULL;
				hr = defaultDevice->Activate(__uuidof(IAudioEndpointVolume), CLSCTX_INPROC_SERVER, NULL, (LPVOID *)&endpointVolume);
				if(hr == S_OK)
				{
					defaultDevice->Release();
					defaultDevice = NULL; 

					volumeNotification = new CVolumeNotification(); 
					//connect(volumeNotification, SIGNAL(volumeChanged(double)), this, SLOT(onVolumeChange(double)));
					connect(volumeNotification, SIGNAL(volumeChanged(int)), this, SLOT(onVolumeChange(int)));
					connect(volumeNotification, SIGNAL(volumeMuted(bool)), this, SLOT(onVolumeMuted(bool)));
					hr = endpointVolume->RegisterControlChangeNotify(volumeNotification);

					endpointVolume->GetMute(&mute);
					endpointVolume->GetMasterVolumeLevelScalar(&currVolume);
				}
			}
		}
	}
	else
	{
		pMasterVolume = new CVolumeOutMaster();
		if ( !pMasterVolume || !pMasterVolume->IsAvailable() )
		{
			// обработка ошибки
		}
		pMasterVolume->Enable();
		connect(pMasterVolume, SIGNAL(volumeChangedExternaly(int)), this, SLOT(setValue(int)));
		connect(pMasterVolume, SIGNAL(muteStateChangedExternaly(bool)), this, SLOT(setMute(bool)));
	}

	//////////////////QString path = "D:\\CONCEPT\\VolumeControl\\VolumeControl\\icons\\";
	////////////////QString path = "D:\\icons\\";

	////////////////_pixList.append(QPixmap( path + "volume_0.png"));
	////////////////_pixList.append(QPixmap( path + "volume_1.png"));
	////////////////_pixList.append(QPixmap( path + "volume_2.png"));
	////////////////_pixList.append(QPixmap( path + "volume_3.png"));
	////////////////_pixList.append(QPixmap( path + "volume_4.png"));
	////////////////_pixList.append(QPixmap( path + "soundoff.png"));
	////////////////_pixList.append(QPixmap( path + "sounddisabled.png"));

	IconStorage* iconStorage = IconStorage::staticStorage(RSR_STORAGE_MENUICONS);

	QList<QString> data;
	data << MNI_SIPPHONE_SOUND_VOLUME0 << MNI_SIPPHONE_SOUND_VOLUME1 << MNI_SIPPHONE_SOUND_VOLUME2 << MNI_SIPPHONE_SOUND_VOLUME3
		<< MNI_SIPPHONE_SOUND_VOLUME4 << MNI_SIPPHONE_SOUND_OFF << MNI_SIPPHONE_SOUND_DISABLED;

	for(int i=0; i<data.size(); i++)
	{
		QIcon currentIcon = iconStorage->getIcon(data[i]);
		if(!currentIcon.isNull())
			_pixList.append(currentIcon.pixmap(42, 24, QIcon::Normal, QIcon::On));
	}

	if(_pixList.size() > 4)
		_currPixmap = _pixList[4];

	connect(this, SIGNAL(valueChanged(int)), this, SLOT(onValueChange(int)));

	//setMouseTracking(false);
	setValue(((int)(currVolume*100)));
	setMute(mute);
}

RVolumeControl::~RVolumeControl()
{
	if(!_isWinXP)
	{
		if(endpointVolume && volumeNotification)
		{
			endpointVolume->UnregisterControlChangeNotify(volumeNotification); 
			endpointVolume->Release();
			volumeNotification->Release();
		}
		CoUninitialize();
	}
	else
	{
		if(pMasterVolume)
		{
			delete pMasterVolume;
			pMasterVolume = NULL;
		}
	}
}


void RVolumeControl::setDark(bool isDark)
{
	if(_isDark == isDark)
		return;
	_isDark = isDark;

	IconStorage* iconStorage = IconStorage::staticStorage(RSR_STORAGE_MENUICONS);
	QList<QString> data;
	if(_isDark)
	{
		data << MNI_SIPPHONE_SOUND_VOLUME0 << MNI_SIPPHONE_SOUND_VOLUME1 << MNI_SIPPHONE_SOUND_VOLUME2 << MNI_SIPPHONE_SOUND_VOLUME3
			<< MNI_SIPPHONE_SOUND_VOLUME4 << MNI_SIPPHONE_SOUND_OFF << MNI_SIPPHONE_SOUND_DISABLED;
	}
	else
	{
		data << MNI_SIPPHONE_WHITE_SOUND_VOLUME0 << MNI_SIPPHONE_WHITE_SOUND_VOLUME1 << MNI_SIPPHONE_WHITE_SOUND_VOLUME2 << MNI_SIPPHONE_WHITE_SOUND_VOLUME3
			<< MNI_SIPPHONE_WHITE_SOUND_VOLUME4 << MNI_SIPPHONE_WHITE_SOUND_OFF << MNI_SIPPHONE_WHITE_SOUND_DISABLED;
	}

	_pixList.clear();
	for(int i=0; i<data.size(); i++)
	{
		QIcon currentIcon = iconStorage->getIcon(data[i]);
		if(!currentIcon.isNull())
			_pixList.append(currentIcon.pixmap(42, 24, QIcon::Normal, QIcon::On));
	}

	updatePixmap(volumeToIndex(_value));
}


int RVolumeControl::value() const
{
	return _value;
}

/*slot*/
void RVolumeControl::setValue( int val )
{
	if( val < minimum() )
	{
		val = minimum();
	}
	if( val > maximum() )
		val = maximum();

	updatePixmap(volumeToIndex(val));
	if( val != _value )
	{
		_value = val;
		emit valueChanged( _value );
	}
}

void RVolumeControl::setValueNoCycle(int val)
{
	if( val < minimum() )
	{
		val = minimum();
	}
	if( val > maximum() )
		val = maximum();

	updatePixmap(volumeToIndex(val));
	if( val != _value )
	{
		_value = val;
		//emit valueChanged( _value );
	}
}


void RVolumeControl::setMute(bool mute)
{
	if(mute)
	{
		setOff();
	}
	else
	{
		setOn();
	}
}

void RVolumeControl::setOff()
{
	_isOn = false;
	if(_isWinXP)
	{
		pMasterVolume->Disable();
		//pMasterVolume->Mute(true);
	}
	else
	{
		if(endpointVolume)
			endpointVolume->SetMute(true, NULL);
	}

	updatePixmap(volumeOff);
	emit stateChanged(false);
}

void RVolumeControl::setOn()
{
	_isOn = true;
	if(_isWinXP)
	{
		pMasterVolume->Enable();
	}
	else
	{
		if(endpointVolume)
			endpointVolume->SetMute(false, NULL);
	}

	updatePixmap(volumeToIndex(_value));
	emit stateChanged(true);
}

void RVolumeControl::setEnableSound(bool val)
{
	_isEnableSound = val;
	if(!_isEnableSound)
		updatePixmap(VolumeIndexes::disableSound);
	else
	{
		if(_isOn)
		{
			updatePixmap(volumeToIndex(_value));
		}
		else
		{
			updatePixmap(volumeOff);
		}
	}
}

int RVolumeControl::minimum() const
{
	return _min;
}
void RVolumeControl::setMinimum(int minValue)
{
	if(minValue != _min)
	{
		_min = minValue;
		if(_min > _max)
			_max = _min;
	}
}

int RVolumeControl::maximum() const
{
	return _max;
}
void RVolumeControl::setMaximum(int maxValue)
{
	if(_max != maxValue)
	{
		_max = maxValue;
		if(_max < _min)
		{
			_max = _min;
		}
	}
}

void RVolumeControl::paintEvent(QPaintEvent *ev)
{
	QPainter painter(this);

	QRect currRect = rect();
	painter.drawPixmap(currRect, _currPixmap);
}

void RVolumeControl::mousePressEvent(QMouseEvent *ev)
{
	if(!_isEnableSound)
		return;

	if(ev->button() == Qt::LeftButton)
	{
		QRect currRect = rect();
		
		int mXZerro = currRect.width() / 4; // 1/4 часть занимает динамик

		if(ev->x() < mXZerro)
		{
			switchStateOnOff();
			return;
		}

		if(_isOn)
		{
			int dif1 = (int)(_max - _min);
			int dif2 = (int)(currRect.width() - mXZerro);
			int res = ((ev->x() - mXZerro) * dif1) / dif2;
			if(res > _max - 5)
				res = _max;

			setValue(res);
		}

	}
	
}
void RVolumeControl::mouseReleaseEvent(QMouseEvent *ev)
{

}
void RVolumeControl::mouseMoveEvent(QMouseEvent *ev)
{

	Qt::MouseButton btn = ev->button();
	Qt::MouseButtons btns = ev->buttons();

	if(!_isEnableSound || !_isOn)
		return;

	if(btn != Qt::LeftButton && btns != Qt::LeftButton)
		return;


	QRect currRect = rect();

	int mXZerro = currRect.width() / 4; // 1/4 часть занимает динамик

	if(ev->x() < mXZerro)
	{
		if(mXZerro - ev->x() < 5 )
			setValue(_min);
		return;
	}
	else if(ev->x() >= currRect.width())
	{
		setValue(_max);
		return;
	}

	int dif1 = (int)(_max - _min);
	int dif2 = (int)(currRect.width() - mXZerro);
	int res = ((ev->x() - mXZerro) * dif1) / dif2;
	if(res > _max - 5)
		res = _max;

	setValue(res);
}

QSize RVolumeControl::sizeHint() const
{
    //ensurePolished();
    //const int SliderLength = 84/*, TickSpace = 5*/;

    //int thick = 20;// style()->pixelMetric(QStyle::PM_SliderThickness, &opt, this);
    //int w = thick, h = SliderLength;
    //if (d->orientation == Qt::Horizontal) {
    //    w = SliderLength;
    //    h = thick;
    //}
    //return QSize(w, h);//style()->sizeFromContents(QStyle::CT_Slider, &opt, QSize(w, h), this).expandedTo(QApplication::globalStrut());
	if(_currPixmap.isNull())
		return QSize(42, 24);
	return _currPixmap.size();
	//return QSize(0,0);
}


QSize RVolumeControl::minimumSizeHint() const
{
    //QSize s = sizeHint();

    //int length = 40;//style()->pixelMetric(QStyle::PM_SliderLength, &opt, this);
    //if (d->orientation == Qt::Horizontal)
    //    s.setWidth(length);
    //else
    //    s.setHeight(length);
    //return s;
	return _currPixmap.size();
	//return QSize(0,0);
}

RVolumeControl::VolumeIndexes RVolumeControl::volumeToIndex(int val)
{
	if(val >= 0 && val < 20)
		return volume0;
	if(val >= 20 && val < 40)
		return volume1;
	if(val >= 40 && val < 60)
		return volume2;
	if(val >= 60 && val < 80)
		return volume3;
	if(val >= 80 && val <= 100)
		return volume4;

	return volumeOff;
}

void RVolumeControl::updatePixmap( VolumeIndexes idx)
{
	if(!_isEnableSound)
	{
		if(_pixList.size() > disableSound)
			_currPixmap = _pixList[disableSound];
		update(rect());
		return;
	}
	if(_pixList.size() > idx)
		_currPixmap = _pixList[idx];
	update(rect());
}

void RVolumeControl::switchStateOnOff()
{
	if(_isOn)
	{
		setOff();
	}
	else
	{
		setOn();
	}
}


void RVolumeControl::onValueChange(int newVolumeInt)
{
	double newVolume = ((double)newVolumeInt)/100;

	if(_isWinXP)
	{
		DWORD val;
		int val65 = (int)(65535 * newVolumeInt / 100);
		if(pMasterVolume)
		{
			val = pMasterVolume->GetCurrentVolume();
			pMasterVolume->SetCurrentVolume(val65);
			val = pMasterVolume->GetCurrentVolume();
		}
	}
	else
	{
		if(endpointVolume)
			endpointVolume->SetMasterVolumeLevelScalar((float)newVolume, NULL);
	}

	//////////////double newVolume = ((double)newVolumeInt)/100;

	//////////////if(endpointVolume)
	//////////////	endpointVolume->SetMasterVolumeLevelScalar((float)newVolume, NULL);
}

void RVolumeControl::onVolumeChange(double val)
{
	int newVal = (int)(val*100);
	if(abs(newVal - _value) <= 2)
		return;

	setValueNoCycle( newVal );
}

void RVolumeControl::onVolumeChange(int val)
{
	int newVal = val;
	if(abs(newVal - _value) < 1)
		return;

	setValueNoCycle( newVal );
}

void RVolumeControl::onVolumeMuted(bool muted)
{
	if(!_isOn == muted)
	{
		return;
	}

	setMute(muted);
}