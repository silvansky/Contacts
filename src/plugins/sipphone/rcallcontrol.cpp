#include "rcallcontrol.h"

#include <QMessageBox>
#include <QPainter>
#include <utils/log.h>

RCallControl::RCallControl(CallSide callSide, QWidget *parent)
	: QWidget(parent), _callStatus(Undefined), _sid("")
{
	ui.setupUi(this);

	setProperty("ringing", true);


#ifdef QT_PHONON_LIB
	FMediaObject = NULL;
	FAudioOutput = NULL;
#else
	_pSoundWait = NULL;
	_pSoundBusy = NULL;
	_pSoundRinging = NULL;
#endif


	StyleStorage::staticStorage(RSR_STORAGE_STYLESHEETS)->insertAutoStyle(this, STS_SIPPHONE);

	//IconStorage * iconStorage;
	//QIcon currentIcon;

	iconStorage = IconStorage::staticStorage(RSR_STORAGE_MENUICONS);
	acceptIcon = iconStorage->getIcon(MNI_SIPPHONE_BTN_ACCEPT);
	hangupIcon = iconStorage->getIcon(MNI_SIPPHONE_BTN_HANGUP);
	//if (!currentIcon.isNull())
	//	iconLabel->setPixmap(currentIcon.pixmap(16, QIcon::Normal, QIcon::On));

	ui.btnAccept->setIcon(acceptIcon);
	ui.btnHangup->setIcon(hangupIcon);
	//ui.btnAccept->setPixmap(acceptIcon.pixmap(16, QIcon::Normal, QIcon::On));
	//ui.btnHangup->setPixmap(hangupIcon.pixmap(16, QIcon::Normal, QIcon::On));


	_callSide = callSide;
	if(callSide == Receiver)
	{
		ui.btnAccept->show();
		ui.btnHangup->show();
	}
	else
	{
		ui.btnAccept->hide();
		ui.btnHangup->show();
	}

	connect(ui.wgtAVControl, SIGNAL(camStateChange(bool)), SIGNAL(camStateChange(bool)));
	connect(ui.wgtAVControl, SIGNAL(micStateChange(bool)), SIGNAL(micStateChange(bool)));
	connect(ui.wgtAVControl, SIGNAL(micVolumeChange(int)), SIGNAL(micVolumeChange(int)));

	connect(ui.btnAccept, SIGNAL(clicked()), this, SLOT(onAccept()));
	connect(ui.btnHangup, SIGNAL(clicked()), this, SLOT(onHangup()));

	callStatusChange(Register);
}


RCallControl::RCallControl(QString sid, CallSide callSide, QWidget *parent)
: QWidget(parent), _callStatus(Undefined), _sid(sid)
{
	ui.setupUi(this);

#ifdef QT_PHONON_LIB
	FMediaObject = NULL;
	FAudioOutput = NULL;
#else
	_pSoundWait = NULL;
	_pSoundBusy = NULL;
	_pSoundRinging = NULL;
#endif


	StyleStorage::staticStorage(RSR_STORAGE_STYLESHEETS)->insertAutoStyle(this, STS_SIPPHONE);

	//IconStorage * iconStorage;
	//QIcon currentIcon;

	iconStorage = IconStorage::staticStorage(RSR_STORAGE_MENUICONS);
	acceptIcon = iconStorage->getIcon(MNI_SIPPHONE_BTN_ACCEPT);
	hangupIcon = iconStorage->getIcon(MNI_SIPPHONE_BTN_HANGUP);
	//if (!currentIcon.isNull())
	//	iconLabel->setPixmap(currentIcon.pixmap(16, QIcon::Normal, QIcon::On));

	ui.btnAccept->setIcon(acceptIcon);
	ui.btnHangup->setIcon(hangupIcon);
	//ui.btnAccept->setPixmap(acceptIcon.pixmap(16, QIcon::Normal, QIcon::On));
	//ui.btnHangup->setPixmap(hangupIcon.pixmap(16, QIcon::Normal, QIcon::On));


	_callSide = callSide;
	if(callSide == Receiver)
	{
		ui.btnAccept->show();
		ui.btnHangup->show();
	}
	else
	{
		ui.btnAccept->hide();
		ui.btnHangup->show();
	}

	//if(_sid == "")
	//{
	//	callStatusChange(Register);
	//}

	connect(ui.wgtAVControl, SIGNAL(camStateChange(bool)), SIGNAL(camStateChange(bool)));
	connect(ui.wgtAVControl, SIGNAL(camStateChange(bool)), SLOT(onCamStateChange(bool)));
	connect(ui.wgtAVControl, SIGNAL(micStateChange(bool)), SIGNAL(micStateChange(bool)));
	connect(ui.wgtAVControl, SIGNAL(micVolumeChange(int)), SIGNAL(micVolumeChange(int)));


	connect(ui.btnAccept, SIGNAL(clicked()), this, SLOT(onAccept()));
	connect(ui.btnHangup, SIGNAL(clicked()), this, SLOT(onHangup()));

	callStatusChange(Ringing);
}

