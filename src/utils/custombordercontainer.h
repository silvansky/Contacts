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
	~CustomBorderContainer();
	QWidget * widget() const
	{
		return containedWidget;
	}
	void setWidget(QWidget * widget);
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
	void mouseMove(const QPoint & p);
	bool pointInBorder(BorderType border, const QPoint & p);
	void checkResizeCondition(const QPoint & p);
	void checkMoveCondition(const QPoint & p);
	void updateCursor();
	void updateShape();
	void setLayoutMargins();
	void drawHeader(QPainter * p);
	void drawBorders(QPainter * p);
	void drawCorners(QPainter * p);
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
