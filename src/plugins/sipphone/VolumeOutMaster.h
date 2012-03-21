#ifndef VOLUMEOUTMASTER_H
#define VOLUMEOUTMASTER_H

#include <QObject>
#include <pjsua.h>


//#define Q_WS_WIN32_NOTNEEDED

#include "IVolume.h"
#ifndef Q_WS_WIN32
typedef unsigned long DWORD;
typedef unsigned int UINT;
#endif

// TODO: implement volume functions for Mac OS X
class CVolumeOutMaster : public QObject
		#ifdef Q_WS_WIN32_NOTNEEDED
		, public IVolume
		#endif
{
	Q_OBJECT
#ifdef Q_WS_WIN32_NOTNEEDED

	////////////////////////
	// IVolume interface
public:
	virtual bool	IsAvailable();
	virtual void	Enable();
	virtual void	Disable();
	virtual DWORD	GetVolumeMetric();
	virtual DWORD	GetMinimalVolume();
	virtual DWORD	GetMaximalVolume();
	virtual DWORD	GetCurrentVolume();
	virtual void	SetCurrentVolume( DWORD dwValue );
	//virtual void	RegisterNotificationSink( PONMICVOULUMECHANGE, DWORD );

	bool isMute();
#endif

public:
	CVolumeOutMaster();
	~CVolumeOutMaster();

	bool isAvailable();
	ulong volumeMetric();
	float minimalVolume();
	float maximalVolume();
	float currentVolume();
	bool isMuted();

	pjsua_call_id currentCall() const { return _currentCall; }
	void setCurrentCall(pjsua_call_id call) { _currentCall = call; setCurrentVolume(_currentVolume); }

public slots:
	void enable();
	void disable();
	void setCurrentVolume(float vol);

signals:
	void volumeChangedExternaly(int);
	void muteStateChangedExternaly(bool);

private:
	bool Init();
	void Done();

	bool Initialize();
	void EnableLine( bool bEnable = true );

protected:
	void timerEvent(QTimerEvent *evt);

private:
	// Status Info
	bool m_bOK;
	bool m_bInitialized;
	bool m_bAvailable;

	int m_updateTimer;
	DWORD m_currVolume;
	bool m_isMute;

	// Mixer Info
	UINT m_uMixerID;
	DWORD m_dwMixerHandle;

	DWORD m_dwLineID;
	DWORD m_dwVolumeControlID;
	int m_nChannelCount;
	
	//HWND	m_hWnd;
	//static	LRESULT CALLBACK MixerWndProc( HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam );
	//void	OnControlChanged( DWORD dwControlID );

	DWORD m_dwMinimalVolume;
	DWORD m_dwMaximalVolume;
	DWORD m_dwVolumeStep;

#ifdef Q_WS_WIN32_NOTNEEDED
	// User Info
	PONMICVOULUMECHANGE m_pfUserSink;
	DWORD m_dwUserValue;
#endif
	pjsua_call_id _currentCall;
	float _minVolume;
	float _maxVolume;
	float _currentVolume;
	float _lastVolume;
	bool _muted;
	bool _enabled;
};

typedef	CVolumeOutMaster*	PCVolumeOutMaster;
///////////////////////////////////////////////////////////////////////////////////////////////

#endif //VOLUMEOUTMASTER_H