RCallControl::~RCallControl()
{
	//close();
#ifdef QT_PHONON_LIB
	delete FMediaObject;
	delete FAudioOutput;
#else
	stopSignal();
	delete _pSoundWait;
	delete _pSoundBusy;
	delete _pSoundRinging;
#endif

	emit closeAndDelete(false);
}

void RCallControl::setSessionId(const QString& sid)
{
	_sid = sid;
}

void RCallControl::setStreamJid(const Jid& AStreamJid)
{
	_streamId = AStreamJid;
}

void RCallControl::setMetaId(const QString& AMetaId)
{
	_metaId = AMetaId;
}

void RCallControl::statusTextChange(const QString& text)
{
	ui.lblStatus->setText(text);
	emit statusTextChanged(text);
}

void RCallControl::onAccept()
{
	//QMessageBox::information(NULL, "Accept", "");
	if(_callSide == Caller)
	{
		//switch(_callStatus)
		//{
		//	case Accepted:
		//	break;
		//	case Hangup:
		//	case RingTimeout:
		//	case CallError:
		//		emit redialCall();
		//	break;
		//	default:
		//	break;
		//}
		if(_callStatus == Accepted)
		{
			// Не должно такого быть
		}
		else if(_callStatus == Hangup)
		{
			emit redialCall();
		}
		else if(_callStatus == Ringing)
		{
			// Не может быть
		}
		else if(_callStatus == RingTimeout)
		{
			emit redialCall();
		}
		else if(_callStatus == CallError)
		{
			emit redialCall();
		}
	}
	if(_callSide == Receiver)
	{
		//switch(_callStatus)
		//{
		//case Ringing:
		//	emit acceptCall();
		//	break;
		//case RingTimeout:
		//	emit callbackCall();
		//	break;
		//default:
		//	break;
		//}
		if(_callStatus == Accepted)
		{
			// Не может быть
		}
		else if(_callStatus == Hangup)
		{
			// Не может быть
		}
		else if(_callStatus == Ringing)
		{
			emit acceptCall();
		}
		else if(_callStatus == RingTimeout)
		{
			emit callbackCall();
		}
		else if(_callStatus == CallError)
		{
			// Не может быть
		}
	}
}

void RCallControl::onHangup()
{
	//QMessageBox::information(NULL, "Hangup", "");
	if(_callSide == Caller)
	{
		//switch(_callStatus)
		//{
		//	case Accepted:
		//	case Ringing:
		//		emit hangupCall();
		//	break;
		//	default:
		//	break;
		//}
		if(_callStatus == Undefined)
		{
			return;
		}
		else if(_callStatus == Accepted)
		{
			emit hangupCall();
		}
		else if(_callStatus == Hangup)
		{
			//emit killThis();
			//return;
			// Не может быть
		}
		else if(_callStatus == Register)
		{
			callStatusChange(Undefined);
			emit abortCall();//hangupCall();
		}
		else if(_callStatus == Ringing)
		{
			callStatusChange(Undefined);
			emit abortCall();//hangupCall();
		}
		else if(_callStatus == RingTimeout)
		{
			// Не может быть
		}
		else if(_callStatus == CallError)
		{
			// Не может быть
		}
		else
		{
			emit hangupCall();
		}
	}
	if(_callSide == Receiver)
	{
		//switch(_callStatus)
		//{
		//	case Accepted:
		//	case Ringing:
		//		emit hangupCall();
		//	break;
		//	default:
		//	break;
		//}
		if(_callStatus == Accepted)
		{
			emit hangupCall();
		}
		else if(_callStatus == Hangup)
		{
			// Не может быть
		}
		else if(_callStatus == Ringing)
		{
			//emit hangupCall();
			emit abortCall();
		}
		else if(_callStatus == Register)
		{
			callStatusChange(Undefined);
			emit abortCall();
		}
		else if(_callStatus == RingTimeout)
		{
			// Не может быть
		}
		else if(_callStatus == CallError)
		{
			// Не может быть
		}
		else
		{
			emit hangupCall();
		}
	}
}

