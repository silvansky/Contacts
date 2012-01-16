#include "sipphonewidget.h"




#include <QMessageBox>
//#include <QSettings>
#include <QDateTime>
#include <QTimer>

//#include "rsipauthentication.h"
//#include "sipphoneproxy.h"

//#include "callaudio.h"

#include "CrossDefine.h"

#include <QResizeEvent>

#include <utils/iconstorage.h>
#include <definitions/resources.h>
#include <definitions/menuicons.h>
#include <utils/stylestorage.h>
#include <definitions/stylesheets.h>

#include "complexvideowidget.h"

#include "qimagelabel.h"
#include "fullscreencontrols.h"

#include "rsipphone.h"



const int fstimerInterval = 3500;
static void updateFSTimer(QTimer*& timer)
{
	if(timer->isActive())
	{
		timer->stop();
		timer->start(fstimerInterval);
	}
	else
	{
		timer->start(fstimerInterval);
	}
}


SipPhoneWidget::SipPhoneWidget(QWidget *parent)
	: QWidget(parent)
{
	ui.setupUi(this);
	setObjectName("SipPhoneWidget");

	setMinimumSize(250, 90);
	curPicAlign = Qt::AlignBottom|Qt::AlignLeft;

	//connect(ui.btnHangup, SIGNAL(clicked()), this, SLOT(hangupCall()));
	StyleStorage::staticStorage(RSR_STORAGE_STYLESHEETS)->insertAutoStyle(this, STS_SIPPHONE);
}

