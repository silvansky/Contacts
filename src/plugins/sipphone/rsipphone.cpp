#include "rsipphone.h"

//#include "vidwin.h"

//#include "pjmedia\frame.h"

#ifdef Q_WS_WIN32
# include <windows.h>
#endif

#if defined(PJ_WIN32)
#   define SDL_MAIN_HANDLED
#endif


#include <SDL.h>
#include <assert.h>

#include <utils/customborderstorage.h>
#include <definitions/resources.h>
#include <definitions/customborder.h>
#include <utils/widgetmanager.h>

#include "sipphonewidget.h"

//#include "complexvideowidget.h"



#define LOG_FILE		"rsipphone.log"
#define THIS_FILE		"rsipphone.cpp"


// These configure SIP registration
//#define SIP_DOMAIN		NULL
//#define SIP_DOMAIN		"sipnet.ru"
#define SIP_DOMAIN		"vsip.rambler.ru"
//#define SIP_DOMAIN		"81.19.80.213"
//#define SIP_DOMAIN		"pjsip.org"

//#define SIP_USERNAME		"spendtime"
//#define SIP_PASSWORD		"gjgjdbx"
//#define SIP_USERNAME		"rvoip-1"
//#define SIP_PASSWORD		"a111111"
//#define SIP_PORT		5067

#define DEFAULT_CAP_DEV		PJMEDIA_VID_DEFAULT_CAPTURE_DEV
//#define DEFAULT_CAP_DEV		1
#define DEFAULT_REND_DEV	PJMEDIA_VID_DEFAULT_RENDER_DEV

static pj_bool_t default_mod_on_rx_request(pjsip_rx_data *rdata);

/* The module instance. */
static pjsip_module mod_default_handler =
{
	NULL, NULL,				/* prev, next.		*/
	{ (char*)"mod-default-handler", 19 },	/* Name.		*/
	-1,					/* Id			*/
	PJSIP_MOD_PRIORITY_APPLICATION + 99,	/* Priority	        */
	NULL,				/* load()		*/
	NULL,				/* start()		*/
	NULL,				/* stop()		*/
	NULL,				/* unload()		*/
	&default_mod_on_rx_request,		/* on_rx_request()	*/
	NULL,				/* on_rx_response()	*/
	NULL,				/* on_tx_request.	*/
	NULL,				/* on_tx_response()	*/
	NULL,				/* on_tsx_state()	*/
};

//
// pjsua callbacks
//
static void on_reg_state(pjsua_acc_id acc_id)
{
	RSipPhone::instance()->on_reg_state(acc_id);
}
static void on_reg_state2(pjsua_acc_id acc_id, pjsua_reg_info *info)
{
	RSipPhone::instance()->on_reg_state2(acc_id, info);
}

static void on_call_state(pjsua_call_id call_id, pjsip_event *e)
{
	RSipPhone::instance()->on_call_state(call_id, e);
}

static void on_incoming_call(pjsua_acc_id acc_id, pjsua_call_id call_id, pjsip_rx_data *rdata)
{
	RSipPhone::instance()->on_incoming_call(acc_id, call_id, rdata);
}

static void on_call_media_state(pjsua_call_id call_id)
{
	RSipPhone::instance()->on_call_media_state(call_id);
}

static void on_call_tsx_state(pjsua_call_id call_id, pjsip_transaction *tsx, pjsip_event *e)
{
	RSipPhone::instance()->on_call_tsx_state(call_id, tsx, e);
//PJSIP_TSX_STATE_TRYING
}

//pj_status_t (*put_frame_callback)(pjmedia_frame *frame, int, int);
// ПОПОВ. callback ф-ия передающая кадр
pj_status_t my_put_frame_callback(pjmedia_frame *frame, int w, int h, int stride)
{
	return RSipPhone::instance()->on_my_put_frame_callback(frame, w, h, stride);
}
pj_status_t my_preview_frame_callback(pjmedia_frame *frame, const char* colormodelName, int w, int h, int stride)
{
	return RSipPhone::instance()->on_my_preview_frame_callback(frame, colormodelName, w, h, stride);
}


#define RGB24_LEN(w,h)      ((w) * (h) * 3)
#define RGB32_LEN(w,h)      ((w) * (h) * 4)
#define YUV420P_LEN(w,h)    (((w) * (h) * 3) / 2)
#define YUV422P_LEN(w,h)    ((w) * (h) * 2)

// YUV --> RGB Conversion macros
#define _S(a)		(a)>255 ? 255 : (a)<0 ? 0 : (a)
#define _R(y,u,v) (0x2568*(y)  			       + 0x3343*(u)) /0x2000
#define _G(y,u,v) (0x2568*(y) - 0x0c92*(v) - 0x1a1e*(u)) /0x2000
#define _B(y,u,v) (0x2568*(y) + 0x40cf*(v))					     /0x2000

//#define _R(y,u,v) (1.164*(y)  			       + 1.596*(u))
//#define _G(y,u,v) (1.164*(y) - 0.813*(v) - 0.391*(u))
//#define _B(y,u,v) (1.164*(y) + 2.018*(v))



//void YUV420PtoRGB32(int width, int height, int stride, const unsigned char *src, unsigned char *dst, int dstSize)
//{
//	int h, w;
//	const unsigned char *py, *pu, *pv;
//	py = src;
//	pu = py + (stride * height);
//	pv = pu + (stride * height) / 4;
//
//	int yStrideDelta = stride - width;
//	int uvStride = stride >> 1;
//
//	if (dstSize < (width * height * 3))
//	{
//		//cout << "YUVtoRGB buffer (" << dstSize << ") too small for " << width << "x" << height << " pixels" << endl;
//		return;
//	}
//
//	for (h=0; h<height; h++) 
//	{
//		for (w=0; w<width; w++)
//		{
//			signed int _r,_g,_b; 
//			signed int r, g, b;
//			signed int y, u, v;
//
//			y = *py++ - 16;
//			u = pu[w>>1] - 128;
//			v = pv[w>>1] - 128;
//
//			_r = _R(y,u,v);
//			_g = _G(y,u,v);
//			_b = _B(y,u,v);
//
//			r = _S(_r);
//			g = _S(_g);
//			b = _S(_b);
//
//			*dst++ = r;
//			*dst++ = g;
//			*dst++ = b;
//			//*dst++ = 0;
//		}
//
//		py += yStrideDelta;
//		if (h%2) {
//			pu += uvStride;
//			pv += uvStride;
//		}
//	}
//}

