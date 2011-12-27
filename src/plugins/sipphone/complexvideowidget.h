#ifndef COMPLEXVIDEOWIDGET_H
#define COMPLEXVIDEOWIDGET_H

#include <QWidget>
#include <QPixmap>
#include <QMutex>
#include "qimagelabel.h"

class ComplexVideoWidget : public QWidget
{
	Q_OBJECT

public:
	ComplexVideoWidget(QWidget *parent);
	~ComplexVideoWidget();

public:
	QPixmap* picture() const;

public slots:
	void setPicture(const QImage&);
	void setPicture(const QPixmap&);

protected:
	// Event handlers
	//virtual void mousePressEvent(QMouseEvent *);
	//virtual void mouseReleaseEvent(QMouseEvent *);
	//virtual void mouseDoubleClickEvent(QMouseEvent *);
	//virtual void mouseMoveEvent(QMouseEvent *);
	//virtual void keyPressEvent(QKeyEvent *);
	//virtual void keyReleaseEvent(QKeyEvent *);
	//virtual void focusInEvent(QFocusEvent *);
	//virtual void focusOutEvent(QFocusEvent *);
	//virtual void enterEvent(QEvent *);
	//virtual void leaveEvent(QEvent *);
	virtual void paintEvent(QPaintEvent *);
	//virtual void moveEvent(QMoveEvent *);
	//virtual void resizeEvent(QResizeEvent *);
	//virtual void closeEvent(QCloseEvent *);

private:
	//QImageLabel* _pCurrPic;
	bool _noSignal;
	QPixmap* _pPixmap;

private:
	QMutex mutex;

};

#endif // COMPLEXVIDEOWIDGET_H
