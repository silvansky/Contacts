#include "rcallcontrol.h"

#include <QMessageBox>

RCallControl::RCallControl(CallSide callSide, QWidget *parent)
	: QWidget(parent), _callStatus(Ringing)
{
	ui.setupUi(this);

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
}


RCallControl::RCallControl(QString sid, CallSide callSide, QWidget *parent)
: QWidget(parent), _callStatus(Ringing), _sid(sid)
{
	ui.setupUi(this);

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
}

RCallControl::~RCallControl()
{
	close();
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
		if(_callStatus == Accepted)
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
			emit abortCall();//hangupCall();
		}
		else if(_callStatus == Ringing)
		{
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


	if(_callStatus == Accepted)
	{
		if(_callSide == Caller)
		{
			statusTextChange(tr("ACCEPT"));
			ui.btnAccept->hide();
			ui.btnHangup->show();
			ui.btnHangup->setText(tr("Hangup"));
		}
		else
		{
			statusTextChange(tr("ACCEPT"));
			ui.btnAccept->hide();
			ui.btnHangup->show();
			ui.btnHangup->setText(tr("Hangup"));
		}
	}
	else if(_callStatus == Hangup)
	{
		if(_callSide == Caller)
		{
			statusTextChange(tr("Calling failure..."));
			ui.btnAccept->show();
			ui.btnHangup->hide();
			ui.btnAccept->setText(tr("Call again"));
			//ui.btnHangup->setText(tr("Abort"));
		}
		else
		{
			// У вызываемого абонента при отмене вызова панель пропадает
		}
	}
	else if(_callStatus == Register)
	{
		if(_callSide == Caller)
		{
			statusTextChange(tr("Registering..."));
			ui.btnAccept->hide();
			ui.btnHangup->show();
			ui.btnHangup->setText(tr("Hangup"));
		}
		else
		{
			//statusTextChange(tr("Incoming Call..."));
			//ui.btnAccept->show();
			//ui.btnHangup->show();
			//ui.btnAccept->setText(tr("Accept"));
			//ui.btnHangup->setText(tr("Hangup"));
		}
	}
	else if(_callStatus == Ringing)
	{
		if(_callSide == Caller)
		{
			statusTextChange(tr("Outgoing Call..."));
			ui.btnAccept->hide();
			ui.btnHangup->show();
			ui.btnHangup->setText(tr("Hangup"));
		}
		else
		{
			statusTextChange(tr("Incoming Call..."));
			ui.btnAccept->show();
			ui.btnHangup->show();
			ui.btnAccept->setText(tr("Accept"));
			ui.btnHangup->setText(tr("Hangup"));
		}
	}
	else if(_callStatus == RingTimeout)
	{
		if(_callSide == Caller)
		{
			statusTextChange(tr("No answer..."));
			ui.btnAccept->show();
			ui.btnHangup->hide();
			ui.btnAccept->setText(tr("Call again"));
			//ui.btnHangup->setText(tr("Abort"));
		}
		else
		{
			statusTextChange(tr("Missed Call..."));
			ui.btnAccept->show();
			ui.btnHangup->hide();
			ui.btnAccept->setText(tr("Callback"));
			ui.btnHangup->setText(tr(""));
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
			statusTextChange(tr("Call Error..."));
			ui.btnAccept->show();
			ui.btnHangup->hide();
			ui.btnAccept->setText(tr("Call again"));
			ui.btnHangup->setText(tr(""));
		}
		else
		{
			// У вызываемого абонента панель пропадает
		}
	}
}

void RCallControl::closeEvent(QCloseEvent *)
{
	onHangup();
	emit closeAndDelete(false);
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
