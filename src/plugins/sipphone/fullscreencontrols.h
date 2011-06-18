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
	void setCameraEnabled(bool isEnabled);
	void SetCameraOn(bool);
	void SetResolutionHigh(bool);

	void setFullScreen(bool);

	void setMicEnabled(bool isEnabled);
	void setVolumeEnabled(bool isEnabled);

	void onCamResolutionChange(bool isHigh);


signals:
	void camStateChange(bool);
	void camPresentChanged(bool);
	void camResolutionChange(bool);
	void micStateChange(bool);
	void micVolumeChange(int);
	void hangup();
	void fullScreenState(bool);
protected:
	void paintEvent(QPaintEvent *);
	


private:
	Ui::FullScreenControls ui;
};

#endif // FULLSCREENCONTROLS_H
