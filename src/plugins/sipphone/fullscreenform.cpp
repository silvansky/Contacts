#include "fullscreenform.h"

#include <QResizeEvent>

#include "complexvideowidget.h"

FullScreenForm::FullScreenForm(QWidget *parent)
	: QWidget(parent)
{
	ui.setupUi(this);
	//ui.lblRemotePic->setScaledContents(true);
	//ui.lblRemotePic->setBaseSize(320, 240);

	_pCurrPic = new QImageLabel(ui.wgtRemoteImage);
	_pShowCurrPic = new QToolButton(ui.wgtRemoteImage);

	connect(_pCurrPic, SIGNAL(visibleState(bool)), _pShowCurrPic, SLOT(setHidden(bool)));
	connect(_pShowCurrPic, SIGNAL(clicked()), _pCurrPic, SLOT(show()));

	_pCurrPic->setFixedSize(160, 120);
	_pCurrPic->setMouseTracking(true);
	_pCurrPic->setScaledContents(true);

	_pShowCurrPic->setFixedSize(20, 20);
	_pShowCurrPic->hide();
	_pShowCurrPic->setMouseTracking(true);

	_pControls = new FullScreenControls(this);//ui.wgtRemoteImage);
	connect(_pControls, SIGNAL(fullScreenState(bool)), this, SLOT(fullScreenStateChange(bool)));
	connect(_pControls, SIGNAL(camStateChange(bool)), this, SLOT(cameraStateChange(bool)));
	
	_pControls->hide();
	
	_pControls->setFixedSize(270, 40);
	//_pControls->setMouseTracking(true);
	_pControls->SetCameraOn(true);
}

FullScreenForm::~FullScreenForm()
{

}

void FullScreenForm::SetCurrImage(const QImage& img)
{
	QPixmap tmpPix = QPixmap::fromImage(img);
	//QRect rect = ui.lblRemotePic->geometry();
	//int h = ui.wgtRemoteImage->size().height();
	//tmpPix = tmpPix.scaledToHeight(h /*- 2*/);

	_pCurrPic->setPixmap(tmpPix);
}


//QSize sizeThis;

void FullScreenForm::SetRemoteImage(const QImage& img)
{
	QPixmap tmpPix = QPixmap::fromImage(img);
	//QRect rect = ui.lblRemotePic->geometry();

		int h = ui.wgtRemoteImage->size().height() - 2;
		int w = ui.wgtRemoteImage->size().width() - 2;
		//tmpPix = tmpPix.scaledToHeight(h - 2, Qt::SmoothTransformation);
		//tmpPix = tmpPix.scaledToHeight(h - 2, Qt::FastTransformation);
		//tmpPix = tmpPix.scaled (w, h, Qt::KeepAspectRatio, Qt::SmoothTransformation );
	//tmpPix = tmpPix.scaled (sizeThis.width() - 2, sizeThis.height() - 2, Qt::KeepAspectRatio, Qt::SmoothTransformation );


	//ui.lblRemotePic->setPixmap(tmpPix);
	ui.wgtRemoteImage->setPicture(tmpPix);
}



void FullScreenForm::resizeEvent(QResizeEvent *r_event)
{
	QWidget::resizeEvent(r_event);

	QPoint pt = ui.wgtRemoteImage->pos();
	QSize size = ui.wgtRemoteImage->size();

	if(_pCurrPic != NULL)
	{
		QSize currPicSize;
		if(size.width() >= 320 || size.height() >= 240)
		{
			currPicSize.setWidth(160);
			currPicSize.setHeight(120);
		}
		else
		{

		}
		_pCurrPic->setFixedSize(currPicSize);
		_pCurrPic->move(4, size.height() - _pCurrPic->rect().height() - 4);
		//_pCurrPic->move(4/*pt.x()*/, /*pt.y()*/ size.height() - 124);
	}

	if(_pShowCurrPic != NULL)
	{
		_pShowCurrPic->move(4, size.height() - _pShowCurrPic->size().height() - 4);
	}


	//QSize currSize = r_event->size();
	//QSize oldSize = r_event->oldSize();



	// // //_pCurrPic->move(0, r_event->size().height() - 120);//(revent->size().height() - 16) / 2);

	QSize controlsSize = _pControls->size();
	_pControls->move(size.width()/2 - controlsSize.width()/2, size.height() - controlsSize.height() - 4);
}

void FullScreenForm::enterEvent(QEvent *e_event)
{
	_pControls->show();
}

void FullScreenForm::leaveEvent(QEvent *)
{
	_pControls->hide();
}

void FullScreenForm::keyPressEvent(QKeyEvent *ev)
{
	int k = ev->key();
	if(k == Qt::Key_Escape)
	{
		_pControls->setFullScreen(false);
	}
}

void FullScreenForm::fullScreenStateChange(bool state)
{
	if(state)
		showFullScreen();
	else
		showNormal();
}

void FullScreenForm::cameraStateChange(bool state)
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