void YUV420PtoRGB32(int width, int height, int stride, const unsigned char *src, unsigned char *dst, int dstSize)
{
	int h, w;
	const unsigned char *py, *pu, *pv;
	py = src;
	pv = py + (stride * height);
	pu = pv + (stride * height) / 4;

	int yStrideDelta = stride - width;
	int uvStride = stride >> 1;

	if (dstSize < (width * height * 3))
	{
		//cout << "YUVtoRGB buffer (" << dstSize << ") too small for " << width << "x" << height << " pixels" << endl;
		return;
	}

	for (h=0; h<height; h++) 
	{
		for (w=0; w<width; w++)
		{
			signed int _r,_g,_b; 
			signed int r, g, b;
			signed int y, u, v;

			y = *py++ - 16;
			v = pv[w>>1] - 128;
			u = pu[w>>1] - 128;

			_r = _R(y,u,v);
			_g = _G(y,u,v);
			_b = _B(y,u,v);

			r = _S(_r);
			g = _S(_g);
			b = _S(_b);

			*dst++ = r;
			*dst++ = g;
			*dst++ = b;
			//*dst++ = 0;
		}

		py += yStrideDelta;
		if (h%2) {
			pu += uvStride;
			pv += uvStride;
		}
	}
}


void YUV422PtoRGB32(int width, int height, const unsigned char *src, unsigned char *dst, int dstSize)
{
	int h, w;
	const unsigned char *py, *pu, *pv;
	py = src;
	pu = py + (width * height);
	pv = pu + (width * height) / 4;

	if (dstSize < (width*height*4))//if (dstSize < (width*height*4))
	{
		//cout << "YUVtoRGB buffer (" << dstSize << ") too small for " << width << "x" << height << " pixels" << endl;
		return;
	}

	for (h=0; h<height; h++) 
	{
		for (w=0; w<width; w++)
		{
			signed int _r,_g,_b; 
			signed int r, g, b;
			signed int y, u, v;

			y = *py++ - 16;
			u = pu[w>>1] - 128;
			v = pv[w>>1] - 128;


			_r = _R(y,u,v);
			_g = _G(y,u,v);
			_b = _B(y,u,v);

			r = _S(_r);
			g = _S(_g);
			b = _S(_b);

			*dst++ = r;
			*dst++ = g;
			*dst++ = b;
			*dst++ = 0;
		}

		//pu += (width>>1);
		//pv += (width>>1);
	}
}

void YUYV422PtoRGB32(int width, int height, const unsigned char *src, unsigned char *dst, int dstSize)
{
	int macropixel = 0;
	int len = height * width / 2;
	const unsigned char *py, *pu, *pv;
	py = src;
	pu = py + 1;//py + (width * height);
	pv = py + 3;//pu + (width * height) / 4;

	if (dstSize < (width*height*3))//if (dstSize < (width*height*4))
	{
		//cout << "YUVtoRGB buffer (" << dstSize << ") too small for " << width << "x" << height << " pixels" << endl;
		return;
	}

	for (macropixel = 0; macropixel < len; macropixel++)
	{
		signed int _r,_g,_b; 
		signed int r, g, b;
		signed int y0, y1, u, v;

		y0 = *py - 16;
		y1 = *(py+2) - 16;
		u = *pu - 128;
		v = *pv - 128;

		_b = _R(y0,u,v);
		_g = _G(y0,u,v);
		_r = _B(y0,u,v);

		r = _S(_r);
		g = _S(_g);
		b = _S(_b);

		*dst++ = r;
		*dst++ = g;
		*dst++ = b;
		//*dst++ = 0;

		_b = _R(y1,u,v);
		_g = _G(y1,u,v);
		_r = _B(y1,u,v);

		r = _S(_r);
		g = _S(_g);
		b = _S(_b);

		*dst++ = r;
		*dst++ = g;
		*dst++ = b;

		py = py + 4; // new
		pu = py + 1;
		pv = py + 3;
	}
}



RSipPhone *RSipPhone::_pInstance;

RSipPhone::RSipPhone(QObject *parent) : QObject(parent), _uri(""),
											//_pVideoInputWidget(NULL), _pVideoPrevWidget(NULL),
											_is_preview_on(false), _pPhoneWidget(NULL), _initialized(false),
											_currentCall(-1), _accountId(-1), _currentRegisterStatus(false)
{
	_pInstance = this;

	connect(this, SIGNAL(signalNewCall(int, bool)), this, SLOT(onNewCall(int, bool)));
	connect(this, SIGNAL(signalCallReleased()), this, SLOT(onCallReleased()));
	connect(this, SIGNAL(signalInitVideoWindow()), this, SLOT(initVideoWindow()));

	connect(this, SIGNAL(signalShowSipPhoneWidget(void*)), this, SLOT(onShowSipPhoneWidget(void*)));
	//connect(this, SIGNAL(signalShowStatus(const QString&)), this, SLOT(doShowStatus(const QString&)));

	emit signalCallReleased();
}

RSipPhone::~RSipPhone()
{
	//delete video_prev_;
	//video_prev_ = NULL;
	//delete video_;
	//video_ = NULL;
	//if(_pVideoPrevWidget)
	//{
	//	delete _pVideoPrevWidget;
	//	_pVideoPrevWidget = NULL;
	//}
	//
	//if(_pVideoInputWidget)
	//{
	//	delete _pVideoInputWidget;
	//	_pVideoInputWidget = NULL;
	//}

	//pj_status_t status;
	//unsigned int maxCount = 32;
	//pjsua_transport_id id[32];
	//pj_status_t num = pjsua_enum_transports(id, &maxCount);
	//for(int i=0; i<maxCount; i++)
	//{
	//	status = pjsua_transport_close(id[i], false);
	//}

	//pjsua_destroy();
	//SDL_QuitSubSystem(SDL_INIT_VIDEO);
	//SDL_Quit();
	//qApp->quit();

	_pInstance = NULL;
}

void RSipPhone::cleanup()
{
	pjsua_destroy();
	//pjsua_destroy2(PJSUA_DESTROY_NO_RX_MSG);
	SDL_QuitSubSystem(SDL_INIT_VIDEO);
	//SDL_Quit();
}

void RSipPhone::showStatus(const char *message)
{
	//PJ_LOG(3,(THIS_FILE, "%s", msg));

	QString msg = QString::fromUtf8(message);
	emit signalShowStatus(msg);
}


void RSipPhone::showError(const char *title, pj_status_t status)
{
	char errmsg[PJ_ERR_MSG_SIZE];
	char errline[120];

	pj_strerror(status, errmsg, sizeof(errmsg));
	snprintf(errline, sizeof(errline), "%s error: %s", title, errmsg);
	showStatus(errline);
}