//SipPhoneWidget::SipPhoneWidget(KSipAuthentication *auth, CallAudio *callAudio, SipCall *initCall, SipPhoneProxy *parent, const char *name) : QWidget(NULL), _pSipCall(initCall)
SipPhoneWidget::SipPhoneWidget( RSipPhone *parent, const char *name) : QWidget(NULL)
{
	Q_UNUSED(parent);
	Q_UNUSED(name);
	ui.setupUi(this);

	setObjectName("SipPhoneWidget");
	setMinimumSize(250, 90);
	curPicAlign = Qt::AlignBottom | Qt::AlignLeft;

	ui.remote->hide();
	//connect(ui.btnHangup, SIGNAL(clicked()), this, SLOT(hangupCall()));
	{

	 // ui.verticalLayout_2->removeWidget(ui.wgtRemoteImage);

		//ui.wgtRemoteImage->setHWND((HWND)hwnd);

		//ui.verticalLayout_2->addWidget(ui.wgtRemoteImage);


		_pCurrPic = new QImageLabel(ui.wgtRemoteImage);
		//_pCurrPic->winId();
		connect(_pCurrPic,SIGNAL(moveTo(const QPoint &)),SLOT(moveCurPicLabel(const QPoint &)));

		_pShowCurrPic = new QToolButton(ui.wgtRemoteImage);
		_pShowCurrPic->setObjectName("showCurrPic");
		connect(_pShowCurrPic, SIGNAL(clicked()), _pCurrPic, SLOT(show()));
		connect(_pCurrPic, SIGNAL(visibleState(bool)), _pShowCurrPic, SLOT(setHidden(bool)));

		//_pCurrPic->setFixedSize(160, 120);
		//_pCurrPic->setMouseTracking(true);
		//_pCurrPic->setScaledContents(true);


		_pShowCurrPic->setFixedSize(24, 24);
		_pShowCurrPic->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
		_pShowCurrPic->hide();
		_pShowCurrPic->setMouseTracking(true);

		_pControls = new FullScreenControls(this);//ui.wgtRemoteImage);

		// Issue 2264. В случае когда нет камеры, нодо заблокировать кнопку камеры с соответствующим значком
		//if(callAudio != NULL && callAudio->videoControl() != NULL)
		//{
		//	_pControls->setCameraEnabled(callAudio->videoControl()->checkCameraPresent());
		//}
		//else
		//{
		//	_pControls->setCameraEnabled(false);
		//}

		connect(_pControls, SIGNAL(fullScreenState(bool)), this, SLOT(fullScreenStateChange(bool)));
		connect(_pControls, SIGNAL(fullScreenState(bool)), this, SIGNAL(fullScreenState(bool)));
		connect(_pControls, SIGNAL(camStateChange(bool)), this, SLOT(cameraStateChange(bool)));

		connect(_pControls, SIGNAL(hangup()), this, SIGNAL(hangupCall()));

		_pControls->hide();

		_pControls->setFixedSize(270, 40);
		//_pControls->setMouseTracking(true);
	}

	StyleStorage::staticStorage(RSR_STORAGE_STYLESHEETS)->insertAutoStyle(this, STS_SIPPHONE);

	//_pSipCallMember = NULL;
	//_pSipAuthentication = NULL;
	//_pAudioContoller = NULL;

	_ringCount = 0;
	_pRingTimer = NULL;
	_isRingingTone = false;
	_pAcceptCallTimer = NULL;
	_subject = "";


	//_pCurrentStatus = ui.lblStatus;
	//_pCurrentAudioStatus = ui.lblTime;
	////_pDialButton = ui.btnDial;
	//_pHangupButton = ui.btnHangup;

	//_pSipAuthentication = auth;

	_currentState = PreDial;

	_isHangupInitiator = false;

	//// Задаем реакции на некоторые события звукового контролера
	//_pAudioContoller = callAudio;
	//connect( _pAudioContoller, SIGNAL( outputDead() ), this, SLOT( audioOutputDead() ) );
	//connect( _pAudioContoller, SIGNAL( statusUpdated() ), this, SLOT( updateAudioStatus() ) );
	//connect( _pAudioContoller, SIGNAL( proxyPictureShow(const QImage&)), this, SLOT(remotePictureShow(const QImage&)));
	//connect( _pAudioContoller, SIGNAL( proxyLocalPictureShow(const QImage&)), this, SLOT(localPictureShow(const QImage&)));

	//// Подключение реакций на изменение состояния камеры
	//connect(this, SIGNAL(startCamera()), _pAudioContoller, SIGNAL(proxyStartCamera()));
	//connect(this, SIGNAL(stopCamera()), _pAudioContoller, SIGNAL(proxyStopCamera()));

	// TODO: implement sizeHint() and uncomment this
	////connect(this, SIGNAL(startCamera()), SLOT(adjustMySize()));

	//connect(_pControls, SIGNAL(camResolutionChange(bool)), _pAudioContoller, SLOT(outputVideoResolutonChangedToHigh(bool)));

	//connect(_pAudioContoller, SIGNAL(audioOutputPresentChange(bool)), _pControls, SLOT(setVolumeEnabled(bool)));
	//connect(_pAudioContoller, SIGNAL(audioInputPresentChange(bool)), _pControls, SLOT(setMicEnabled(bool)));


	// ОТЛАДКА
	//connect( this, SIGNAL(callDeleted(bool)), this, SLOT(callDeletedSlot(bool)) );

	_ringCount = 0;
	_pRingTimer = new QTimer();
	//connect( _pRingTimer, SIGNAL( timeout() ), this, SLOT( ringTimeout() ) );

	_pAcceptCallTimer = new QTimer();
	connect( _pAcceptCallTimer, SIGNAL( timeout() ), this, SLOT( acceptCallTimeout() ) );

	//switchCall(initCall);


	// Таймер отслеживающий работу мыши в режиме fullscreen
	_fsTimer = new QTimer(this);
	_fsTimer->setSingleShot(true);
	connect(_fsTimer, SIGNAL(timeout()), this, SLOT(processOneMouseIdle()));
	setMouseTracking(true);
	ui.wgtRemoteImage->setMouseTracking(true);
	ui.wgtRemoteImage->installEventFilter(this);


	// По умолчанию камера выключена (Issue 2249)
	_pControls->SetCameraOn(false);
}


SipPhoneWidget::~SipPhoneWidget()
{
	ui.wgtRemoteImage->removeEventFilter(this);
	delete _pRingTimer;
	delete _pAcceptCallTimer;
	//if( _pSipCall )
	//{
	//	delete _pSipCall;
	//}

	if(_fsTimer)
	{
		if(_fsTimer->timerId() != -1)
			_fsTimer->stop();
		delete _fsTimer;
		_fsTimer = NULL;
	}

	//if(_pAudioContoller)
	//{
	//	delete _pAudioContoller;
	//	_pAudioContoller = NULL;
	//}
}