void RCallControl::callSideChange(CallSide side)
{
	if(_callSide != side)
	{
		_callSide = side;
		emit callSideChanged(side);
	}
	else
		return;
}

void RCallControl::callStatusChange(CallStatus status)
{
	if(_callStatus != status)
	{
		_callStatus = status;
		emit callStatusChanged(status);
	}
	else
		return;

	stopSignal();

	setProperty("ringing", false);
	// TODO: change if - else if -> switch
	if(_callStatus == Accepted)
	{
		if(_callSide == Caller)
		{
			statusTextChange(tr("Accepted"));
			ui.btnAccept->hide();
			ui.btnHangup->show();
			ui.btnHangup->setEnabled(true);
			ui.btnHangup->setText(tr("Hangup"));
			//stopSignal();
		}
		else
		{
			statusTextChange(tr("Accepted"));
			ui.btnAccept->hide();
			ui.btnHangup->show();
			ui.btnHangup->setEnabled(true);
			ui.btnHangup->setText(tr("Hangup"));
			//stopSignal();
		}
	}
	else if(_callStatus == Hangup)
	{
		if(_callSide == Caller)
		{
			Log(QString("[Call failure]"));
			statusTextChange(tr("Calling failure..."));
			ui.btnAccept->show();
			ui.btnAccept->setEnabled(true);
			ui.btnAccept->setText(tr("Call again"));
			ui.btnHangup->hide();
			//ui.btnHangup->setText(tr("Abort"));
			playSignalBusy(3);
		}
		else
		{
			//stopSignal();
			// У вызываемого абонента при отмене вызова панель пропадает
		}
	}
	else if(_callStatus == Register)
	{
		if(_callSide == Caller)
		{
			_sid = "";
			statusTextChange(tr("Registering on call server..."));
			ui.btnAccept->hide();
			ui.btnHangup->show();
			ui.btnHangup->setText(tr("Cancel"));
			ui.btnHangup->setEnabled(false);
			//stopSignal();
		}
		else
		{
			statusTextChange(tr("Registering on call server..."));
			ui.btnAccept->show();
			ui.btnAccept->setEnabled(false);
			ui.btnHangup->show();
			ui.btnHangup->setEnabled(true);
			//stopSignal();
		}
	}
	else if(_callStatus == Ringing)
	{
		if(_callSide == Caller)
		{
			statusTextChange(tr("Outgoing Call..."));
			ui.btnAccept->hide();
			ui.btnHangup->show();
			ui.btnHangup->setEnabled(true);
			ui.btnHangup->setText(tr("Hangup"));

			//playSignal(Ringing, 30);
			playSignalWait(30);
		}
		else
		{
			statusTextChange(tr("Incoming Call..."));
			ui.btnAccept->show();
			ui.btnAccept->setEnabled(true);
			ui.btnAccept->setText(tr("Accept"));

			ui.btnHangup->show();
			ui.btnHangup->setEnabled(true);
			ui.btnHangup->setText(tr("Hangup"));
			setProperty("ringing", true);

			playSignalRinging(2);
			qDebug("playSignalRinging");
			//playSignal(Ringing, 30);
		}
	}
	else if(_callStatus == RingTimeout)
	{
		if(_callSide == Caller)
		{
			statusTextChange(tr("No answer..."));
			ui.btnAccept->show();
			ui.btnAccept->setEnabled(true);
			ui.btnAccept->setText(tr("Call again"));
			ui.btnHangup->hide();
			//ui.btnHangup->setText(tr("Abort"));
			//playSignal(RingTimeout, 3);
		}
		else
		{
			statusTextChange(tr("Missed Call..."));
			ui.btnAccept->show();
			ui.btnAccept->setEnabled(true);
			ui.btnAccept->setText(tr("Callback"));
			ui.btnHangup->setText(tr(""));
			ui.btnHangup->hide();
			//stopSignal();
		}
	}
	//else if(_callStatus == CallStatus::Trying)
	//{
	//	if(_callSide == Caller)
	//	{

	//	}
	//	else
	//	{

	//	}
	//}
	else // Прочие ошибки звонка
	{
		if(_callSide == Caller)
		{
			Log(QString("[Call error]"));
			statusTextChange(tr("Call Error..."));
			ui.btnAccept->show();
			ui.btnAccept->setEnabled(true);
			ui.btnAccept->setText(tr("Call again"));
			ui.btnHangup->setText(tr(""));
			ui.btnHangup->hide();
			//playSignal(CallError, 3);
		}
		else
		{
			// У вызываемого абонента панель пропадает
		}
	}
	setStyleSheet(styleSheet());
}