void RSipPhone::onNewCall(int cid, bool incoming)
{
	myframe.put_frame_callback = &my_put_frame_callback;
	myframe.preview_frame_callback = &my_preview_frame_callback;

	pjsua_call_info ci;

	pj_assert(_currentCall == -1);
	_currentCall = cid;

	pjsua_call_get_info(cid, &ci);

	if(ci.role == PJSIP_ROLE_UAS)
		_currentRole = INCOMMING;
	else if(ci.role == PJSIP_ROLE_UAC)
		_currentRole = OUTGOING;
	



	//url_->setText(ci.remote_info.ptr);
	//url_->setEnabled(false);
	
	//hangupButton_->setEnabled(true);

	//if (incoming)
	//{
	//	callButton_->setText(tr("Answer"));
	//	callButton_->setEnabled(true);
	//}
	//else
	//{
	//	callButton_->setEnabled(false);
	//}

	//////video_->setText(ci.remote_contact.ptr);
	//////video_->setWindowTitle(ci.remote_contact.ptr);
}

void RSipPhone::onCallReleased()
{
	//url_->setEnabled(true);
	//callButton_->setEnabled(true);
	//callButton_->setText(tr("Call"));
	//hangupButton_->setEnabled(false);
	
	if(_currentCall == -1)
		return;

	_currentCall = -1;

	//if(_pVideoInputWidget)
	//{
	//	delete _pVideoInputWidget;
	//	_pVideoInputWidget = NULL;
	//}

	myframe.put_frame_callback = NULL;
	myframe.preview_frame_callback = NULL;

	////////////////////////////////////////if(_pPhoneWidget)
	////////////////////////////////////////{
	////////////////////////////////////////	delete _pPhoneWidget;
	////////////////////////////////////////	_pPhoneWidget = NULL;
	////////////////////////////////////////}

	if(_pPhoneWidget)
	{
		if (_pPhoneWidget->parentWidget())
			_pPhoneWidget->parentWidget()->deleteLater();
		_pPhoneWidget->deleteLater();
		_pPhoneWidget = NULL;
	}
}


void RSipPhone::preview()
{
	if (_is_preview_on)
	{
		//delete video_prev_;
		//video_prev_ = NULL;
		//if(_pVideoPrevWidget)
		//{
		//	delete _pVideoPrevWidget;
		//	_pVideoPrevWidget = NULL;
		//}
		pjsua_vid_preview_stop(DEFAULT_CAP_DEV);

		showStatus("Preview stopped");
		//previewButton_->setText(tr("Start &Preview"));
	}
	else
	{
		pjsua_vid_win_id wid;
		pjsua_vid_win_info wi;
		pjsua_vid_preview_param pre_param;
		pj_status_t status;

		pjsua_vid_preview_param_default(&pre_param);
		pre_param.rend_id = DEFAULT_REND_DEV;
		pre_param.show = PJ_FALSE;
		//pre_param.wnd_flags = !PJMEDIA_VID_DEV_WND_BORDER;

		status = pjsua_vid_preview_start(DEFAULT_CAP_DEV, &pre_param);
		if (status != PJ_SUCCESS)
		{
			char errmsg[PJ_ERR_MSG_SIZE];
			pj_strerror(status, errmsg, sizeof(errmsg));
			//QMessageBox::critical(0, "Error creating preview", errmsg);
			return;
		}
		wid = pjsua_vid_preview_get_win(DEFAULT_CAP_DEV);
		pjsua_vid_win_get_info(wid, &wi);

		//_pVideoPrevWidget = new VidWin(&wi.hwnd);
		//vbox_left->addWidget(video_prev_, 1);
		//emit signalVideoPrevWidgetSet(_pVideoPrevWidget);
		emit signalVideoPrevWidgetSet((void*)wi.hwnd.info.win.hwnd);



		//////Using this will cause SDL window to display blank
		//////screen sometimes, probably because it's using different
		//////X11 Display
		//////status = pjsua_vid_win_set_show(wid, PJ_TRUE);
		//////This is handled by VidWin now
		//////video_prev_->show_sdl();
		showStatus("Preview started");

		//previewButton_->setText(tr("Stop &Preview"));
	}
	_is_preview_on = !_is_preview_on;

	emit previewStatusChanged(_is_preview_on);
}

void RSipPhone::call()
{
	pjsua_call_setting call_setting;

	if (_currentRole == INCOMMING)// (callButton_->text() == "Answer")
	{
		pj_assert(_currentCall != -1);
		pjsua_call_answer(_currentCall, 200, NULL, NULL);

		
		//pjsua_call_setting_default(&call_setting);
		//call_setting.vid_cnt = 0;
		//pjsua_call_answer2(_currentCall, &call_setting, 200, NULL, NULL);

		//callButton_->setEnabled(false);
	}
	else if(_currentRole == OUTGOING)
	{
		pj_status_t status;
		QString dst = _uri;// url_->text();
		char uriTmp[256];

		pj_ansi_strncpy(uriTmp, dst.toAscii().data(), sizeof(uriTmp));
		pj_str_t uri = pj_str((char*)uriTmp);

		pj_assert(_currentCall == -1);

		status = pjsua_call_make_call(_accountId, &uri, 0, NULL, NULL, &_currentCall);
		if (status != PJ_SUCCESS)
		{
			showError("make call", status);
			return;
		}


		//pjsua_call_setting_default(&call_setting);
		//call_setting.vid_cnt = 0;//(vidEnabled_->checkState()==Qt::Checked);

		//status = pjsua_call_make_call(_accountId, &uri, &call_setting, NULL, NULL, &_currentCall);
		//if (status != PJ_SUCCESS)
		//{
		//	showError("make call", status);
		//	return;
		//}

	}
}

void RSipPhone::call(const QString& uriToCall)
{
	call(uriToCall.toUtf8().data());
}

void RSipPhone::call(const char* uriToCall)
{
	pj_status_t status;
	//QString dst = _uri;// url_->text();
	char uriTmp[512];

	//pj_ansi_strncpy(uriTmp, uriToCall, sizeof(uriTmp));
	//pj_ansi_sprintf(uriTmp, "sip:%s@vsip.rambler.ru", uriToCall);
	pj_ansi_sprintf(uriTmp, "sip:%s", uriToCall);
	pj_str_t uri = pj_str((char*)uriTmp);

	pj_assert(_currentCall == -1);


	//pjsua_call_setting call_setting;
	//pjsua_call_setting_default(&call_setting);
	//call_setting.vid_cnt = 0;//(vidEnabled_->checkState()==Qt::Checked);

	// NULL SOUND
	//int capture_dev = -99;
	//int playback_dev = -99;
	//pjsua_get_snd_dev(&capture_dev, &playback_dev);
	////status = pjsua_set_null_snd_dev();
	//pjsua_set_snd_dev(-99, playback_dev);


	status = pjsua_call_make_call(_accountId, &uri, 0, NULL, NULL, &_currentCall);
	//status = PJMEDIA_EAUD_NODEFDEV;
	if (status != PJ_SUCCESS)
	{
		if(status == PJMEDIA_EAUD_NODEFDEV)
		{
			//emit signal_DeviceError();
			emit signal_InviteStatus(false, 2, tr("Failed to find default audio device"));
		}
		showError("make call", status);
		return;
	}
	emit signal_InviteStatus(true, 0, "");
}

