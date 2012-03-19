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
	BtnSynchro(QAbstractButton* btn);
	int AddRef(QAbstractButton* btn);
	int Release(QAbstractButton* btn);
signals:
	void stateChange(bool);
public slots:
	void setCheckState(bool);
	void setEnabledState(bool);
	void setToolTipState(const QString &);
};

class AVControl : public QWidget
{
	Q_OBJECT

	static BtnSynchro* __bSyncCamera;
	static BtnSynchro* __bSyncMic;
	static BtnSynchro* __bSyncHQ;

public:
	AVControl(QWidget *parent = 0);
	~AVControl();

	void setDark(bool isDark);
	void updateMicToolTip();
	void updateCamToolTip();
	void updateHQToolTip();

public slots:
	void setCameraOn(bool);
	void setCameraEnabled(bool);
	void setResolutionHigh(bool);
	void setMicOn(bool);
	void setMicEnabled(bool);
	void setVolumeEnabled(bool);
	void onCallStarted(int slot);
	void onCallEnded(int slot);

signals:
	void camStateChange(bool);
	void camPresentChanged(bool);
	void camResolutionChange(bool);
	void micStateChange(bool);
	void micPresentChanged(bool);
	void micVolumeChange(int);
	void volumePresentChanged(bool);

protected:
	void paintEvent(QPaintEvent *);
	
private slots:
	void onAudioSettings();

private:
	Ui::AVControl ui;
};

#endif // AVCONTROL_H