QSize SipPhoneWidget::sizeHint()
{
	// TODO: small when no video, big when video is active
	return minimumSize();
}

void SipPhoneWidget::SetCurrImage(const QImage& img)
{
	QPixmap tmpPix = QPixmap::fromImage(img);
	//delete[] img.bits();

	_pCurrPic->setPixmap(tmpPix);
}

void SipPhoneWidget::SetRemoteImage(const QImage& img)
{
	QPixmap tmpPix = QPixmap::fromImage(img);

	//free((void*)img.bits());
	delete[] img.bits();


	int h = ui.wgtRemoteImage->size().height() - 2;
	int w = ui.wgtRemoteImage->size().width() - 2;
	Q_UNUSED(h);
	Q_UNUSED(w);

	ui.wgtRemoteImage->setPicture(tmpPix);
}



void SipPhoneWidget::resizeEvent(QResizeEvent *r_event)
{
	QWidget::resizeEvent(r_event);

	QPoint pt = ui.wgtRemoteImage->pos();
	QSize size = ui.wgtRemoteImage->size();

	if(_pCurrPic != NULL)
	{
		QSize currPicSize;
		if(size.width() >= 250 || size.height() >= 150)
		{
			currPicSize.setWidth(160);
			currPicSize.setHeight(120);
		}
		_pCurrPic->setFixedSize(currPicSize);

		QPoint newPos = _pCurrPic->correctTopLeftPos(_pCurrPic->geometry().topLeft());
		if (curPicAlign & Qt::AlignTop)
			newPos.setY(QImageLabel::spacing);
		else if (curPicAlign & Qt::AlignBottom)
			newPos.setY(rect().height()-_pCurrPic->geometry().height()-QImageLabel::spacing);
		if (curPicAlign & Qt::AlignLeft)
			newPos.setX(QImageLabel::spacing);
		else if (curPicAlign & Qt::AlignRight)
			newPos.setX(rect().width()-_pCurrPic->geometry().width()-QImageLabel::spacing);
		_pCurrPic->move(newPos);
	}

	if(_pShowCurrPic != NULL)
	{
		_pShowCurrPic->move(4, size.height() - _pShowCurrPic->size().height() - 4);
	}

	QSize controlsSize = _pControls->size();
	_pControls->move(size.width()/2 - controlsSize.width()/2, size.height() - controlsSize.height() - 4);
}

void SipPhoneWidget::closeEvent(QCloseEvent * ev)
{
	Q_UNUSED(ev);
	emit hangupCall();
}

void SipPhoneWidget::enterEvent(QEvent *e_event)
{
	Q_UNUSED(e_event);
	_pControls->show();
}

void SipPhoneWidget::leaveEvent(QEvent *)
{
	_pControls->hide();
}

void SipPhoneWidget::keyPressEvent(QKeyEvent *ev)
{
	int k = ev->key();
	if(k == Qt::Key_Escape)
	{
		_pControls->setFullScreen(false);
	}
}

void SipPhoneWidget::mouseMoveEvent(QMouseEvent *ev)
{
	Q_UNUSED(ev);
	unsetCursor();
	_pControls->show();
	updateFSTimer(_fsTimer);
}



void SipPhoneWidget::fullScreenStateChange(bool state)
{
	if(state)
	{
		//showFullScreen();
		//setCursor(QCursor( Qt::BlankCursor ));
		_fsTimer->start(fstimerInterval);
	}
	else
	{
		//showNormal();
		unsetCursor();
		_fsTimer->stop();
	}
}

void SipPhoneWidget::processOneMouseIdle()
{
	setCursor(QCursor( Qt::BlankCursor ));
	_pControls->hide();
}

void SipPhoneWidget::adjustMySize()
{
	(parentWidget() ? parentWidget() : (QWidget*)this)->adjustSize();
}