void RSipPhone::hangup()
{
	if(_currentCall == -1)
		return;
	//pj_assert(_currentCall != -1);
	//pjsua_call_hangup(_currentCall, PJSIP_SC_BUSY_HERE, NULL, NULL);
	pjsua_call_hangup_all();

	emit signalCallReleased();
}





pj_status_t RSipPhone::on_my_preview_frame_callback(pjmedia_frame *frame, const char* colormodelName, int w, int h, int stride)
{
	if(frame->type != PJMEDIA_FRAME_TYPE_VIDEO)
		return 0;


	int dstSize = w * h * 3;
	unsigned char * dst = new unsigned char[dstSize];

	if(strstr(colormodelName, "RGB") != 0)
	{
		memcpy(dst, frame->buf, dstSize);
		//_currimg = QImage((uchar*)dst, w, h, QImage::Format_RGB888);
		_currimg = QImage((uchar*)dst, w, h, QImage::Format_RGB888).rgbSwapped();

		delete[] dst;
	}
	else if(strstr(colormodelName, "YUY") != 0)
	{
		YUYV422PtoRGB32(w, h, (unsigned char *)frame->buf, dst, dstSize);
		_currimg = QImage((uchar*)dst, w, h, QImage::Format_RGB888).copy();

		delete[] dst;
	}
	else
	{
		YUV420PtoRGB32(w, h, stride, (unsigned char *)frame->buf, dst, dstSize);
		_currimg = QImage((uchar*)dst, w, h, QImage::Format_RGB888).copy();

		delete[] dst;
	}

	//////////YUYV422PtoRGB32(w, h, (unsigned char *)frame->buf, dst, dstSize);
	////////////YUV420PtoRGB32(w, h, stride, (unsigned char *)frame->buf, dst, dstSize);
	//////////_currimg = QImage((uchar*)dst, w, h, QImage::Format_RGB888);
	////////////_currimg = QImage((uchar*)dst, w, h, QImage::Format_RGB666);
	////////////_currimg = QImage((uchar*)dst, w, h, QImage::Format_ARGB32);

	if(_pPhoneWidget)
	{
		//_pPhoneWidget->SetCurrImage(_currimg);
		emit signal_SetCurrentImage(_currimg);
	}

	return 0;
}

pj_status_t RSipPhone::on_my_put_frame_callback(pjmedia_frame *frame, int w, int h, int stride)
{
	if(frame->type != PJMEDIA_FRAME_TYPE_VIDEO)
		return 0;


	int dstSize = w * h * 3;
	unsigned char * dst = new unsigned char[dstSize];

	//YUV422PtoRGB32(w, h, (unsigned char *)frame->buf, dst, dstSize);
	YUV420PtoRGB32(w, h, stride, (unsigned char *)frame->buf, dst, dstSize);
	////_img = QImage((uchar*)frame->buf, w, h, QImage::Format_RGB888);
	_img = QImage((uchar*)dst, w, h, QImage::Format_RGB888);
	////_img = QImage((uchar*)dst, w, h, QImage::Format_ARGB32);
	////_img = QImage((uchar*)dst, w, h, QImage::Format_ARGB32_Premultiplied);

	if(_pPhoneWidget)
	{
		////_pPhoneWidget->SetCurrImage(_img);
		//_pPhoneWidget->SetRemoteImage(_img);
		emit signal_SetRomoteImage(_img);
	}

	return 0;
}


void RSipPhone::initVideoWindow()
{
	//myframe.put_frame_callback = &my_put_frame_callback;
	//myframe.preview_frame_callback = &my_preview_frame_callback;

	pjsua_call_info ci;
	unsigned i;

	if (_currentCall == -1)
		return;

	pjsua_call_get_info(_currentCall, &ci);
	for (i = 0; i < ci.media_cnt; ++i)
	{
		if ((ci.media[i].type == PJMEDIA_TYPE_VIDEO) && (ci.media[i].dir & PJMEDIA_DIR_DECODING))
		{
			pjsua_vid_win_info wi;
			pjsua_vid_win_get_info(ci.media[i].stream.vid.win_in, &wi);

			//video_= new VidWin(&wi.hwnd);
			//vbox_left->addWidget(video_, 1);

			//_pVideoInputWidget = new VidWin(&wi.hwnd);
			//emit signalVideoInputWidgetSet(_pVideoInputWidget);
			//emit signalVideoInputWidgetSet((HWND*)wi.hwnd.info.win.hwnd);
			emit signalShowSipPhoneWidget(wi.hwnd.info.win.hwnd);
#ifdef Q_WS_WIN32
			ShowWindow((HWND)wi.hwnd.info.win.hwnd, SW_HIDE);
#endif

			//preview();

			break;
		}
	}
	//emit signalShowSipPhoneWidget(0);
}

void RSipPhone::on_reg_state2(pjsua_acc_id acc_id, pjsua_reg_info *info)
{
	int i;
	i = info->cbparam->code;
	i++;

}

void RSipPhone::on_reg_state(pjsua_acc_id acc_id)
{
	pjsua_acc_info info;

	pjsua_acc_get_info(acc_id, &info);

	char reg_status[80];
	char status[120];

	if(info.status != PJSIP_SC_OK)
	{
		pj_ansi_snprintf(reg_status, sizeof(reg_status), "%.*s",
			(int)info.status_text.slen,
			info.status_text.ptr);
	}
	else
	{
		pj_ansi_snprintf(reg_status, sizeof(reg_status),
			"%d/%.*s (expires=%d)",
			info.status,
			(int)info.status_text.slen,
			info.status_text.ptr,
			info.expires);
	}
	//if (!info.has_registration)
	//{
	//	pj_ansi_snprintf(reg_status, sizeof(reg_status), "%.*s",
	//		(int)info.status_text.slen,
	//		info.status_text.ptr);
	//}
	//else
	//{
	//	pj_ansi_snprintf(reg_status, sizeof(reg_status),
	//		"%d/%.*s (expires=%d)",
	//		info.status,
	//		(int)info.status_text.slen,
	//		info.status_text.ptr,
	//		info.expires);
	//}

	// Текущий статус регистрации сравниваем с вновь полученым
	// и выдаем сигнал, если статус изменился.
	bool regStatus = false;
	//if(info.has_registration > 0)
	if(info.status == PJSIP_SC_OK)
	{
		regStatus = true;
	}
	if(_currentRegisterStatus != regStatus)
	{
		
		_currentRegisterStatus = regStatus;
	}
	emit signalRegistrationStatusChanged(regStatus);

	snprintf(status, sizeof(status), "%.*s: %s\n", (int)info.acc_uri.slen, info.acc_uri.ptr, reg_status);
	showStatus(status);
}

