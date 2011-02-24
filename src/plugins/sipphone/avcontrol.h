#ifndef AVCONTROL_H
#define AVCONTROL_H

#include <QWidget>
#include "ui_avcontrol.h"

class AVControl : public QWidget
{
	Q_OBJECT

public:
	AVControl(QWidget *parent = 0);
	~AVControl();

public slots:
	void SetCameraOn(bool);

signals:
	void camStateChange(bool);
	void micStateChange(bool);
	void micVolumeChange(int);

private:
	Ui::AVControl ui;
};

#endif // AVCONTROL_H