bool SipPhoneWidget::eventFilter( QObject *obj, QEvent *evt )
{
	if(isFullScreen() && obj == ui.wgtRemoteImage)
	{
		QEvent::Type tp = evt->type();
		//if(tp != QEvent::Paint && tp != QEvent::WindowActivate && tp!=QEvent::WindowDeactivate)
		{
			if( tp == QEvent::MouseMove )
			{
				unsetCursor();
				_pControls->show();
				updateFSTimer(_fsTimer);
			}
		}
	}

	return QWidget::eventFilter( obj, evt );
}


void SipPhoneWidget::cameraStateChange(bool state)
{
	if(state)
	{
		emit startCamera();
	}
	else
	{
		emit stopCamera();
		_pCurrPic->setPixmap(QPixmap());
	}
}


//void SipPhoneWidget::callDeletedSlot( bool )
//{
//	QMessageBox::information(NULL, "debug", "SipPhoneWidget::callDeletedSlot()");
//}


void SipPhoneWidget::forceDisconnect( void )
{
	dbgPrintf( "KCallWidget: Starting force disconnect...\n" );
	//if( _pAudioContoller->getCurrentCall() == _pSipCall )
	//{
	//	_pAudioContoller->detachFromCall();
	//}
	//if( _pSipCallMember )
	//	disconnect( _pSipCallMember, 0, this, 0 );

	//_pSipCallMember = 0;

	//if( _pSipCall )
	//{
	//	delete _pSipCall;
	//	_pSipCall = 0;
	//}

	emit callDeleted(_isHangupInitiator);

	_pRingTimer->stop();
	//_pHangupButton->setEnabled(false);

	////_pDialButton->setEnabled(false);
	////holdbutton->setEnabled(false);
	////transferbutton->setEnabled(false);
	////morebutton->setEnabled(false);
}

//void SipPhoneWidget::callMemberStatusUpdated( void )
//{
//	SdpMessage sdpm;
//	SdpMessage rsdp;
//	if( _pSipCallMember->getState() == SipCallMember::state_Disconnected )
//	{
//		if( _pSipCallMember->getLocalStatusDescription().left( 2 ) == "!!" )
//		{
//			//_pCurrentStatus->setText( tr("Call Failed") );
//			statusChanged(tr("Call Failed"));
//			QMessageBox::critical( this, "Contacts", _pSipCallMember->getLocalStatusDescription().remove(0,2) );
//			//setHide();
//		}
//		else
//		{
//			//_pCurrentStatus->setText( _pSipCallMember->getLocalStatusDescription() + " at " + QDateTime::currentDateTime().toString("hh:mm:ss"));
//			statusChanged(_pSipCallMember->getLocalStatusDescription() + " at " + QDateTime::currentDateTime().toString("hh:mm:ss"));
//		}
//		forceDisconnect();
//	}
//	else if( _pSipCallMember->getState() == SipCallMember::state_Redirected )
//	{
//		//_pCurrentStatus->setText( _pSipCallMember->getLocalStatusDescription() );
//		statusChanged(_pSipCallMember->getLocalStatusDescription());
//		handleRedirect();
//	}
//	else if( _pSipCallMember->getState() == SipCallMember::state_Refer )
//	{
//		//_pCurrentStatus->setText( _pSipCallMember->getLocalStatusDescription() );
//		statusChanged(_pSipCallMember->getLocalStatusDescription());
//		_pSipCallMember->setState( SipCallMember::state_Refer_handling );
//		//handleRefer();
//	}
//	else if( _pSipCallMember->getState() == SipCallMember::state_Connected )
//	{
//		//_pCurrentStatus->setText( _pSipCallMember->getLocalStatusDescription() );
//		statusChanged(_pSipCallMember->getLocalStatusDescription());
//		// hidebutton->setEnabled( false );
//		//_pDialButton->setEnabled( false );
//		//_pHangupButton->setEnabled( true );
//		_currentState = Connected;
//		//if (!dtmfsenderTimer->isActive())
//		//  dtmfsenderTimer->start(dtmfsenderdelay->value() * 10);
//	}
//	else
//	{
//		//_pCurrentStatus->setText( _pSipCallMember->getLocalStatusDescription() );
//		statusChanged( _pSipCallMember->getLocalStatusDescription());
//	}
//}

