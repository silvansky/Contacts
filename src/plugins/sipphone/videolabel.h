#ifndef VIDEOLABEL_H
#define VIDEOLABEL_H

#include <QLabel>
#include <QMovie>

class VideoLabel : 
	public QLabel
{
	Q_OBJECT;
public:
	VideoLabel(QWidget *AParent = NULL);
	~VideoLabel();
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
signals:
	void moveTo(const QPoint &APos);
	void resizeTo(Qt::Corner ACorner, const QPoint &APos);
public:
	QSize sizeHint() const;
	QSize minimumSizeHint() const;
public slots:
	void setPixmap(const QPixmap &APixmap);
protected:
	void enterEvent(QEvent *AEvent);
	void leaveEvent(QEvent *AEvent);
	void mouseMoveEvent(QMouseEvent *AEvent);
	void mousePressEvent(QMouseEvent *AEvent);
	void mouseReleaseEvent(QMouseEvent *AEvent);
	void paintEvent(QPaintEvent *AEvent);
protected slots:
	void onWaitMovieFrameChanged(int AFrameNumber);
private:
	QMovie *FWaitMovie;
	bool FMoveEnabled;
	bool FResizeEnabled;
	QPixmap FResizeIcon;
	QPoint FPressedPos;
	int FCursorCorner;
	QSize FMinimumSize;
	QSize FMaximumSize;
	Qt::Alignment FAlignment;
};

#endif // VIDEOLABEL_H