void RSipPhone::on_call_state(pjsua_call_id call_id, pjsip_event *e)
{
	pjsua_call_info ci;

	PJ_UNUSED_ARG(e);

	pjsua_call_get_info(call_id, &ci);

	if ( _currentCall == -1 && ci.state < PJSIP_INV_STATE_DISCONNECTED && ci.state != PJSIP_INV_STATE_INCOMING)
	//if (_currentCall == -1 && ci.state < PJSIP_INV_STATE_DISCONNECTED)
	{
		emit signalNewCall(call_id, false);
	}

	//if (ci.state == PJSIP_INV_STATE_INCOMING)
	//{
	//	pjsua_call_answer(call_id, 200, NULL, NULL);
	//}

	if(ci.state == PJSIP_INV_STATE_CONFIRMED)
	{
		//emit signalShowSipPhoneWidget(NULL);
	}

	char status[80];
	if (ci.state == PJSIP_INV_STATE_DISCONNECTED)
	{
		snprintf(status, sizeof(status), "Call is %s (%s)", ci.state_text.ptr, ci.last_status_text.ptr);
		showStatus(status);
		emit signalCallReleased();
	}
	else
	{
		snprintf(status, sizeof(status), "Call is %s", pjsip_inv_state_name(ci.state));
		showStatus(status);
	}
}

void RSipPhone::on_incoming_call(pjsua_acc_id acc_id, pjsua_call_id call_id, pjsip_rx_data *rdata)
{
	PJ_UNUSED_ARG(acc_id);
	PJ_UNUSED_ARG(rdata);

	if (_currentCall != -1)
	{
		pjsua_call_answer(call_id, PJSIP_SC_BUSY_HERE, NULL, NULL);
		return;
	}

	emit signalNewCall(call_id, true);

	pjsua_call_info ci;
	char status[80];

	pjsua_call_get_info(call_id, &ci);
	snprintf(status, sizeof(status), "Incoming call from %.*s", (int)ci.remote_info.slen, ci.remote_info.ptr);

	// ВНИМАНИЕ! Автоматически акцептим входящий вызов !!!
	pjsua_call_answer(call_id, 200, NULL, NULL);

	//pjsua_call_setting call_setting;
	//pjsua_call_setting_default(&call_setting);
	//call_setting.vid_cnt = 0;
	//pjsua_call_answer2(call_id, &call_setting, 200, NULL, NULL);

	showStatus(status);
}

void RSipPhone::on_call_media_state(pjsua_call_id call_id)
{
	pjsua_call_info ci;
	pjsua_call_get_info(call_id, &ci);

	for (unsigned i=0; i<ci.media_cnt; ++i)
	{
		if (ci.media[i].type == PJMEDIA_TYPE_AUDIO)
		{
			switch (ci.media[i].status)
			{
			case PJSUA_CALL_MEDIA_ACTIVE:
				pjsua_conf_connect(ci.media[i].stream.aud.conf_slot, 0);
				pjsua_conf_connect(0, ci.media[i].stream.aud.conf_slot);
				break;
			default:
				break;
			}
		}
		else if (ci.media[i].type == PJMEDIA_TYPE_VIDEO)
		{
			emit signalInitVideoWindow();
		}
	}
}

void RSipPhone::on_call_tsx_state(pjsua_call_id call_id, pjsip_transaction *tsx, pjsip_event *e)
{
	if(tsx->state == PJSIP_TSX_STATE_TRYING)
	{
		//pjsua_call_answer(call_id, 200, NULL, NULL);
	}

	if(tsx->state == PJSIP_TSX_STATE_COMPLETED)
	{
		//emit signalShowSipPhoneWidget();
	}
}


bool RSipPhone::isCameraReady() const
{
	if(!_initialized)
		return false;

	int devCount = pjmedia_vid_dev_count();
	for (int id=0; id<devCount; id++)
	{
		pjmedia_vid_dev_info info;
		pjmedia_vid_dev_get_info(id, &info);
		if(info.dir == PJMEDIA_DIR_CAPTURE && !strstr(info.name, "Colorbar"))
			return true;
	}
	return false;
}

void RSipPhone::updateCallerName()
{
	QWidget *topWidget = _pPhoneWidget;
	while(topWidget && topWidget->parentWidget()!=NULL)
		topWidget = topWidget->parentWidget();

	if (topWidget)
	{
		if (_currentRole == INCOMMING)
			topWidget->setWindowTitle(tr("Call from %1").arg(_callerName));
		else
			topWidget->setWindowTitle(tr("Call to %1").arg(_callerName));
	}
}

bool RSipPhone::sendVideo(bool isSending)
{
	pjsua_call_setting call_setting;

	if(!_initialized || _currentCall == -1)
		return false;

  pjsua_call_setting_default(&call_setting);
	call_setting.vid_cnt = isSending ? 1 : 0;

  pjsua_call_reinvite2(_currentCall, &call_setting, NULL);

	return true;
}



