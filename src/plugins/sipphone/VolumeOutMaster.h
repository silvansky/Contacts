// VolumeOutMaster.h : Module interface declaration.
// IVolume implementation for master audio volume
// Developer : Alex Chmut
// Created : 8/11/98
#pragma once
#include "IVolume.h"
#include <QObject>

///////////////////////////////////////////////////////////////////////////////////////////////
class CVolumeOutMaster : public QObject, public IVolume
{
	Q_OBJECT

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


public:
	CVolumeOutMaster();
	~CVolumeOutMaster();

signals:
	void volumeChangedExternaly(int);
	void muteStateChangedExternaly(bool);

private:
	bool	Init();
	void	Done();

	bool	Initialize();
	void	EnableLine( bool bEnable = true );

protected:
	void timerEvent(QTimerEvent *evt);

private:
	// Status Info
	bool	m_bOK;
	bool	m_bInitialized;
	bool	m_bAvailable;

	int m_updateTimer;
	DWORD m_currVolume;
	bool m_isMute;

	// Mixer Info
	UINT	m_uMixerID;
	DWORD	m_dwMixerHandle;

	DWORD	m_dwLineID;
	DWORD	m_dwVolumeControlID;
	int		m_nChannelCount;
	
	//HWND	m_hWnd;
	//static	LRESULT CALLBACK MixerWndProc( HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam );
	//void	OnControlChanged( DWORD dwControlID );

	DWORD	m_dwMinimalVolume;
	DWORD	m_dwMaximalVolume;
	DWORD	m_dwVolumeStep;

	// User Info
	PONMICVOULUMECHANGE		m_pfUserSink;
	DWORD					m_dwUserValue;
};

typedef	CVolumeOutMaster*	PCVolumeOutMaster;
///////////////////////////////////////////////////////////////////////////////////////////////