#ifndef FULLSCREENFORM_H
#define FULLSCREENFORM_H

#include <QWidget>

#include "ui_fullscreenform.h"
#include "qimagelabel.h"
#include "fullscreencontrols.h"

class FullScreenForm : public QWidget
{
	Q_OBJECT

public:
	FullScreenForm(QWidget *parent = 0);
	~FullScreenForm();

public slots:
	void SetCurrImage(const QImage& img);
	void SetRemoteImage(const QImage& img);

protected slots:
	void fullScreenStateChange(bool);

signals:
	void startCamera();
	void stopCamera();

public slots:
	void cameraStateChange(bool);

protected:
	void keyPressEvent(QKeyEvent *);
	//virtual void focusInEvent(QFocusEvent *);
	//virtual void focusOutEvent(QFocusEvent *);
	virtual void enterEvent(QEvent *);
	virtual void leaveEvent(QEvent *);
	//virtual void moveEvent(QMoveEvent *);
	virtual void resizeEvent(QResizeEvent *);
	//virtual void closeEvent(QCloseEvent *);

private:
	QImageLabel* _pCurrPic;
	QToolButton* _pShowCurrPic;
	FullScreenControls* _pControls;
private:
	Ui::FullScreenForm ui;
};

#endif // FULLSCREENFORM_H
