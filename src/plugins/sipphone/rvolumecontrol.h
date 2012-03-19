#ifndef RVOLUMECONTROL_H
#define RVOLUMECONTROL_H

#include <QWidget>
#include <QList>

#ifdef Q_WS_WIN32
# include "winsock2.h"
//# include <stdio.h>
//# include <windows.h>
# include <mmdeviceapi.h>
# include <endpointvolume.h>
#else
typedef long LONG;
typedef int BOOL;
#endif

#include "VolumeOutMaster.h"

class CVolumeNotification : public QObject
#ifdef Q_WS_WIN32
		, public IAudioEndpointVolumeCallback
#endif
{
	Q_OBJECT
	LONG m_RefCount;
	~CVolumeNotification(void) {}
public:
	CVolumeNotification(void) : m_RefCount(1)
	{
		_currVolume = -1;
		_mute = false;
	}
#ifdef Q_WS_WIN32
	STDMETHODIMP_(ULONG)AddRef() { return InterlockedIncrement(&m_RefCount); }
	STDMETHODIMP_(ULONG)Release()
	{
		LONG ref = InterlockedDecrement(&m_RefCount);
		if (ref == 0)
			delete this;
		return ref;
	}
	STDMETHODIMP QueryInterface(REFIID IID, void **ReturnValue)
	{
		if (IID == IID_IUnknown || IID== __uuidof(IAudioEndpointVolumeCallback))
		{
			*ReturnValue = static_cast<IUnknown*>(this);
			AddRef();
			return S_OK;
		}
		*ReturnValue = NULL;
		return E_NOINTERFACE;
	}

	STDMETHODIMP OnNotify(PAUDIO_VOLUME_NOTIFICATION_DATA NotificationData);
#endif

signals:
	//void volumeChanged(double);
	void volumeChanged(int);
	void volumeMuted(bool);

private:
	int _currVolume;
	BOOL _mute;
};

class RVolumeControl : public QWidget
{
	Q_OBJECT

	enum VolumeIndexes
	{
		volume0,
		volume1,
		volume2,
		volume3,
		volume4,
		volumeOff,
		disableSound
	};
	//Q_PROPERTY(int minimum READ minimum WRITE setMinimum)
	//Q_PROPERTY(int maximum READ maximum WRITE setMaximum)
	//Q_PROPERTY(int value READ value WRITE setValue)

public:
	RVolumeControl(QWidget *parent);
	~RVolumeControl();

	int minimum() const;
	void setMinimum(int);

	int maximum() const;
	void setMaximum(int);

	int value() const;

	void setCallSlot(int slot);
	void resetCallSlot(int slot);

public slots:
	void setValue(int);
	void setOff();
	void setOn();
	void setMute(bool);
	void setEnableSound(bool);

signals:
	void valueChanged(int); // Изменение значения
	void stateChanged(bool isOn); // Изменение состояния (включен/выключен)

public:
	QSize sizeHint() const;
	QSize minimumSizeHint() const;
	void setDark(bool isDark);

protected:
	void paintEvent(QPaintEvent *ev);
	void mousePressEvent(QMouseEvent *ev);
	void mouseReleaseEvent(QMouseEvent *ev);
	void mouseMoveEvent(QMouseEvent *ev);
	void wheelEvent(QWheelEvent *);
protected:
	void updatePixmap( VolumeIndexes val );
	VolumeIndexes volumeToIndex(int val);
	void switchStateOnOff();
	//void initStyleOption(QStyleOptionSlider *option) const;

protected slots:
	void onValueChange(int);
	void onVolumeChange(double);
	void onVolumeChange(int);
	void onVolumeMuted(bool);

private:
	QPixmap _currPixmap;
	QList<QPixmap> _pixList;
	Q_DISABLE_COPY(RVolumeControl)
	int _value;
	int _min, _max;
	bool _isOn;
	bool _isEnableSound;
	bool _isDark;
#ifdef Q_WS_WIN32
	IAudioEndpointVolume *endpointVolume;
	CVolumeNotification *volumeNotification;
#endif
	void setValueNoCycle(int);


	// XP support
	bool _isWinXP;
	CVolumeOutMaster* pMasterVolume;
};

#endif // RVOLUMECONTROL_H
