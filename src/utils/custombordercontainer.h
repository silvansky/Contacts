#ifndef CUSTOMBORDERCONTAINER_H
#define CUSTOMBORDERCONTAINER_H

#include <QWidget>
#include <QLayout>
#include <QDebug>
#include "utilsexport.h"

class CustomBorderContainerPrivate;

class UTILS_EXPORT CustomBorderContainer : public QWidget
{
	Q_OBJECT
	friend class CustomBorderContainerPrivate;

public:
	explicit CustomBorderContainer(QWidget * widgetToContain = 0);
	CustomBorderContainer(const CustomBorderContainerPrivate &style);
	~CustomBorderContainer();
	QWidget * widget() const
	{
		return containedWidget;
	}
	// WARNING! the old widget will be deleted! use releaseWidget() to just unset widget
	void setWidget(QWidget * widget);
	QWidget * releaseWidget();
	void loadFile(const QString & fileName);
protected:
	// event handlers
	void changeEvent(QEvent *e);
	void resizeEvent(QResizeEvent *);
	void mousePressEvent(QMouseEvent *);
	void mouseMoveEvent(QMouseEvent *);
	void mouseReleaseEvent(QMouseEvent *);
	void mouseDoubleClickEvent(QMouseEvent *);
	void moveEvent(QMoveEvent *);
	void paintEvent(QPaintEvent *);
	void enterEvent(QEvent *);
	void leaveEvent(QEvent *);
	void focusInEvent(QFocusEvent *);
	void focusOutEvent(QFocusEvent *);
	void contextMenuEvent(QContextMenuEvent *);
	// event filter
	bool event(QEvent *);
	bool eventFilter(QObject *, QEvent *);
protected:
	// common initialization
	void init();
	enum GeometryState
	{
		None,
		Resizing,
		Moving
	};
	GeometryState geometryState() const
	{
		return currentGeometryState;
	}
	void setGeometryState(GeometryState newGeometryState)
	{
		currentGeometryState = newGeometryState;
	}
	void updateGeometry(const QPoint & p);
	enum BorderType
	{
		NoneBorder = 0,
		TopLeftCorner,
		TopRightCorner,
		BottomLeftCorner,
		BottomRightCorner,
		LeftBorder,
		RightBorder,
		TopBorder,
		BottomBorder
	};
	void mouseMove(const QPoint & p, QWidget * widget);
	void mousePress(const QPoint & p, QWidget * widget);
	void mouseRelease(const QPoint & p, QWidget * widget);
	void mouseDoubleClick(const QPoint & p, QWidget * widget);
	bool pointInBorder(BorderType border, const QPoint & p);
	void checkResizeCondition(const QPoint & p);
	void checkMoveCondition(const QPoint & p);
	void updateCursor(QWidget * widget = 0);
	void updateShape();
	void setLayoutMargins();
	void drawHeader(QPainter * p);
	void drawIcon(QPainter * p);
	void drawTitle(QPainter * p);
	void drawBorders(QPainter * p);
	void drawCorners(QPainter * p);
	QPoint mapFromWidget(QWidget * widget, const QPoint &point);
private:
	// widgets/layouts
	QWidget * containedWidget;
	QLayout * containerLayout;
	QWidget * headerWidget;
	GeometryState currentGeometryState;
	QRect oldGeometry;
	QPoint oldPressPoint;
	CustomBorderContainerPrivate * myPrivate;
	BorderType resizeBorder;
	bool canMove;
};

#endif // CUSTOMBORDERCONTAINER_H