void RSipPhone::onShowSipPhoneWidget(void* hwnd)
{
	//if(_pPhoneWidget)
	//{
	//	delete _pPhoneWidget;
	//	_pPhoneWidget = NULL;
	//}

	if(_pPhoneWidget)
	{
		if (_pPhoneWidget->parentWidget())
			_pPhoneWidget->parentWidget()->deleteLater();
		_pPhoneWidget->deleteLater();
		_pPhoneWidget = NULL;
	}




	_pPhoneWidget = new SipPhoneWidget( this );

	connect(_pPhoneWidget, SIGNAL(hangupCall()), this, SLOT(hangup()));
	connect(this, SIGNAL(signal_SetRomoteImage(const QImage&)), _pPhoneWidget, SLOT(SetRemoteImage(const QImage&)), Qt::QueuedConnection);
	connect(this, SIGNAL(signal_SetCurrentImage(const QImage&)), _pPhoneWidget, SLOT(SetCurrImage(const QImage&)), Qt::QueuedConnection);

	connect(_pPhoneWidget, SIGNAL(signal_CameraStateChange(bool)), this, SLOT(sendVideo(bool)));


	bool camera = isCameraReady();
	//_pPhoneWidget->show();
	//return;


	////widget->setWindowTitle(tr("Videocall with: ") + subject);
	////connect(widget, SIGNAL(callDeleted(bool)), this, SIGNAL(callDeletedProxy(bool)));
	////connect(widget, SIGNAL(fullScreenState(bool)), this, SLOT(onFullScreenState(bool)));
	////connect(widget, SIGNAL(callWasHangup()), this, SLOT(onHangupCall()));

	////if(!_pWorkWidget.isNull())
	////	delete _pWorkWidget;
	////_pWorkWidget = widget;

	////widget->setRemote( num );
	////if( !num.isEmpty() )
	////{
	////	widget->clickDial();
	////}


	CustomBorderContainer * border = CustomBorderStorage::staticStorage(RSR_STORAGE_CUSTOMBORDER)->addBorder(_pPhoneWidget, CBS_VIDEOCALL);
	if (border)
	{
		border->setMinimizeButtonVisible(false);
		border->setMaximizeButtonVisible(false);
		border->setCloseButtonVisible(false);
		border->setMovable(true);
		border->setResizable(true);
		border->resize(621, 480);
		//border->installEventFilter(this);
		//border->setStaysOnTop(true);
		WidgetManager::alignWindow(border, Qt::AlignCenter);
	}
	else
	{
		_pPhoneWidget->resize(621, 480);
		WidgetManager::alignWindow(_pPhoneWidget, Qt::AlignCenter);
	}

	//if(!_pWorkWidgetContainer.isNull())
	//	delete _pWorkWidgetContainer;
	//_pWorkWidgetContainer = border;


	//connect( widget, SIGNAL( redirectCall( const SipUri &, const QString & ) ), this, SLOT( redirectCall( const SipUri &, const QString & ) ) );

	//widget->show();
	updateCallerName();
	WidgetManager::showActivateRaiseWindow(_pPhoneWidget->window()); //!!!!!!!!!!!!!!!!!!!!
}



//
// initStack()
//
bool RSipPhone::initStack(const QString& sip_domain, int sipPortNum, const QString& sip_username, const QString& sip_password)
{
	return RSipPhone::initStack(sip_domain.toUtf8().data(), sipPortNum, sip_username.toUtf8().data(), sip_password.toUtf8().data());
}
bool RSipPhone::initStack(const char* sip_domain, int sipPortNum, const char* sip_username, const char* sip_password)
{
	if ( SDL_InitSubSystem(SDL_INIT_VIDEO) < 0 )
	{
		printf("Unable to init SDL: %s\n", SDL_GetError());
		return false;
	}


	pj_status_t status;

	//showStatus("Creating stack..");
	status = pjsua_create();
	if (status != PJ_SUCCESS)
	{
		showError("pjsua_create", status);
		return false;
	}

	showStatus("Initializing stack..");

	pjsua_config ua_cfg;
	pjsua_config_default(&ua_cfg);
	pjsua_callback ua_cb;
	pj_bzero(&ua_cb, sizeof(ua_cb));
	ua_cfg.cb.on_reg_state = &::on_reg_state;
	ua_cfg.cb.on_call_state = &::on_call_state;
	ua_cfg.cb.on_incoming_call = &::on_incoming_call;
	ua_cfg.cb.on_call_media_state = &::on_call_media_state;
	ua_cfg.cb.on_call_tsx_state = &::on_call_tsx_state;

	//ua_cfg.stun_srv_cnt = 1;
	//ua_cfg.stun_srv[0] = pj_str((char*)"talkpad.ru:5065");
	//ua_cfg.stun_srv[0] = pj_str((char*)"vsip.rambler.ru:5065");
	

	pjsua_logging_config log_cfg;
	pjsua_logging_config_default(&log_cfg);
	log_cfg.log_filename = pj_str((char*)LOG_FILE);

	pjsua_media_config med_cfg;
	pjsua_media_config_default(&med_cfg);
	//med_cfg.enable_ice = true;

	

	status = pjsua_init(&ua_cfg, &log_cfg, &med_cfg);
	//status = pjsua_init(&ua_cfg, NULL, &med_cfg);
	if (status != PJ_SUCCESS)
	{
		showError("pjsua_init", status);
		goto on_error;
	}

	//
	// Create UDP and TCP transports
	//
	pjsua_transport_config udp_cfg;
	pjsua_transport_id udp_id;
	pjsua_transport_config_default(&udp_cfg);
	//udp_cfg.port =  SIP_PORT;
	udp_cfg.port = sipPortNum;

	status = pjsua_transport_create(PJSIP_TRANSPORT_UDP, &udp_cfg, &udp_id);
	if (status != PJ_SUCCESS)
	{
		showError("UDP transport creation", status);
		goto on_error;
	}

	pjsua_transport_info udp_info;
	status = pjsua_transport_get_info(udp_id, &udp_info);
	if (status != PJ_SUCCESS)
	{
		showError("UDP transport info", status);
		goto on_error;
	}

	//pjsua_transport_config tcp_cfg;
	//pjsua_transport_config_default(&tcp_cfg);
	//tcp_cfg.port = 0;
	//status = pjsua_transport_create(PJSIP_TRANSPORT_TCP, &tcp_cfg, NULL);
	//if (status != PJ_SUCCESS)
	//{
	//	showError("TCP transport creation", status);
	//	goto on_error;
	//}

	//
	// Create account
	//
	pjsua_acc_config acc_cfg;
	pjsua_acc_config_default(&acc_cfg);


	//if(!sip_domain.isEmpty())
	{
		char idtmp[1024];
		//pj_ansi_strncpy(idtmp, QString("sip:" + sip_username + "@" + sip_domain).toUtf8().data(), sizeof(idtmp));
		pj_ansi_snprintf(idtmp, sizeof(idtmp), "sip:%s@%s",sip_username, sip_domain);

		acc_cfg.id = pj_str((char*)idtmp);

		char reg_uritmp[1024];
		//pj_ansi_strncpy(reg_uritmp, QString("sip:" + sip_domain).toUtf8().data(), sizeof(reg_uritmp));
		pj_ansi_snprintf(reg_uritmp, sizeof(reg_uritmp), "sip:%s", sip_domain);
		acc_cfg.reg_uri = pj_str((char*)reg_uritmp);

		acc_cfg.cred_count = 1;
		acc_cfg.cred_info[0].realm = pj_str((char*)"*");
		acc_cfg.cred_info[0].scheme = pj_str((char*)"digest");

		char usernametmp[512];
		//pj_ansi_strncpy(usernametmp, sip_username.toUtf8().data(), sizeof(usernametmp));
		pj_ansi_strncpy(usernametmp, sip_username, sizeof(usernametmp));
		//pj_ansi_vsnprintf(usernametmp, sizeof(usernametmp), "%s", sip_username);
		acc_cfg.cred_info[0].username = pj_str((char*)usernametmp);

		char passwordtmp[512];
		//pj_ansi_strncpy(passwordtmp, sip_password.toUtf8().data(), sizeof(passwordtmp));
		pj_ansi_strncpy(passwordtmp, sip_password, sizeof(passwordtmp));
		acc_cfg.cred_info[0].data = pj_str((char*)passwordtmp);
	}



//#ifdef SIP_DOMAIN
//	acc_cfg.id = pj_str( "sip:" SIP_USERNAME "@" SIP_DOMAIN);
//	acc_cfg.reg_uri = pj_str((char*) ("sip:" SIP_DOMAIN));
//	acc_cfg.cred_count = 1;
//	acc_cfg.cred_info[0].realm = pj_str((char*)"*");
//	acc_cfg.cred_info[0].scheme = pj_str((char*)"digest");
//	acc_cfg.cred_info[0].username = pj_str((char*)SIP_USERNAME);
//	acc_cfg.cred_info[0].data = pj_str((char*)SIP_PASSWORD);
//#else
//	char sip_id[80];
//	snprintf(sip_id, sizeof(sip_id),
//		"sip:%s@%.*s:%u", SIP_USERNAME,
//		(int)udp_info.local_name.host.slen,
//		udp_info.local_name.host.ptr,
//		udp_info.local_name.port);
//	acc_cfg.id = pj_str(sip_id);
//#endif

	//acc_cfg.max_video_cnt = 1;
	acc_cfg.vid_cap_dev = DEFAULT_CAP_DEV;
	acc_cfg.vid_rend_dev = DEFAULT_REND_DEV;
	acc_cfg.vid_in_auto_show = PJ_TRUE;
	acc_cfg.vid_out_auto_transmit = PJ_TRUE;
	//acc_cfg.vid_out_auto_transmit = PJ_FALSE;
	acc_cfg.register_on_acc_add = PJ_FALSE;

	status = pjsua_acc_add(&acc_cfg, PJ_TRUE, &_accountId);
	if (status != PJ_SUCCESS)
	{
		showError("Account creation", status);
		goto on_error;
	}

	//localUri_->setText(acc_cfg.id.ptr);

	//
	// Start pjsua!
	//
	showStatus("Starting stack..");
	status = pjsua_start();
	if (status != PJ_SUCCESS)
	{
		showError("pjsua_start", status);
		goto on_error;
	}

	/* We want to be registrar too! */
	if (pjsua_get_pjsip_endpt())
	{
		pjsip_endpt_register_module(pjsua_get_pjsip_endpt(), &mod_default_handler);
	}

	showStatus("Ready");

	_initialized = true;
	return true;

on_error:
	pjsua_destroy();
	_initialized = false;
	return false;
}


