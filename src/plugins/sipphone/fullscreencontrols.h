#ifndef FULLSCREENCONTROLS_H
#define FULLSCREENCONTROLS_H

#include <QWidget>
#include "ui_fullscreencontrols.h"

class FullScreenControls : public QWidget
{
	Q_OBJECT

public:
	FullScreenControls(QWidget *parent = 0);
	~FullScreenControls();

	//AVControl* avControl() const { return ui.wgtAVControl; }

public slots:
	void SetCameraOn(bool);
	void setFullScreen(bool);

signals:
	void camStateChange(bool);
	void micStateChange(bool);
	void micVolumeChange(int);
	void hangup();
	void fullScreenState(bool);


private:
	Ui::FullScreenControls ui;
};

#endif // FULLSCREENCONTROLS_H