//SipCall *SipPhoneWidget::getCall()
//{
//	return _pSipCall;
//}

//void SipPhoneWidget::switchCall( SipCall *newCall )
//{
//	_pRingTimer->stop();
//	dbgPrintf( "KCallWidget: Switching calls...\n" );
//
//	_pSipCall = newCall;
//
//	if(_pSipCall == NULL)
//		return;
//
//	_pAudioContoller->setBodyMask( _pSipCall->getSdpMessageMask() );
//	////_pDialButton->setText( tr("Dial") );
//	//_pHangupButton->setText( tr("Hangup") );
//
//	if( _pSipCallMember )
//		disconnect( _pSipCallMember, 0, this, 0 );
//
//	SipCallMemberIterator mIt = _pSipCall->getMemberList(); // ПОПОВ
//	mIt.toFront();
//
//	_pSipCallMember = mIt.peekNext();//call->getMemberList().toFirst();
//	if( _pSipCallMember )
//	{
//		if( _pSipCallMember->getState() == SipCallMember::state_Disconnected )
//		{
//			forceDisconnect();
//			return;
//		}
//		if( _pSipCallMember->getState() == SipCallMember::state_Redirected )
//		{
//			handleRedirect();
//			return;
//		}
//		//_pCurrentStatus->setText( _pSipCallMember->getLocalStatusDescription() );
//		emit statusChanged(_pSipCallMember->getLocalStatusDescription());
//		connect( _pSipCallMember, SIGNAL( statusUpdated( SipCallMember * ) ), this, SLOT( callMemberStatusUpdated() ) );
//		//setCaption( QString( tr("Call: ") ) + call->getCallId() );
//		if( _pSipCall->getSubject() == tr("Incoming call") )
//		{
//			if( _pSipCall->getCallType() == SipCall::videoCall )
//			{
//				//setCaption( QString( tr("Incoming Video Call: ") ) + call->getCallId() );
//				//holdbutton->setEnabled( false );
//				//transferbutton->setEnabled( false );
//			}
//			else
//			{
//				//setCaption( QString( tr("Incoming Call: ") ) + call->getCallId() );
//				//holdbutton->setEnabled( true );
//				//transferbutton->setEnabled( true );
//			}
//			QString ss = _pSipCallMember->getUri().uri();
//			QDateTime t = QDateTime::currentDateTime();
//
//			//incomingCall = new IncomingCall( ss, t );
//			//missedCalls.append( incomingCall );
//			//incomingCall = IncomingCall( ss, t );
//			//missedCalls.append( incomingCall );
//
//			//updateCallRegister();
//
//			// Ringing tone
//			//QSettings settings;
//			//QString p = "/settings/General";
//			_isRingingTone = true;// ( settings.value( p + "/ringingtone", "No" ).toString().toUpper() == "YES" );
//
//			_ringCount = 0;
//			_pRingTimer->setSingleShot(true);
//			_pRingTimer->start( ringTime_1 );
//		}
//
//		// REMOTE
//		ui.remote->setText( _pSipCallMember->getUri().uri() );
//
//		_subject = _pSipCall->getSubject();
//		//_pHangupButton->setEnabled( true );
//		//_pHangupButton->setFocus();
//		if( _pSipCallMember->getState() == SipCallMember::state_InviteRequested )
//		{
//			////hidebutton->setEnabled( false );
//			////_pDialButton->setEnabled( false );
//			//_pHangupButton->setEnabled( true );
//			_currentState = Calling;
//		}
//		else if( _pSipCallMember->getState() == SipCallMember::state_RequestingInvite )
//		{
//			////_pDialButton->setText( tr("Accept") );
//			//_pHangupButton->setText( tr("Reject") );
//			////hidebutton->setEnabled( false );
//			////_pDialButton->setEnabled( true );
//			//_pHangupButton->setEnabled( true );
//			_currentState = Called;
//		}
//		else
//		{
//			////hidebutton->setEnabled( false );
//			////_pDialButton->setEnabled( false );
//			//_pHangupButton->setEnabled( true );
//			_currentState = Connected;
//			//if (!dtmfsenderTimer->isActive())
//			//  dtmfsenderTimer->start(dtmfsenderdelay->value() * 10);
//		}
//	}
//	else
//	{
//		//_pCurrentStatus->setText( QString::null );
//		emit statusChanged(QString::null);
//		if( _pSipCall->getCallType() == SipCall::videoCall )
//		{
//			//setCaption( getUserPrefix() + QString( tr("Outgoing Video Call: ") ) + call->getCallId() );
//			//holdbutton->setEnabled( false );
//			//transferbutton->setEnabled( false );
//		}
//		else
//		{
//			//setCaption( getUserPrefix() + QString( tr("Outgoing Call: ") ) + call->getCallId() );
//			//holdbutton->setEnabled( true );
//			//transferbutton->setEnabled( true );
//		}
//		_subject = _pSipCall->getSubject();
//
//		ui.remote->setText( QString::null );
//
//		////hidebutton->setEnabled( true );
//		////_pDialButton->setEnabled( true );
//		////_pDialButton->setFocus();
//		//_pHangupButton->setEnabled( false );
//		_currentState = PreDial;
//	}
//	updateAudioStatus();
//}