bool RSipPhone::initStack(const QString& sip_server, int sipPortNum, const QString& sip_username, const QString& sip_password, const QString& sip_domain)
{
	return RSipPhone::initStack(sip_server.toUtf8().data(), sipPortNum, sip_username.toUtf8().data(), sip_password.toUtf8().data(), sip_domain.toUtf8().data());
}

bool RSipPhone::initStack(const char* sip_server, int sipPortNum, const char* sip_username, const char* sip_password, const char* sip_domain)
{
	if ( SDL_InitSubSystem(SDL_INIT_VIDEO) < 0 )
	{
		printf("Unable to init SDL: %s\n", SDL_GetError());
		return false;
	}


	pj_status_t status;

	//showStatus("Creating stack..");
	status = pjsua_create();
	if (status != PJ_SUCCESS)
	{
		showError("pjsua_create", status);
		return false;
	}

	showStatus("Initializing stack..");

	pjsua_config ua_cfg;
	pjsua_config_default(&ua_cfg);
	pjsua_callback ua_cb;
	pj_bzero(&ua_cb, sizeof(ua_cb));
	ua_cfg.cb.on_reg_state = &::on_reg_state;
	ua_cfg.cb.on_reg_state2 = &::on_reg_state2;
	ua_cfg.cb.on_call_state = &::on_call_state;
	ua_cfg.cb.on_incoming_call = &::on_incoming_call;
	ua_cfg.cb.on_call_media_state = &::on_call_media_state;
	ua_cfg.cb.on_call_tsx_state = &::on_call_tsx_state;

	ua_cfg.outbound_proxy_cnt = 1;
	char proxyTmp[512];
	//pj_ansi_strncpy(proxyTmp, sip_server, sizeof(proxyTmp));
	pj_ansi_snprintf(proxyTmp, sizeof(proxyTmp), "sip:%s", sip_server);
	//acc_cfg.proxy[0] = pj_str((char*)reg_uritmp);
	ua_cfg.outbound_proxy[0] = pj_str((char*)proxyTmp);

	//ua_cfg.stun_srv_cnt = 1;
	//ua_cfg.stun_srv[0] = pj_str((char*)"talkpad.ru:5065");
	//ua_cfg.stun_srv[0] = pj_str((char*)"vsip.rambler.ru:5065");




	pjsua_logging_config log_cfg;
	pjsua_logging_config_default(&log_cfg);
	log_cfg.log_filename = pj_str((char*)LOG_FILE);

	pjsua_media_config med_cfg;
	pjsua_media_config_default(&med_cfg);
	//med_cfg.enable_ice = true;


	status = pjsua_init(&ua_cfg, &log_cfg, &med_cfg);
	//status = pjsua_init(&ua_cfg, NULL, &med_cfg);
	if (status != PJ_SUCCESS)
	{
		showError("pjsua_init", status);
		goto on_error;
	}

	//
	// Create UDP and TCP transports
	//
	pjsua_transport_config udp_cfg;
	pjsua_transport_id udp_id;
	pjsua_transport_config_default(&udp_cfg);
	//udp_cfg.port =  SIP_PORT;
	udp_cfg.port = sipPortNum;
	//udp_cfg.public_addr = pj_str((char*)"vsip.rambler.ru");

	status = pjsua_transport_create(PJSIP_TRANSPORT_UDP, &udp_cfg, &udp_id);
	if (status != PJ_SUCCESS)
	{
		showError("UDP transport creation", status);
		goto on_error;
	}

	pjsua_transport_info udp_info;
	status = pjsua_transport_get_info(udp_id, &udp_info);
	if (status != PJ_SUCCESS)
	{
		showError("UDP transport info", status);
		goto on_error;
	}

	////pjsua_transport_config tcp_cfg;
	////pjsua_transport_config_default(&tcp_cfg);
	////tcp_cfg.port = 0;

	////status = pjsua_transport_create(PJSIP_TRANSPORT_TCP, &tcp_cfg, NULL);
	////if (status != PJ_SUCCESS)
	////{
	////	showError("TCP transport creation", status);
	////	goto on_error;
	////}

	//
	// Create account
	//
	pjsua_acc_config acc_cfg;
	pjsua_acc_config_default(&acc_cfg);


	//if(!sip_domain.isEmpty())
	{
		char idtmp[1024];
		//pj_ansi_strncpy(idtmp, QString("sip:" + sip_username + "@" + sip_domain).toUtf8().data(), sizeof(idtmp));
		pj_ansi_snprintf(idtmp, sizeof(idtmp), "sip:%s@%s",sip_username, sip_domain);

		acc_cfg.id = pj_str((char*)idtmp);

		char reg_uritmp[1024];
		//pj_ansi_strncpy(reg_uritmp, QString("sip:" + sip_domain).toUtf8().data(), sizeof(reg_uritmp));
		//pj_ansi_snprintf(reg_uritmp, sizeof(reg_uritmp), "sip:%s", sip_server);
		pj_ansi_snprintf(reg_uritmp, sizeof(reg_uritmp), "sip:%s", sip_domain);
		acc_cfg.reg_uri = pj_str((char*)reg_uritmp);

		//////////acc_cfg.proxy_cnt = 1;
		//////////char proxyTmp[512];
		////////////pj_ansi_strncpy(proxyTmp, sip_server, sizeof(proxyTmp));
		//////////pj_ansi_snprintf(proxyTmp, sizeof(proxyTmp), "sip:%s", sip_server);
		////////////acc_cfg.proxy[0] = pj_str((char*)reg_uritmp);
		//////////acc_cfg.proxy[0] = pj_str((char*)proxyTmp);


		acc_cfg.cred_count = 1;
		acc_cfg.cred_info[0].realm = pj_str((char*)"*");
		acc_cfg.cred_info[0].scheme = pj_str((char*)"digest");

		char usernametmp[512];
		//pj_ansi_strncpy(usernametmp, sip_username.toUtf8().data(), sizeof(usernametmp));
		//pj_ansi_strncpy(usernametmp, sip_username, sizeof(usernametmp));
		pj_ansi_snprintf(usernametmp, sizeof(usernametmp), "%s@%s", sip_username, sip_domain);
		//acc_cfg.cred_info[0].username = pj_str((char*)idtmp);
		acc_cfg.cred_info[0].username = pj_str((char*)usernametmp);

		char passwordtmp[512];
		//pj_ansi_strncpy(passwordtmp, sip_password.toUtf8().data(), sizeof(passwordtmp));
		pj_ansi_strncpy(passwordtmp, sip_password, sizeof(passwordtmp));
		acc_cfg.cred_info[0].data = pj_str((char*)passwordtmp);
	}


	//acc_cfg.max_video_cnt = 1;
	acc_cfg.vid_cap_dev = DEFAULT_CAP_DEV;
	acc_cfg.vid_rend_dev = DEFAULT_REND_DEV;
	acc_cfg.vid_in_auto_show = PJ_TRUE;
	acc_cfg.vid_out_auto_transmit = PJ_TRUE;
	//acc_cfg.vid_out_auto_transmit = PJ_FALSE;
	acc_cfg.register_on_acc_add = PJ_FALSE;

	status = pjsua_acc_add(&acc_cfg, PJ_TRUE, &_accountId);
	if (status != PJ_SUCCESS)
	{
		showError("Account creation", status);
		goto on_error;
	}

	//
	// Start pjsua!
	//
	showStatus("Starting stack..");
	status = pjsua_start();
	if (status != PJ_SUCCESS)
	{
		showError("pjsua_start", status);
		goto on_error;
	}

	/* We want to be registrar too! */
	if (pjsua_get_pjsip_endpt())
	{
		pjsip_endpt_register_module(pjsua_get_pjsip_endpt(), &mod_default_handler);
	}

	showStatus("Ready");

	_initialized = true;
	return true;

on_error:
	pjsua_destroy();
	_initialized = false;
	return false;
}



