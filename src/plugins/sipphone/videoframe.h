#ifndef VIDEOLABEL_H
#define VIDEOLABEL_H

#include <QLabel>
#include <QMovie>
#include <QDateTime>
#include <interfaces/isipphone.h>

class VideoFrame : 
	public QFrame
{
	Q_OBJECT;
public:
	VideoFrame(QWidget *AParent = NULL);
	~VideoFrame();
	bool isEmpty() const;
	bool isCollapsed() const;
	void setCollapsed(bool ACollapsed);
	bool isMoveEnabled() const;
	void setMoveEnabled(bool AEnabled);
	bool isResizeEnabled() const;
	void setResizeEnabled(bool AEnabled);
	Qt::Alignment alignment() const;
	void setAlignment(Qt::Alignment AAlign);
	QSize minimumVideoSize() const;
	void setMinimumVideoSize(const QSize &ASize);
	QSize maximumVideoSize() const;
	void setMaximumVideoSize(const QSize &ASize);
	const QPixmap *pixmap() const;
	void setPixmap(const QPixmap &APixmap);
	int videoDeviceState() const;
	void setVideoDeviceState(int AState);
signals:
	void singleClicked();
	void doubleClicked();
	void moveTo(const QPoint &APos);
	void resizeTo(Qt::Corner ACorner, const QPoint &APos);
public:
	QSize sizeHint() const;
	QSize minimumSizeHint() const;
protected:
	void enterEvent(QEvent *AEvent);
	void leaveEvent(QEvent *AEvent);
	void mouseMoveEvent(QMouseEvent *AEvent);
	void mousePressEvent(QMouseEvent *AEvent);
	void mouseReleaseEvent(QMouseEvent *AEvent);
	void mouseDoubleClickEvent(QMouseEvent *AEvent);
	void paintEvent(QPaintEvent *AEvent);
protected slots:
	void onEmitSingleClicked();
	void onWaitMovieFrameChanged(int AFrameNumber);
private:
	bool FMoved;
	bool FClicked;
	bool FCollapsed;
	bool FMoveEnabled;
	bool FResizeEnabled;
	int FDeviceState;
	int FCursorCorner;
	QPoint FPressedPos;
	QPoint FGlobalPressed;
	QSize FMinimumSize;
	QSize FMaximumSize;
	QMovie *FWaitMovie;
	QPixmap FVideoFrame;
	QPixmap FResizeIcon;
	QPixmap FCollapsedIcon;
	QPixmap FCameraDisabledIcon;
	Qt::Alignment FAlignment;
	QDateTime FDoubleClickTime;
};

#endif // VIDEOLABEL_H