void RCallControl::closeEvent(QCloseEvent *)
{
	onHangup();
	//emit closeAndDelete(false);
}

void RCallControl::paintEvent(QPaintEvent *)
{
	QStyleOption opt;
	opt.init(this);
	QPainter p(this);
	style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
}

void RCallControl::onCamStateChange(bool state)
{
	if(state)
	{
		emit startCamera();
	}
	else
	{
		emit stopCamera();
	}
}


void RCallControl::playSignalWait(int loops)
{
	QString soundFile = FileStorage::staticStorage(RSR_STORAGE_SOUNDS)->fileFullName(SDF_SIPPHONE_CALL_WAIT);
	if (soundFile.isEmpty())
	{
		return;
	}

#ifdef QT_PHONON_LIB

	if (!FMediaObject)
	{
		FMediaObject = new Phonon::MediaObject(this);
		FAudioOutput = new Phonon::AudioOutput(Phonon::CommunicationCategory, this);
		Phonon::createPath(FMediaObject, FAudioOutput);
	}

	if(FMediaObject->state() == Phonon::PlayingState)
	{
		FMediaObject->clear();
		FMediaObject->stop();
	}

	if (FMediaObject->state() != Phonon::PlayingState)
	{
		Phonon::MediaSource ms(soundFile);
		for(int i=0; i<loops; i++)
		{
			FMediaObject->enqueue(ms);
		}
		FMediaObject->play();
	}
#else
	if (!soundFile.isEmpty())
	{
		if (QSound::isAvailable())
		{
			if(_pSoundWait)
			{
				//_pSoundWait->setLoops(0);
				_pSoundWait->stop();
				//usleep(50);
				qApp->processEvents();
				delete _pSoundWait;
				_pSoundWait = NULL;
			}

			_pSoundWait = new QSound(soundFile);
			_pSoundWait->setLoops(loops);
			_pSoundWait->play();
		}
	}

#endif
}

void RCallControl::playSignalBusy(int loops)
{
	QString soundFile = FileStorage::staticStorage(RSR_STORAGE_SOUNDS)->fileFullName(SDF_SIPPHONE_CALL_BUSY);
	if (soundFile.isEmpty())
	{
		return;
	}

#ifdef QT_PHONON_LIB

	if (!FMediaObject)
	{
		FMediaObject = new Phonon::MediaObject(this);
		FAudioOutput = new Phonon::AudioOutput(Phonon::CommunicationCategory, this);
		Phonon::createPath(FMediaObject, FAudioOutput);
	}

	if(FMediaObject->state() == Phonon::PlayingState)
	{
		FMediaObject->clear();
		FMediaObject->stop();
	}

	if (FMediaObject->state() != Phonon::PlayingState)
	{
		Phonon::MediaSource ms(soundFile);
		for(int i=0; i<loops; i++)
		{
			FMediaObject->enqueue(ms);
		}
		FMediaObject->play();
	}
#else
	if (!soundFile.isEmpty())
	{
		if (QSound::isAvailable())
		{
			if(_pSoundBusy)
			{
				//_pSoundBusy->setLoops(0);
				_pSoundBusy->stop();
				//usleep(50);
				qApp->processEvents();
				delete _pSoundBusy;
				_pSoundBusy = NULL;
			}

			_pSoundBusy = new QSound(soundFile);
			_pSoundBusy->setLoops(loops);
			_pSoundBusy->play();
		}
	}
#endif
}

