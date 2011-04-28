#ifndef AVCONTROL_H
#define AVCONTROL_H

#include <QWidget>
#include "ui_avcontrol.h"

#include <definitions/resources.h>
#include <definitions/stylesheets.h>
#include <utils/stylestorage.h>


class BtnSynchro : public QObject
{
	Q_OBJECT
	int _refCount;
	~BtnSynchro(){}

	QList<QAbstractButton*> _buttons;

public:
	BtnSynchro(QAbstractButton* btn)  : _refCount(1)
	{
		_buttons.append(btn);
	}

	int AddRef(QAbstractButton* btn)
	{
		if(!_buttons.contains(btn)) // Не реально что такое может случиться, но тем не менее
			_buttons.append(btn);
		return _refCount++;
	}
	int Release(QAbstractButton* btn)
	{
		_refCount--;
		_buttons.removeOne(btn);
		if (_refCount == 0)
    {
			delete this;
      return 0;
    }
		return _refCount;
	}

signals:
	void stateChange(bool);

public slots:
	void onStateChange(bool);
};

class AVControl : public QWidget
{
	Q_OBJECT

	static BtnSynchro* __bSyncCamera;
	static BtnSynchro* __bSyncMic;

public:
	AVControl(QWidget *parent = 0);
	~AVControl();

	void setDark(bool isDark);

public slots:
	void SetCameraOn(bool);

signals:
	void camStateChange(bool);
	void micStateChange(bool);
	void micVolumeChange(int);
protected:
	void paintEvent(QPaintEvent *);

private:
	Ui::AVControl ui;
	bool _isDark;
};

#endif // AVCONTROL_H
