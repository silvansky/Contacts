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
public:
	QSize sizeHint() const;
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
	Qt::Alignment FAlignment;
};

#endif // VIDEOLABEL_H
