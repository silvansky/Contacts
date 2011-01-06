#ifndef CUSTOMBORDERCONTAINER_H
#define CUSTOMBORDERCONTAINER_H

#include <QWidget>
#include <QLayout>
#include <QDebug>
#include "utilsexport.h"
#include "menu.h"

class CustomBorderContainerPrivate;
struct HeaderButton;

// NOTE: QWidget::isMaximized() will return false even if widget is maximized. I will try to fix it later.

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
signals:
	void minimizeClicked();
	void maximizeClicked();
	void closeClicked();
	void iconClicked();
	void minimized();
	void maximized();
	void closed();
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
	enum HeaderButtonState
	{
		Normal,
		NormalHover,
		Pressed,
		Disabled,
		PressedDisabled
	};
	enum HeaderButtons
	{
		NoneButton,
		MinimizeButton,
		MaximizeButton,
		CloseButton
	};

public:
	enum HeaderButtonsFlag
	{
		MinimizeVisible = 0x01,
		MaximizeVisible = 0x02,
		CloseVisible = 0x04,
		MinimizeEnabled = 0x08,
		MaximizeEnabled = 0x10,
		CloseEnabled = 0x20
	};
	Q_DECLARE_FLAGS(HeaderButtonsFlags, HeaderButtonsFlag)
	HeaderButtonsFlags headerButtonsFlags() const
	{
		return buttonsFlags;
	}
	void setHeaderButtonFlags(HeaderButtonsFlag flags)
	{
		buttonsFlags = HeaderButtonsFlags(flags);
	}
	// minimize button
	bool isMinimizeButtonVisible() const
	{
		return buttonsFlags.testFlag(MinimizeVisible);
	}
	void setMinimizeButtonVisible(bool visible = true)
	{
		if (visible)
			addHeaderButtonFlag(MinimizeVisible);
		else
			removeHeaderButtonFlag(MinimizeVisible);
	}
	bool isMinimizeButtonEnabled() const
	{
		return buttonsFlags.testFlag(MinimizeEnabled);
	}
	void setMinimizeButtonEnabled(bool enabled = true)
	{
		if (enabled)
			addHeaderButtonFlag(MinimizeEnabled);
		else
			removeHeaderButtonFlag(MinimizeEnabled);
	}
	// maximize button
	bool isMaximizeButtonVisible() const
	{
		return buttonsFlags.testFlag(MaximizeVisible);
	}
	void setMaximizeButtonVisible(bool visible = true)
	{
		if (visible)
			addHeaderButtonFlag(MaximizeVisible);
		else
			removeHeaderButtonFlag(MaximizeVisible);
	}
	bool isMaximizeButtonEnabled() const
	{
		return buttonsFlags.testFlag(MaximizeEnabled);
	}
	void setMaximizeButtonEnabled(bool enabled = true)
	{
		if (enabled)
			addHeaderButtonFlag(MaximizeEnabled);
		else
			removeHeaderButtonFlag(MaximizeEnabled);
	}
	// close button
	bool isCloseButtonVisible() const
	{
		return buttonsFlags.testFlag(CloseVisible);
	}
	void setCloseButtonVisible(bool visible = true)
	{
		if (visible)
			addHeaderButtonFlag(CloseVisible);
		else
			removeHeaderButtonFlag(CloseVisible);
	}
	bool isCloseButtonEnabled() const
	{
		return buttonsFlags.testFlag(CloseEnabled);
	}
	void setCloseButtonEnabled(bool enabled = true)
	{
		if (enabled)
			addHeaderButtonFlag(CloseEnabled);
		else
			removeHeaderButtonFlag(CloseEnabled);
	}
protected:
	// header button flags manipulations
	void addHeaderButtonFlag(HeaderButtonsFlag flag)
	{
		buttonsFlags |= flag;
	}
	void removeHeaderButtonFlag(HeaderButtonsFlag flag)
	{
		if (buttonsFlags.testFlag(flag))
			buttonsFlags ^= flag;
	}
	// header buttons under mouse
	int headerButtonsCount() const;
	QRect headerButtonRect(HeaderButtons button) const;
	bool minimizeButtonUnderMouse() const;
	bool maximizeButtonUnderMouse() const;
	bool closeButtonUnderMouse() const;
	HeaderButtons headerButtonUnderMouse() const;
	QRect headerButtonsRect() const;
	void repaintHeaderButtons();
	QRect windowIconRect() const;
	// etc...
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
	void mouseRelease(const QPoint & p, QWidget * widget, Qt::MouseButton button = Qt::LeftButton);
	void mouseDoubleClick(const QPoint & p, QWidget * widget);
	bool pointInBorder(BorderType border, const QPoint & p);
	bool pointInHeader(const QPoint & p);
	void checkResizeCondition(const QPoint & p);
	void checkMoveCondition(const QPoint & p);
	void updateCursor(QWidget * widget = 0);
	void updateShape();
	void updateIcons();
	void setLayoutMargins();
	QRect headerRect() const;
	QRect headerMoveRect() const;
	void drawHeader(QPainter * p);
	void drawButton(HeaderButton & button, QPainter * p, HeaderButtonState state = Normal);
	void drawButtons(QPainter * p);
	void drawIcon(QPainter * p);
	void drawTitle(QPainter * p);
	void drawBorders(QPainter * p);
	void drawCorners(QPainter * p);
	QPoint mapFromWidget(QWidget * widget, const QPoint &point);
	QImage loadImage(const QString & key);
	QIcon loadIcon(const QString & key);
protected slots:
	void minimizeWidget();
	void maximizeWidget();
	void closeWidget();
private:
	// widgets/layouts
	QWidget * containedWidget;
	QLayout * containerLayout;
	QWidget * headerWidget;
	GeometryState currentGeometryState;
	QRect oldGeometry;
	QPoint oldPressPoint;
	QPoint lastMousePosition;
	CustomBorderContainerPrivate * myPrivate;
	BorderType resizeBorder;
	bool canMove;
	HeaderButtonsFlags buttonsFlags;
	HeaderButtons pressedHeaderButton;
	bool isMaximized;
	QRect normalGeometry;
	Menu * windowMenu;
	Action * minimizeAction;
	Action * maximizeAction;
	Action * closeAction;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(CustomBorderContainer::HeaderButtonsFlags)

#endif // CUSTOMBORDERCONTAINER_H