//void SipPhoneWidget::updateAudioStatus( void )
//{
//	if( _pAudioContoller->getCurrentCall() == _pSipCall )
//	{
//		if( _pAudioContoller->isRemoteHold() )
//			//_pCurrentAudioStatus->setText( tr("Attached [holding]") );
//			emit audioStatusChanged(tr("Attached [holding]"));
//		else
//			//_pCurrentAudioStatus->setText( tr("Attached [active]") );
//			emit audioStatusChanged(tr("Attached [active]"));
//	}
//	else
//	{
//		//_pCurrentAudioStatus->setText( tr("Unattached") );
//		emit audioStatusChanged(tr("Unattached"));
//	}
//}

void SipPhoneWidget::audioOutputDead( void )
{
	dbgPrintf( "KCallAudio: Broken output pipe, disconnecting unpolitely\n" );
	qDebug("SipPhoneWidget::audioOutputDead");
	forceDisconnect();
}

void SipPhoneWidget::setRemote( QString newremote )
{
	ui.remote->setText( newremote );
}

void SipPhoneWidget::remotePictureShow(const QImage& img)
{
	//ui.lblRemoteImage->setPixmap(QPixmap::fromImage(img));
	//SetCurrImage(img);
	SetRemoteImage(img);
}


void SipPhoneWidget::localPictureShow(const QImage& img)
{
	SetCurrImage(img);
	//SetRemoteImage(img);
}

void SipPhoneWidget::moveCurPicLabel(const QPoint & point)
{
	_pCurrPic->move(point);

	QRect remouteRect = rect();
	QRect localRect = _pCurrPic->geometry();
	localRect.moveTo(point);

	curPicAlign = 0;
	if (localRect.left() <= QImageLabel::spacing+2)
		curPicAlign |= Qt::AlignLeft;
	else if (localRect.right() >= remouteRect.right()-QImageLabel::spacing-2)
		curPicAlign |= Qt::AlignRight;
	if (localRect.top() <= QImageLabel::spacing+2)
		curPicAlign |= Qt::AlignTop;
	else if (localRect.bottom() >= remouteRect.bottom()-QImageLabel::spacing-2)
		curPicAlign |= Qt::AlignBottom;
}

void SipPhoneWidget::clickDial()
{
	//dialClicked();
}

void SipPhoneWidget::clickHangup()
{
	emit hangupCall();
}

//void SipPhoneWidget::pleaseDial( const SipUri &dialuri )
//{
//	ui.remote->setText( dialuri.reqUri() );
//	//dialClicked();
//}

