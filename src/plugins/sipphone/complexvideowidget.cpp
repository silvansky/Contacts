#include "complexvideowidget.h"

#include <windows.h>

#include <QPainter>
#include <QPaintEvent>

ComplexVideoWidget::ComplexVideoWidget(QWidget *parent) : QWidget(parent), _noSignal(false), _pPixmap(NULL)
{
	setObjectName("ComplexVideoWidget");

}

ComplexVideoWidget::~ComplexVideoWidget()
{
	mutex.lock();
	if(_pPixmap)
	{
		delete _pPixmap;
		_pPixmap = NULL;
	}
	mutex.unlock();
}

QPixmap* ComplexVideoWidget::picture() const
{
	return _pPixmap;
}

void ComplexVideoWidget::setPicture(const QImage& img)
{
	setPicture(QPixmap::fromImage(img));
}


void ComplexVideoWidget::setPicture(const QPixmap& pxmp)
{
	//if (!d->pixmap || d->pixmap->cacheKey() != pixmap.cacheKey()) {
	//	d->clearContents();
	//	d->pixmap = new QPixmap(pixmap);
	//}

	//if (d->pixmap->depth() == 1 && !d->pixmap->mask())
	//	d->pixmap->setMask(*((QBitmap *)d->pixmap));

	//d->updateLabel();

	mutex.lock();

	if(_noSignal)
		_noSignal = false;

	//static int k=0;
	//k++;
	//if(k > 50)
	//	return;

	if(!_pPixmap || _pPixmap->cacheKey() != pxmp.cacheKey())
	{
		if(_pPixmap != NULL)
		{
			delete _pPixmap;
			_pPixmap = NULL;
		}
		_pPixmap = new QPixmap(pxmp);
		update(rect());
	}

	mutex.unlock();
}

void ComplexVideoWidget::paintEvent(QPaintEvent *ev)
{
	Q_UNUSED(ev);
	//QWidget::paintEvent(ev);

	QPainter painter(this);

	mutex.lock();

	painter.fillRect(/*ev->*/rect(), Qt::black);
	// нет изображения для отображения
	if(_noSignal || _pPixmap == NULL)
	{
		//painter.fillRect(/*ev->*/rect(), Qt::black);
		QRect currRect = /*ev->*/rect();
		QSize currSize = size();
		int x = currRect.x() + currRect.width()/2;
		int y = currRect.y() + currRect.height()/2;
		Q_UNUSED(currSize);
		Q_UNUSED(x);
		Q_UNUSED(y);
		//QRect pixRect(QPoint(x,y), QSize(scaledPixmap.width(), scaledPixmap.height()));
		painter.setPen(Qt::white);
		painter.drawText(currRect, Qt::AlignCenter, tr("Camera OFF"));
	}
	else
	{
		//QWidget::paintEvent(ev);
		QRect currRect = /*ev->*/rect();
		QSize currSize = size();

		//QPixmap scaledPixmap = _pPixmap->scaled(currSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
		//int x = currRect.x() + currRect.width()/2 - scaledPixmap.width()/2;
		//int y = currRect.y() + currRect.height()/2 - scaledPixmap.height()/2;
		//QRect pixRect(QPoint(x,y), QSize(scaledPixmap.width(), scaledPixmap.height()));

		//QPixmap scaledPixmap = *_pPixmap;//->scaled(currSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
		

		QSize newSize(_pPixmap->width(), _pPixmap->height());

		newSize.scale(currSize.width(), currSize.height(), Qt::KeepAspectRatio);


		//int x = currRect.x() + currRect.width()/2 - currSize.height()*aspectRatio/2;
		//int y = currRect.y() + currRect.height()/2 - currSize.height()/2;


		int x = currRect.x() + currRect.width()/2 - newSize.width()/2;
		int y = currRect.y() + currRect.height()/2 - newSize.height()/2;

		QRect pixRect(QPoint(x,y), newSize);

		//painter.drawPixmap(pixRect, scaledPixmap);
		painter.drawPixmap(pixRect, *_pPixmap);
	}

	mutex.unlock();

}

//void ComplexVideoWidget::moveEvent(QMoveEvent *ev)
//{
//	QWidget::moveEvent(ev);
//update();
//updateGeometry();
//}
//
//void ComplexVideoWidget::resizeEvent(QResizeEvent *ev)
//{
//	QWidget::resizeEvent(ev);
//	update();
//	updateGeometry();
//}
