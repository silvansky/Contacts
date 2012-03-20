#ifndef SIPPHONEWIDGET_H
#define SIPPHONEWIDGET_H

#include <QWidget>
#include <QToolButton>
//#include <QDialog>
#include "ui_sipphonewidget.h"

//#include "qimagelabel.h"
//#include "fullscreencontrols.h"

//#include "rsipphone.h"

class RSipPhone;
class QImageLabel;
class FullScreenControls;

class SipPhoneWidget : public QWidget
{
	Q_OBJECT

public:
	SipPhoneWidget(QWidget *parent = 0);
	//SipPhoneWidget(KSipAuthentication *auth, CallAudio *callaudio, SipCall *initcall, SipPhoneProxy *parent, const char *name = 0);
	SipPhoneWidget( RSipPhone *parent, const char *name = 0);
	~SipPhoneWidget();

	QSize sizeHint();

protected:
	bool eventFilter(QObject *, QEvent *);

public:
	//SipCall *getCall();
	//void switchCall( SipCall *newcall );
	void setRemote( QString newremote );
	void clickDial( void );
	void clickHangup( void );
	//

public slots:
	//void pleaseDial( const SipUri &dialuri );
	//void callDeletedSlot( bool );
		//
signals:
		void callDeleted( bool );
		void hangupCall();
		//void redirectCall( const SipUri &calluri, const QString &subject );
		//void statusChanged(const QString&);
		//void audioStatusChanged(const QString&);
		//void callWasHangup();

private slots:
	//void hangupCall( void );
	//void dialClicked( void );
	void audioOutputDead( void );
	void acceptCall( void );
	//void callMemberStatusUpdated( void );
	//void updateAudioStatus( void );
	//void handleRedirect( void );
	//void getUri( void );
	// void ringTimeout( void );
	//void acceptCallTimeout( void );
	void remotePictureShow(const QImage&);
	void localPictureShow(const QImage&);
	void moveCurPicLabel(const QPoint &);

private:
	// Sip Stuff
	//SipCall *_pSipCall;
	//SipCallMember *_pSipCallMember;
	//KSipAuthentication *_pSipAuthentication;

	// Audio Stuff
	//CallAudio *_pAudioContoller;

	int _ringCount;
	QTimer *_pRingTimer;
	bool _isRingingTone;
	//QTimer *_pAcceptCallTimer;
	QString _subject; // Клиент кому звоним

	//// DTMF Stuff
	//QTimer *dtmfsenderTimer;

	//// GUI Stuff
	//QLabel *_pCurrentStatus;
	//QLabel *_pCurrentAudioStatus;
	////QPushButton *_pDialButton;
	//QPushButton *_pHangupButton;

	// State stuff
	enum CallState
	{
		PreDial,
		Calling,
		Connected,
		Called
	};
	CallState _currentState;

	//  // Private functions
	void forceDisconnect( void );
	bool _isHangupInitiator;


public slots:
	void SetCurrImage(const QImage& img);
	void SetRemoteImage(const QImage& img);

protected slots:
	void fullScreenStateChange(bool);
	void processOneMouseIdle();
	void adjustMySize();

signals:
	void startCamera();
	void stopCamera();
	void fullScreenState(bool);
	void signal_CameraStateChange(bool);

public slots:
	void cameraStateChange(bool);

protected:
	void keyPressEvent(QKeyEvent *);
	void mouseMoveEvent(QMouseEvent *);
	//virtual void focusInEvent(QFocusEvent *);
	//virtual void focusOutEvent(QFocusEvent *);
	virtual void enterEvent(QEvent *);
	virtual void leaveEvent(QEvent *);
	//virtual void moveEvent(QMoveEvent *);
	virtual void resizeEvent(QResizeEvent *);
	virtual void closeEvent(QCloseEvent *);

private:
	bool curPicMoved;
	Qt::Alignment curPicAlign;
	QImageLabel* _pCurrPic;
	QToolButton* _pShowCurrPic;
	FullScreenControls* _pControls;
	QTimer* _fsTimer;

private:
	Ui::SipPhoneWidget ui;
};

#endif // SIPPHONEWIDGET_H