//void SipPhoneWidget::dialClicked( void )
//{
////	// Multi-purpose buttons hack
////	if( _currentState == Called )
////	{
//////#pragma message("**** setAutoDelete не нужен KCallWidget::dialClicked")
////		//missedCalls.setAutoDelete( false );
////
////		//missedCalls.remove( incomingCall );
////
////		//missedCalls.setAutoDelete( true );
////
////		//receivedCalls.append( incomingCall );
////		//updateCallRegister();
////		acceptCall();
////		return;
////	}
////
////	if( ui.remote->text().length() == 0 )
////	{
////		QMessageBox::critical( this, tr("Error: No Destination"), tr("You must specify someone to call.") );
////		return;
////	}
////
////	QString strRemoteUri;
////	QString s = ui.remote->text();
////	if( s.contains( '[' ) && s.contains( ']' ) )
////	{
////		strRemoteUri = s.mid( s.indexOf( '[' ) + 1, s.indexOf( ']' ) - s.indexOf( '[' ) - 1 );
////	}
////	else
////	{
////		if( s.left( 4 ).toLower() != "tel:" )
////		{
////			if( s.left( 4 ).toLower() != "sip:" )
////			{
////				s = "sip:" + s;
////			}
////			if( !s.contains( '@' ) )
////			{
////				s = s + "@" + _pSipCall->getHostname();
////			}
////		}
////		strRemoteUri = s;
////	}
////
////	//////QSettings settings;
////	//////QString p = "/settings/Call register/";
////	//////QString str;
////	//////QString label;
////
////	//////label = p + "/Dialled" + str.setNum( 0 );
////	//////settings.setValue(label, QString(ui.remote->text().toLatin1()) );
////
////	////////for( int i = 0; i < remote->count(); i++ )
////	////////{
////	////////  label = p + "/Dialled" + str.setNum( 0 );
////	////////  //settings.writeEntry(label, remote->text(i).toLatin1() );
////	////////  settings.setValue(label, QString(ui.remote->text().toLatin1()) );
////	////////}
////	////////label = p + "/Dialled" + str.setNum( remote->count() );
////
////	//////label = p + "/Dialled1";
////	//////settings.setValue( label, "" );
////
////	SipUri remoteuri( strRemoteUri );
////
////	//remoteuri.setUsername("Buratini");
////	//remoteuri.setPassword("8uratino");
////
////	_pAudioContoller->setRtpCodec( codecUnknown );
////	_pAudioContoller->setVideoRtpCodec( codecUnknown );
////
////	_pSipCallMember = new SipCallMember( _pSipCall, remoteuri );
////	connect( _pSipCallMember, SIGNAL( statusUpdated( SipCallMember * ) ), this, SLOT( callMemberStatusUpdated() ) );
////	connect( _pSipCallMember, SIGNAL( statusUpdated( SipCallMember * ) ), _pSipAuthentication, SLOT( authRequest( SipCallMember * ) ) );
////
////	_pAudioContoller->setCurrentCall( _pSipCall );
////	_pAudioContoller->attachToCallMember( _pSipCallMember ); // ПОПОВ
////
////	SdpMessage invSdpMsg = _pAudioContoller->audioOut();
////	QString sdpMessageStr = invSdpMsg.message( _pAudioContoller->getRtpCodec(), _pAudioContoller->getVideoRtpCodec(), _pAudioContoller->getBodyMask() );
////
////	_pSipCallMember->requestInvite(sdpMessageStr , MimeContentType( "application/sdp" ) );
////
////	////_pDialButton->setEnabled( false );
////	//_pHangupButton->setEnabled( true );
////	_currentState = Calling;
////	//_pCurrentStatus->setText( _pSipCallMember->getLocalStatusDescription() );
////	emit statusChanged(_pSipCallMember->getLocalStatusDescription());
//}