void RCallControl::playSignalRinging(int loops)
{
	QString soundFile = FileStorage::staticStorage(RSR_STORAGE_SOUNDS)->fileFullName(SDF_SIPPHONE_CALL_RINGING);
	if (soundFile.isEmpty())
	{
		return;
	}

#ifdef QT_PHONON_LIB

	if (!FMediaObject)
	{
		FMediaObject = new Phonon::MediaObject(this);
		FAudioOutput = new Phonon::AudioOutput(Phonon::CommunicationCategory, this);
		Phonon::createPath(FMediaObject, FAudioOutput);
	}

	if(FMediaObject->state() == Phonon::PlayingState)
	{
		FMediaObject->clear();
		FMediaObject->stop();
	}

	if (FMediaObject->state() != Phonon::PlayingState)
	{
		Phonon::MediaSource ms(soundFile);
		for(int i=0; i<loops; i++)
		{
			FMediaObject->enqueue(ms);
		}
		FMediaObject->play();
	}
#else

	if (!soundFile.isEmpty())
	{
		if (QSound::isAvailable())
		{
			if(_pSoundRinging)
			{
				//_pSoundRinging->setLoops(0);
				_pSoundRinging->stop();
				//usleep(50);
				qApp->processEvents();
				delete _pSoundRinging;
				_pSoundRinging = NULL;
			}

			_pSoundRinging = new QSound(soundFile);
			_pSoundRinging->setLoops(loops);//loops);
			_pSoundRinging->play();
		}
	}
#endif
}


void RCallControl::playSignal(CallStatus status, int loops)
{
//	QString soundName;
//
//	if(status == Ringing)
//	{
//		if(_callSide == Caller)
//			soundName = SDF_SIPPHONE_CALL_WAIT;
//		else
//			soundName = SDF_SIPPHONE_CALL_RINGING;
//	}
//	else if(status == Hangup || status == RingTimeout || status == CallError)
//	{
//		soundName = SDF_SIPPHONE_CALL_BUSY;
//	}
//
//	QString soundFile = FileStorage::staticStorage(RSR_STORAGE_SOUNDS)->fileFullName(soundName);
//	if (!soundFile.isEmpty())
//	{
//		if (QSound::isAvailable())
//		{
//			//if(_pSound)
//			//{
//			//	if(!_pSound->isFinished())
//			//	{
//			//		_pSound->setLoops(0);
//			//		_pSound->stop();
//			//	}
//			//	delete _pSound;
//			//	_pSound = NULL;
//			//}
//
//			//if (!_pSound || (_pSound && _pSound->isFinished()))
//			//{
//			//  delete _pSound;
//			if(_pSound)
//			{
//				_pSound->setLoops(0);
//				_pSound->stop();
//				delete _pSound;
//				_pSound = NULL;
//			}
//				
//			_pSound = new QSound(soundFile);
//			_pSound->setLoops(loops);
//			_pSound->play();
//
//			//}
//		}
//	}
}

void RCallControl::stopSignal()
{

#ifdef QT_PHONON_LIB
	//if (!FMediaObject)
	//{
	//	FMediaObject = new Phonon::MediaObject(this);
	//	FAudioOutput = new Phonon::AudioOutput(Phonon::CommunicationCategory, this);
	//	Phonon::createPath(FMediaObject, FAudioOutput);
	//}
	//if (FMediaObject->state() != Phonon::PlayingState)
	//{
	//	//FMediaObject->setCurrentSource(soundFile);
	//	FMediaObject->setQueue();
	//	FMediaObject->play();
	//}

	if(FMediaObject)
	{
		FMediaObject->clear();
		FMediaObject->stop();
	}


#else

	if (QSound::isAvailable())
	{
		if(_pSoundWait)
		{
			if(!_pSoundWait->isFinished())
			{
				_pSoundWait->setLoops(1);
				_pSoundWait->play();
				_pSoundWait->stop();
				//usleep(50);
				qApp->processEvents();
			}
			delete _pSoundWait;
			_pSoundWait = NULL;
		}
		if(_pSoundBusy)
		{
			if(!_pSoundBusy->isFinished())
			{
				_pSoundBusy->setLoops(1);
				_pSoundBusy->play();
				_pSoundBusy->stop();
				//usleep(50);
				qApp->processEvents();
			}
			delete _pSoundBusy;
			_pSoundBusy = NULL;
		}
		if(_pSoundRinging)
		{
			if(!_pSoundRinging->isFinished())
			{
				_pSoundRinging->setLoops(1);
				_pSoundRinging->play();
				_pSoundRinging->stop();
				//usleep(50);
				qApp->processEvents();
			}
			delete _pSoundRinging;
			_pSoundRinging = NULL;
		}
	}
#endif

}