void RSipPhone::registerAccount(bool rstatus)
{
	//if (rstatus)
	pjsua_acc_set_registration(_accountId, rstatus);
}

QString RSipPhone::callerName() const
{
	return _callerName;
}

void RSipPhone::setCallerName( const QString &AName )
{
	_callerName = AName;
	updateCallerName();
}

/*
* A simple registrar, invoked by default_mod_on_rx_request()
*/
static void simple_registrar(pjsip_rx_data *rdata)
{
	pjsip_tx_data *tdata;
	const pjsip_expires_hdr *exp;
	const pjsip_hdr *h;
	unsigned cnt = 0;
	pjsip_generic_string_hdr *srv;
	pj_status_t status;

	status = pjsip_endpt_create_response(pjsua_get_pjsip_endpt(), rdata, 200, NULL, &tdata);
	if (status != PJ_SUCCESS)
		return;

	exp = (pjsip_expires_hdr*)pjsip_msg_find_hdr(rdata->msg_info.msg, PJSIP_H_EXPIRES, NULL);

	h = rdata->msg_info.msg->hdr.next;
	while (h != &rdata->msg_info.msg->hdr)
	{
		if (h->type == PJSIP_H_CONTACT)
		{
			const pjsip_contact_hdr *c = (const pjsip_contact_hdr*)h;
			int e = c->expires;

			if (e < 0)
			{
				if (exp)
					e = exp->ivalue;
				else
					e = 3600;
			}

			if (e > 0)
			{
				pjsip_contact_hdr *nc = (pjsip_contact_hdr*)pjsip_hdr_clone(tdata->pool, h);
				nc->expires = e;
				pjsip_msg_add_hdr(tdata->msg, (pjsip_hdr*)nc);
				++cnt;
			}
		}
		h = h->next;
	}

	srv = pjsip_generic_string_hdr_create(tdata->pool, NULL, NULL);
	srv->name = pj_str((char*)"Server");
	srv->hvalue = pj_str((char*)"pjsua simple registrar");
	pjsip_msg_add_hdr(tdata->msg, (pjsip_hdr*)srv);

	pjsip_endpt_send_response2(pjsua_get_pjsip_endpt(), rdata, tdata, NULL, NULL);
}

/* Notification on incoming request */
static pj_bool_t default_mod_on_rx_request(pjsip_rx_data *rdata)
{
	/* Simple registrar */
	if (pjsip_method_cmp(&rdata->msg_info.msg->line.req.method, &pjsip_register_method) == 0)
	{
		simple_registrar(rdata);
		return PJ_TRUE;
	}
	return PJ_FALSE;
}

