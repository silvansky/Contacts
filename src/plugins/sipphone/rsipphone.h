#ifndef RSIPPHONE_H
#define RSIPPHONE_H

#include <QObject>
#include <QImage>
//#include "sipphonewidget.h"

#include <pjsua.h>


//class VidWin;

class SipPhoneWidget;

class RSipPhone : public QObject
{
	Q_OBJECT

public:
	RSipPhone(QObject *parent = NULL);
	~RSipPhone();

	static RSipPhone *instance() { return _pInstance;}

public:
	//bool initStack();
	bool initStack(const char* sip_domain, int sipPortNum, const char* sip_username, const char* sip_password);
	bool initStack(const QString& sip_domain, int sipPortNum, const QString& sip_username, const QString& sip_password);

	bool initStack(const char* sip_server, int sipPortNum, const char* sip_username, const char* sip_password, const char* sip_domain);
	bool initStack(const QString& sip_server, int sipPortNum, const QString& sip_username, const QString& sip_password, const QString& sip_domain);

	void showError(const char *title, pj_status_t status);
	void showStatus(const char *msg);

	void registerAccount(bool);
	bool regStatus() const { return _currentRegisterStatus; }


	void on_reg_state(pjsua_acc_id acc_id);
	void on_reg_state2(pjsua_acc_id acc_id, pjsua_reg_info *info);
	void on_call_state(pjsua_call_id call_id, pjsip_event *e);
	void on_incoming_call(pjsua_acc_id acc_id, pjsua_call_id call_id, pjsip_rx_data *rdata);
	void on_call_media_state(pjsua_call_id call_id);
	void on_call_tsx_state(pjsua_call_id call_id, pjsip_transaction *tsx, pjsip_event *e);

	pj_status_t on_my_put_frame_callback(pjmedia_frame *frame, int w, int h, int stride);
	pj_status_t on_my_preview_frame_callback(pjmedia_frame *frame, const char*, int w, int h, int stride);


signals:
	void signalNewCall(int, bool);
	void signalCallReleased();
	void signalInitVideoWindow();
	void signalShowStatus(const QString&);

	void signalRegistrationStatusChanged(bool);

	void previewStatusChanged(bool isPreview);

	void signalVideoPrevWidgetSet(QWidget*);
	void signalVideoInputWidgetSet(QWidget*);

	void signalVideoPrevWidgetSet(void*);
	void signalVideoInputWidgetSet(void*);


	// Сигналы для совместимости с прежней версией (Уберу)
	void camPresentChanged(bool);
	void micPresentChanged(bool);
	void volumePresentChanged(bool);

	void proxyStartCamera();
	void proxyStopCamera();
	void proxyCamResolutionChange(bool);
	void proxySuspendStateChange(bool);

	void signalShowSipPhoneWidget(void* hwnd);

	void signal_SetRomoteImage(const QImage&);
	void signal_SetCurrentImage(const QImage&);
	


public slots:
		void preview();
		void call();
		void call(const char* uri);
		void call(const QString& uri);

		void hangup();
		void onShowSipPhoneWidget(void* hwnd);
		void cleanup();
		//void quit();
		bool sendVideo(bool isSending);

private slots:
		void onNewCall(int cid, bool incoming);
		void onCallReleased();
		void initVideoWindow();
		
		//void doShowStatus(const QString& msg);

private:
	bool isCameraReady() const;

private:
	static RSipPhone *_pInstance;
	pjsua_acc_id _accountId;
	pjsua_call_id _currentCall;
	bool _is_preview_on;

	bool _currentRegisterStatus;


	enum callrole { INCOMMING, OUTGOING, NONE};
	callrole _currentRole;

private:
	//QPushButton *callButton_,
	//	*hangupButton_,
	//	*quitButton_,
	//	*previewButton_;
	//QLineEdit *url_;
	//VidWin *video_;
	//VidWin *video_prev_;
	////QStatusBar *statusBar_;
	//QLabel *statusBar_;
	//QLabel *localUri_;

	//QVBoxLayout *vbox_left;


	//VidWin *_pVideoPrevWidget;
	//VidWin *_pVideoInputWidget;
	QString _uri;

	//QWidget* _pParentWidgetForPreview;
	//QWidget* _pParentWidgetForIncomingVideo;

	//void initLayout();
private:
	QImage _img, _currimg;
	SipPhoneWidget* _pPhoneWidget;
	bool _initialized;

	//pjmedia_vid_dev_myframe myframe;
};

#endif // RSIPPHONE_H