// ПОПОВ Переделать ф-ию KCallWidget::acceptCallTimeout
//void SipPhoneWidget::acceptCallTimeout( void )
//{
//	//if( _pAudioContoller->checkCodec( _pSipCallMember ) )
//	//{
//	//	_pAudioContoller->setCurrentCall( _pSipCall );
//	//	_pAudioContoller->attachToCallMember( _pSipCallMember );
//
//	//	SdpMessage invitee = _pAudioContoller->audioOut();
//
//	//	QString sdpMessage = invitee.message( _pAudioContoller->getRtpCodec(), _pAudioContoller->getVideoRtpCodec(), _pAudioContoller->getBodyMask() );
//
//	//	_pSipCallMember->acceptInvite(sdpMessage , MimeContentType( "application/sdp" ) );
//	//	////_pDialButton->setText( tr("Dial") );
//	//	//_pHangupButton->setText( tr("Hangup") );
//	//	////hidebutton->setEnabled( false );
//	//	////_pDialButton->setEnabled( false );
//	//	//_pHangupButton->setEnabled( true );
//	//}
//	//else
//	//{
//	//	_pSipCallMember->notAcceptableHere();
//	//	QMessageBox::information( this, tr("Accept Call"), tr("Accepted codec not found.") );
//	//}
//}




void SipPhoneWidget::acceptCall( void )
{
	_pRingTimer->stop();

	_pAcceptCallTimer->setSingleShot(true);
	//_pAcceptCallTimer->start( acceptTime );
}


//void SipPhoneWidget::hangupCall( void )
//{
//	//if(!_pHangupButton || !_pHangupButton->isEnabled() )
//	//{
//	//	return;
//	//}
//
//	_pRingTimer->stop();
//	_isHangupInitiator = true;
//
//	//// Reject call if that's our current state
//	//if( _currentState == Called )
//	//{
//	//	_pSipCallMember->declineInvite();
//	//	//setHide();
//	//	//setHidden(true);
//	//	deleteLater();
//	//	emit callWasHangup();
//	//}
//	//else if(_pSipCall && _pSipCall->getCallStatus() != SipCall::callDead )
//	//{
//	//	//_pHangupButton->setEnabled( false );
//	//	if( _pSipCallMember->getState() == SipCallMember::state_Connected )
//	//	{
//	//		_pSipCallMember->requestDisconnect();
//	//		// ВНИМАНИЕ Проверка! Отключение ресурсов звонка по нажатию кнопки hangupCall
//	//		if( _pAudioContoller->getCurrentCall() == _pSipCall )
//	//		{
//	//			_pAudioContoller->detachFromCall();
//	//		}
//	//	}
//	//	else
//	//	{
//	//		_pSipCallMember->cancelTransaction();
//	//		if( _pAudioContoller->getCurrentCall() == _pSipCall )
//	//		{
//	//			_pAudioContoller->detachFromCall();
//	//		}
//	//	}
//	//	//setHidden(true);
//	//	deleteLater();
//	//	emit callWasHangup();
//	//}
//}


//void SipPhoneWidget::handleRedirect( void )
//{
//	dbgPrintf( "KCallWidget: Handling redirect...\n" );
//	//Q3ValueList<SipUri>::Iterator it;
//
//	SipUriList urilist = _pSipCallMember->getRedirectList();
//	while ( urilist.getListLength() > 0 )
//	{
//		SipUri redirto = urilist.getPriorContact();
//		QMessageBox mb( tr("Redirect"),
//			tr("Call redirected to: ") + "\n" + redirto.reqUri() + "\n\n" +
//			tr("Do you want to proceed ? "),
//			QMessageBox::Information,
//			QMessageBox::Yes | QMessageBox::Default,
//			QMessageBox::No,
//			QMessageBox::Cancel | QMessageBox::Escape );
//		switch( mb.exec() )
//		{
//		case QMessageBox::No:
//			continue;
//		case QMessageBox::Cancel:
//			forceDisconnect();
//			return;
//		}
//		if( _pAudioContoller->getCurrentCall() == _pSipCall )
//		{
//			_pAudioContoller->detachFromCall();
//		}
//		if( _pSipCallMember )
//			disconnect( _pSipCallMember, 0, this, 0 );
//
//		_pSipCallMember = 0;
//		if( _pSipCall )
//		{
//			delete _pSipCall;
//			_pSipCall = 0;
//		}
//
//		redirectCall( redirto, _subject );
//		//setHide();
//		return;
//	}
//	forceDisconnect();
//}
