#ifndef RCALLCONTROL_H
#define RCALLCONTROL_H

#include <QWidget>
#include "ui_rcallcontrol.h"

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
		Accepted,		// Звонок принят
		Hangup,			// Положили трубку
		Ringing,		// Звонок
		RingTimeout,// Вышло время попытки звонка (нет ответа)
		CallError		// Иная ошибка
		//Trying,			// Попытка осуществить вызов
	};

public:
	RCallControl(CallSide, QWidget *parent = 0);
	~RCallControl();

	CallSide side() {return _callSide;}

signals:
	void camStateChange(bool);
	void micStateChange(bool);
	void micVolumeChange(int);

signals: // Сигналы управляющие звонком
	void acceptCall();		// Принять звонок
	void hangupCall();		// Отбой звонка
	void redialCall();		// Сигнал посылает вызывающая сторона в случае повторного звонка
	void callbackCall();	// Сигнал посылает вызываемая сторона в случае обратного звонка на пропущенный вызов

public slots:
	void statusTextChanged(const QString&);
	void callStatusChanged(CallStatus);

protected slots:
	void onAccept();
	void onHangup();

private:
	CallSide _callSide;			// Идентифицирует сторону звонка (звонящий/принимающий)
	CallStatus _callStatus;	// Статус звонка
	Ui::RCallControl ui;
};

#endif // RCALLCONTROL_H
