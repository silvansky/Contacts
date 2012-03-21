#ifndef RCALLCONTROL_H
#define RCALLCONTROL_H

#include <QWidget>

#ifdef USE_PHONON
# include <Phonon/Phonon>
# include <Phonon/MediaSource>
#else
# include <QSound>
#endif

#include "ui_rcallcontrol.h"

#include <definitions/menuicons.h>
#include <definitions/toolbargroups.h>
#include <definitions/resources.h>
#include <definitions/stylesheets.h>
#include <definitions/soundfiles.h>
#include <utils/iconstorage.h>
#include <utils/stylestorage.h>
#include <utils/jid.h>

class RCallControl : public QWidget
{
	Q_OBJECT

public:
	enum CallSide
	{
		Receiver = 0,
		Caller = 1
	};
	enum CallStatus
	{
		Undefined,
		Register,		// Регистрация
		Accepted,		// Звонок принят
		Hangup,			// Положили трубку
		Ringing,		// Звонок
		RingTimeout,// Вышло время попытки звонка (нет ответа)
		CallError		// Иная ошибка
		//Trying,			// Попытка осуществить вызов
	};

public:
	RCallControl(QString sid, CallSide, QWidget *parent = 0);
	~RCallControl();

	CallSide side() const {return _callSide;}
	QString getSessionID() const {return _sid;}
	CallStatus status() const {return _callStatus;}

	Jid getStreamJid() const { return _streamId; }
	QString getMetaId() const { return _metaId; }

	void setSessionId(const QString& sid);
	void setStreamJid(const Jid& AStreamJid);
	void setMetaId(const QString& AMetaId);

signals:
	void camStateChange(bool);
	void camResolutionChange(bool);
	void startCamera();
	void stopCamera();
	void micStateChange(bool);
	void micVolumeChange(int);

signals: // Сигналы управляющие звонком
	void acceptCall();		// Принять звонок
	void hangupCall();		// Отбой звонка
	void abortCall();			// Отмена звонка
	void redialCall();		// Сигнал посылает вызывающая сторона в случае повторного звонка
	void callbackCall();	// Сигнал посылает вызываемая сторона в случае обратного звонка на пропущенный вызов
	void closeAndDelete(bool);

public slots:
	void statusTextChange(const QString&);
	void callStatusChange(CallStatus);
	void callSideChange(CallSide);
	void setCameraEnabled(bool);

	void setMicEnabled(bool);
	void setVolumeEnabled(bool);

signals:
	void statusTextChanged(const QString&);
	void callStatusChanged(CallStatus);
	void callSideChanged(CallSide);

protected slots:
	void onAccept();
	void onHangup();
	void onCamStateChange(bool);

protected:
	void closeEvent(QCloseEvent *);
	void paintEvent(QPaintEvent *);
	void playSignal(CallStatus status, int loops);
	void stopSignal();
	void playSignalWait(int loops);
	void playSignalBusy(int loops);
	void playSignalRinging(int loops);
	
private:
	IconStorage* iconStorage;
	QIcon acceptIcon;
	QIcon hangupIcon;

#ifdef USE_PHONON
	Phonon::MediaObject *FMediaObject;
	Phonon::AudioOutput *FAudioOutput;
#else
	QSound *_pSoundWait;
	QSound *_pSoundBusy;
	QSound *_pSoundRinging;
#endif

	Jid _streamId;
	QString _metaId;
	QString _sid; // Идентификатор сессии которой управляем в данный момент
	CallSide _callSide;			// Идентифицирует сторону звонка (звонящий/принимающий)
	CallStatus _callStatus;	// Статус звонка
	Ui::RCallControl ui;
};

#endif // RCALLCONTROL_H
