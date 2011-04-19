#include "custombordercontainer.h"
#include "log.h"
#include <QEvent>
#include <QMouseEvent>
#include <QVBoxLayout>
#include "custombordercontainer_p.h"
#ifdef DEBUG_ENABLED
# include <QDebug>
#endif
#include <QLinearGradient>
#include <QGradientStop>
#include <QPainter>
#include <QApplication>
#include <QTextDocument>
#include <QTextCursor>
#include <QDesktopWidget>
#include <QStyle>
#include <QBitmap>
#include <QChildEvent>
#include <QPushButton>
#include <QToolButton>
#include <QLineEdit>
#include <QTextEdit>
#include <QSplitter>
#include <QAbstractItemView>
#include <QTreeView>
#include <QComboBox>
#include <QScrollBar>
// damn, i didn't want that!
#include <QWebView>
#include "iconstorage.h"
#ifdef Q_WS_WIN
# include <qt_windows.h>
#endif

// internal functions

static void childsRecursive(QObject *object, QWidget *watcher, bool install)
{
	// ensure object is widget but not a menu
	if (object->isWidgetType() && !qobject_cast<QMenu*>(object) && !qobject_cast<Menu*>(object))
	{
		QWidget * widget = qobject_cast<QWidget*>(object);
		if (widget->parent() && widget->isWindow())
			return;
		if (install)
			object->installEventFilter(watcher);
		else
			object->removeEventFilter(watcher);
		// TODO: return params back
		widget->setAutoFillBackground(true);
		widget->setMouseTracking(true);
		widget->setProperty("defaultCursorShape", widget->cursor().shape());
	}
	QObjectList children = object->children();
	foreach(QObject *child, children)
	{
		childsRecursive(child, watcher, install);
	}
}

static void repaintRecursive(QWidget *widget, const QRect & globalRect)
{
	if (widget && widget->isVisible())
	{
		QPoint topleft = widget->mapFromGlobal(globalRect.topLeft());
		QRect newRect = globalRect;
		newRect.moveTopLeft(topleft);
		widget->repaint(newRect);
		QObjectList children = widget->children();
		foreach(QObject *child, children)
		{
			repaintRecursive(qobject_cast<QWidget*>(child), globalRect);
		}
	}
}

/**************************************
 * class CustomBorderContainerPrivate *
 **************************************/

CustomBorderContainerPrivate::CustomBorderContainerPrivate(CustomBorderContainer *parent)
{
	p = parent;
	setAllDefaults();
}

CustomBorderContainerPrivate::CustomBorderContainerPrivate(const CustomBorderContainerPrivate& other) :
	topLeft(other.topLeft),
	topRight(other.topRight),
	bottomLeft(other.bottomLeft),
	bottomRight(other.bottomRight),
	left(other.left),
	right(other.right),
	top(other.top),
	bottom(other.bottom),
	header(other.header),
	title(other.title),
	icon(other.icon),
	controls(other.controls),
	minimize(other.minimize),
	maximize(other.maximize),
	close(other.close),
	restore(other.restore),
	headerButtons(other.headerButtons),
	dragAnywhere(other.dragAnywhere),
	p(NULL)
{
}

CustomBorderContainerPrivate::~CustomBorderContainerPrivate()
{
	delete header.gradient;
	delete header.gradientInactive;
	delete left.gradient;
	delete right.gradient;
	delete top.gradient;
	delete bottom.gradient;
	delete topLeft.gradient;
	delete topRight.gradient;
	delete bottomLeft.gradient;
	delete bottomRight.gradient;
	delete minimize.gradientDisabled;
	delete minimize.gradientHover;
	delete minimize.gradientHoverDisabled;
	delete minimize.gradientNormal;
	delete minimize.gradientPressed;
	delete minimize.gradientPressedDisabled;
	delete maximize.gradientDisabled;
	delete maximize.gradientHover;
	delete maximize.gradientHoverDisabled;
	delete maximize.gradientNormal;
	delete maximize.gradientPressed;
	delete maximize.gradientPressedDisabled;
	delete close.gradientDisabled;
	delete close.gradientHover;
	delete close.gradientHoverDisabled;
	delete close.gradientNormal;
	delete close.gradientPressed;
	delete close.gradientPressedDisabled;
}

void CustomBorderContainerPrivate::parseFile(const QString &fileName)
{
	QFile file(fileName);
	if (file.open(QFile::ReadOnly))
	{
		QString s = QString::fromUtf8(file.readAll());
		QDomDocument doc;
		if (doc.setContent(s))
		{
			// parsing our document
			QDomElement root = doc.firstChildElement("window-border-style");
			if (!root.isNull())
			{
				// borders
				QDomElement leftBorderEl = root.firstChildElement("left-border");
				parseBorder(leftBorderEl, left);
				QDomElement rightBorderEl = root.firstChildElement("right-border");
				parseBorder(rightBorderEl, right);
				QDomElement topBorderEl = root.firstChildElement("top-border");
				parseBorder(topBorderEl, top);
				QDomElement bottomBorderEl = root.firstChildElement("bottom-border");
				parseBorder(bottomBorderEl, bottom);
				// corners
				QDomElement topLeftCornerEl = root.firstChildElement("top-left-corner");
				parseCorner(topLeftCornerEl, topLeft);
				QDomElement topRightCornerEl = root.firstChildElement("top-right-corner");
				parseCorner(topRightCornerEl, topRight);
				QDomElement bottomLeftCornerEl = root.firstChildElement("bottom-left-corner");
				parseCorner(bottomLeftCornerEl, bottomLeft);
				QDomElement bottomRightCornerEl = root.firstChildElement("bottom-right-corner");
				parseCorner(bottomRightCornerEl, bottomRight);
				// header
				QDomElement headerEl = root.firstChildElement("header");
				parseHeader(headerEl, header);
				// icon
				QDomElement iconEl = root.firstChildElement("window-icon");
				parseWindowIcon(iconEl, icon);
				// title
				QDomElement titleEl = root.firstChildElement("title");
				parseHeaderTitle(titleEl, title);
				// window controls
				QDomElement windowControlsEl = root.firstChildElement("window-controls");
				parseWindowControls(windowControlsEl, controls);
				// minimize button
				QDomElement button = root.firstChildElement("minimize-button");
				parseHeaderButton(button, minimize);
				// maximize button
				button = root.firstChildElement("maximize-button");
				parseHeaderButton(button, maximize);
				// close button
				button = root.firstChildElement("close-button");
				parseHeaderButton(button, close);
				// restore button
				button = root.firstChildElement("restore-button");
				parseHeaderButton(button, restore);
				// drag anywhere
				QDomElement drag = root.firstChildElement("drag-anywhere");
				if (!drag.isNull())
				{
					dragAnywhere = (drag.attribute("enabled").compare("true", Qt::CaseInsensitive) == 0);
				}

			}
			else
			{
				Log(QString("Can\'t parse file %1! Unknown root element.").arg(fileName));
			}
		}
		else
		{
			Log(QString("Can\'t parse file %1!").arg(fileName));
		}
	}
	else
	{
		Log(QString("Can\'t open file %1!").arg(fileName));
	}
}

void CustomBorderContainerPrivate::setAllDefaults()
{
	dragAnywhere = false;
	setDefaultBorder(left);
	setDefaultBorder(right);
	setDefaultBorder(top);
	setDefaultBorder(bottom);
	setDefaultCorner(topLeft);
	setDefaultCorner(topRight);
	setDefaultCorner(bottomLeft);
	setDefaultCorner(bottomRight);
	setDefaultHeader(header);
	setDefaultHeaderTitle(title);
	setDefaultWindowIcon(icon);
	setDefaultWindowControls(controls);
	setDefaultHeaderButton(minimize);
	setDefaultHeaderButton(maximize);
	setDefaultHeaderButton(close);
	setDefaultHeaderButton(restore);
}

QColor CustomBorderContainerPrivate::parseColor(const QString & name)
{
	QColor color;
	if (QColor::isValidColor(name))
		color.setNamedColor(name);
	else
	{
		// trying to parse "#RRGGBBAA" color
		if (name.length() == 9)
		{
			QString solidColor = name.left(7);
			if (QColor::isValidColor(solidColor))
			{
				color.setNamedColor(solidColor);
				int alpha = name.right(2).toInt(0, 16);
				color.setAlpha(alpha);
			}
		}
	}
	if (!color.isValid())
		Log(QString("Can\'t parse color: %1").arg(name));
	return color;
}

// HINT: only linear gradients are supported for now
QGradient * CustomBorderContainerPrivate::parseGradient(const QDomElement & element)
{
	if (!element.isNull())
	{
		QString type = element.attribute("type");
		if (type == "linear" || type.isEmpty())
		{
			QLinearGradient * gradient = new QLinearGradient;
			// detecting single color
			if (!element.attribute("color").isEmpty())
			{
				gradient = new QLinearGradient(0.0, 0.0, 1.0, 0.0);
				gradient->setColorAt(0.0, parseColor(element.attribute("color")));
				gradient->setSpread(QGradient::RepeatSpread);
				return gradient;
			}
			QDomElement direction = element.firstChildElement("direction");
			double x1, x2, y1, y2;
			if (!direction.isNull())
			{
				x1 = direction.attribute("x1").toFloat();
				x2 = direction.attribute("x2").toFloat();
				y1 = direction.attribute("y1").toFloat();
				y2 = direction.attribute("y2").toFloat();
			}
			else
			{
				x1 = y1 = y2 = 0.0;
				x2 = 1.0;
			}
			gradient->setStart(x1, y1);
			gradient->setFinalStop(x2, y2);
			QDomElement gradientStop = element.firstChildElement("gradient-stop");
			while (!gradientStop.isNull())
			{
				gradient->setColorAt(gradientStop.attribute("at").toFloat(), parseColor(gradientStop.attribute("color")));
				gradientStop = gradientStop.nextSiblingElement("gradient-stop");
			}
			gradient->setSpread(QGradient::ReflectSpread);
			return gradient;
		}
	}
	QLinearGradient * lg = new QLinearGradient(0.0, 0.0, 1.0, 0.0);
	lg->setColorAt(0.0, QColor::fromRgb(0, 0, 0));
	return lg;
}

ImageFillingStyle CustomBorderContainerPrivate::parseImageFillingStyle(const QString & style)
{
	ImageFillingStyle s = Stretch;
	if (style == "keep")
		s = Keep;
	else if (style == "tile-horizontally")
		s = TileHor;
	else if (style == "tile-vertically")
		s = TileVer;
	else if (style == "tile")
		s = Tile;
	return s;
}

void CustomBorderContainerPrivate::setDefaultBorder(Border & border)
{
	// 1 px solid black
	border.width = 1;
	border.resizeWidth = 5;
	border.resizeMargin = 0;
	border.image = QString::null;
	border.imageFillingStyle = Stretch;
	border.gradient = NULL;
}

void CustomBorderContainerPrivate::parseBorder(const QDomElement & borderElement, Border & border)
{
	if (!borderElement.isNull())
	{
		QDomElement width = borderElement.firstChildElement("width");
		if (!width.isNull())
		{
			border.width = width.text().toInt();
		}
		QDomElement resizeWidth = borderElement.firstChildElement("resize-width");
		if (!resizeWidth.isNull())
		{
			border.resizeWidth = resizeWidth.text().toInt();
		}
		QDomElement resizeMargin = borderElement.firstChildElement("resize-margin");
		if (!resizeMargin.isNull())
		{
			border.resizeMargin = resizeMargin.text().toInt();
		}
		QDomElement gradient = borderElement.firstChildElement("gradient");
		if (!gradient.isNull())
		{
			border.gradient = parseGradient(gradient);
		}
		QDomElement image = borderElement.firstChildElement("image");
		if (!image.isNull())
		{
			border.image = image.attribute("src");
			border.imageFillingStyle = parseImageFillingStyle(image.attribute("image-filling-style"));
		}
	}
}

void CustomBorderContainerPrivate::setDefaultCorner(Corner & corner)
{
	corner.width = 10;
	corner.height = 10;
	corner.gradient = NULL;
	corner.image = corner.mask = QString::null;
	corner.imageFillingStyle = Stretch;
	corner.radius = 10;
	corner.resizeLeft = corner.resizeRight = corner.resizeTop = corner.resizeBottom = 0;
	corner.resizeWidth = corner.resizeHeight = 10;
}

void CustomBorderContainerPrivate::parseCorner(const QDomElement & cornerElement, Corner & corner)
{
	if (!cornerElement.isNull())
	{
		QDomElement width = cornerElement.firstChildElement("width");
		if (!width.isNull())
		{
			corner.width = width.text().toInt();
		}
		QDomElement height = cornerElement.firstChildElement("height");
		if (!height.isNull())
		{
			corner.height = height.text().toInt();
		}
		QDomElement gradient = cornerElement.firstChildElement("gradient");
		if (!gradient.isNull())
		{
			corner.gradient = parseGradient(gradient);
		}
		QDomElement image = cornerElement.firstChildElement("image");
		if (!image.isNull())
		{
			corner.image = image.attribute("src");
			corner.imageFillingStyle = parseImageFillingStyle(image.attribute("image-filling-style"));
		}
		QDomElement radius = cornerElement.firstChildElement("radius");
		if (!radius.isNull())
		{
			corner.radius = radius.text().toInt();
		}
		QDomElement mask = cornerElement.firstChildElement("mask");
		if (!mask.isNull())
		{
			corner.mask = mask.attribute("src");
		}
		QDomElement resizeLeft = cornerElement.firstChildElement("resize-left");
		if (!resizeLeft.isNull())
		{
			corner.resizeLeft = resizeLeft.text().toInt();
		}
		QDomElement resizeRight = cornerElement.firstChildElement("resize-right");
		if (!resizeRight.isNull())
		{
			corner.resizeRight = resizeRight.text().toInt();
		}
		QDomElement resizeTop = cornerElement.firstChildElement("resize-top");
		if (!resizeTop.isNull())
		{
			corner.resizeTop = resizeTop.text().toInt();
		}
		QDomElement resizeBottom = cornerElement.firstChildElement("resize-bottom");
		if (!resizeBottom.isNull())
		{
			corner.resizeBottom = resizeBottom.text().toInt();
		}
		QDomElement resizeWidth = cornerElement.firstChildElement("resize-width");
		if (!resizeWidth.isNull())
		{
			corner.resizeWidth = resizeWidth.text().toInt();
		}
		QDomElement resizeHeight = cornerElement.firstChildElement("resize-height");
		if (!resizeHeight.isNull())
		{
			corner.resizeHeight = resizeHeight.text().toInt();
		}
	}
}

void CustomBorderContainerPrivate::setDefaultHeader(Header & header)
{
	header.height = 26;
	header.margins = QMargins(2, 2, 2, 2);
	header.gradient = NULL;
	header.gradientInactive = NULL;
	header.image = QString::null;
	header.imageInactive = QString::null;
	header.imageFillingStyle = Stretch;
	header.spacing = 5;
	header.moveHeight = header.height;
	header.moveLeft = header.moveRight = header.moveTop = 0;
}

void CustomBorderContainerPrivate::parseHeader(const QDomElement & headerElement, Header & header)
{
	if (!headerElement.isNull())
	{
		QDomElement height = headerElement.firstChildElement("height");
		if (!height.isNull())
		{
			header.height = height.text().toInt();
		}
		QDomElement moveHeight = headerElement.firstChildElement("move-height");
		if (!moveHeight.isNull())
		{
			header.moveHeight = moveHeight.text().toInt();
		}
		QDomElement moveLeft = headerElement.firstChildElement("move-left");
		if (!moveLeft.isNull())
		{
			header.moveLeft = moveLeft.text().toInt();
		}
		QDomElement moveRight = headerElement.firstChildElement("move-right");
		if (!moveRight.isNull())
		{
			header.moveRight = moveRight.text().toInt();
		}
		QDomElement moveTop= headerElement.firstChildElement("move-top");
		if (!moveTop.isNull())
		{
			header.moveTop = moveTop.text().toInt();
		}
		QDomElement spacing = headerElement.firstChildElement("spacing");
		if (!spacing.isNull())
		{
			header.spacing = spacing.text().toInt();
		}
		QDomElement gradient = headerElement.firstChildElement("gradient");
		if (!gradient.isNull())
		{
			header.gradient = parseGradient(gradient);
		}
		gradient = headerElement.firstChildElement("gradient-inactive");
		if (!gradient.isNull())
		{
			header.gradientInactive = parseGradient(gradient);
		}
		QDomElement margins = headerElement.firstChildElement("margins");
		if (!margins.isNull())
		{
			header.margins.setLeft(margins.attribute("left").toInt());
			header.margins.setRight(margins.attribute("right").toInt());
			header.margins.setTop(margins.attribute("top").toInt());
			header.margins.setBottom(margins.attribute("bottom").toInt());
		}
	}
}

void CustomBorderContainerPrivate::setDefaultHeaderTitle(HeaderTitle & title)
{
	title.color = QColor(255, 255, 255);
	title.text = "";
}

void CustomBorderContainerPrivate::parseHeaderTitle(const QDomElement & titleElement, HeaderTitle & title)
{
	if (!titleElement.isNull())
	{
		title.color = QColor(titleElement.attribute("color"));
		title.text= titleElement.attribute("text");
	}
}

void CustomBorderContainerPrivate::setDefaultWindowIcon(WindowIcon & windowIcon)
{
	windowIcon.height = 16;
	windowIcon.width = 16;
	windowIcon.icon = QString::null;
}

void CustomBorderContainerPrivate::parseWindowIcon(const QDomElement & iconElement, WindowIcon & windowIcon)
{
	if (!iconElement.isNull())
	{
		QDomElement width = iconElement.firstChildElement("width");
		if (!width.isNull())
		{
			windowIcon.width = width.text().toInt();
		}
		QDomElement height = iconElement.firstChildElement("height");
		if (!height.isNull())
		{
			windowIcon.height = height.text().toInt();
		}
		QDomElement icon = iconElement.firstChildElement("icon");
		if (!icon.isNull())
		{
			windowIcon.icon = icon.attribute("src");
		}
	}
}

void CustomBorderContainerPrivate::setDefaultWindowControls(WindowControls & windowControls)
{
	windowControls.spacing = 2;
}

void CustomBorderContainerPrivate::parseWindowControls(const QDomElement & controlsElement, WindowControls & windowControls)
{
	if (!controlsElement.isNull())
	{
		windowControls.spacing = controlsElement.attribute("spacing").toInt();
	}
}

void CustomBorderContainerPrivate::setDefaultHeaderButton(HeaderButton & button)
{
	button.width = button.height = 16;
	button.borderColor = QColor(0, 0, 0);
	button.borderRadius = 0;
	button.borderWidth = 1;
	button.borderImage = QString::null;
	button.gradientNormal = NULL;
	button.gradientHover = NULL;
	button.gradientPressed = NULL;
	button.gradientDisabled = NULL;
	button.gradientHoverDisabled = NULL;
	button.gradientPressedDisabled = NULL;
	button.imageNormal = button.imageHover = button.imagePressed = button.imageDisabled = button.imageHoverDisabled = button.imagePressedDisabled = QString::null;
}

void CustomBorderContainerPrivate::parseHeaderButton(const QDomElement & buttonElement, HeaderButton & button)
{
	if (!buttonElement.isNull())
	{
		QDomElement width = buttonElement.firstChildElement("width");
		if (!width.isNull())
		{
			button.width = width.text().toInt();
		}
		QDomElement height = buttonElement.firstChildElement("height");
		if (!height.isNull())
		{
			button.height = height.text().toInt();
		}
		QDomElement border = buttonElement.firstChildElement("border");
		if (!border.isNull())
		{
			button.borderWidth = border.attribute("width").toInt();
			button.borderColor = parseColor(border.attribute("color"));
			button.borderRadius = border.attribute("radius").toInt();
			button.borderImage = border.attribute("image");
		}
		QDomElement gradient = buttonElement.firstChildElement("gradient-normal");
		button.gradientNormal = parseGradient(gradient);
		gradient = buttonElement.firstChildElement("gradient-hover");
		button.gradientHover = parseGradient(gradient);
		gradient = buttonElement.firstChildElement("gradient-pressed");
		button.gradientPressed = parseGradient(gradient);
		gradient = buttonElement.firstChildElement("gradient-disabled");
		button.gradientDisabled = parseGradient(gradient);
		gradient = buttonElement.firstChildElement("gradient-hover-disabled");
		button.gradientHoverDisabled = parseGradient(gradient);
		gradient = buttonElement.firstChildElement("gradient-pressed-disabled");
		button.gradientPressedDisabled = parseGradient(gradient);
		QDomElement image = buttonElement.firstChildElement("image-normal");
		if (!image.isNull())
		{
			button.imageNormal = image.attribute("src");
		}
		image = buttonElement.firstChildElement("image-hover");
		if (!image.isNull())
		{
			button.imageHover = image.attribute("src");
		}
		image = buttonElement.firstChildElement("image-pressed");
		if (!image.isNull())
		{
			button.imagePressed = image.attribute("src");
		}
		image = buttonElement.firstChildElement("image-disabled");
		if (!image.isNull())
		{
			button.imageDisabled = image.attribute("src");
		}
		image = buttonElement.firstChildElement("image-hover-disabled");
		if (!image.isNull())
		{
			button.imageHoverDisabled = image.attribute("src");
		}
		image = buttonElement.firstChildElement("image-pressed-disabled");
		if (!image.isNull())
		{
			button.imagePressedDisabled = image.attribute("src");
		}
	}
}

/*******************************
 * class CustomBorderContainer *
 *******************************/

CustomBorderContainer::CustomBorderContainer(QWidget * widgetToContain) :
	QWidget(NULL)
{
	init();
	borderStyle = new CustomBorderContainerPrivate(this);
	setWidget(widgetToContain);
}

CustomBorderContainer::CustomBorderContainer(const CustomBorderContainerPrivate &style) :
	QWidget(NULL)
{
	init();
	setWidget(NULL);
	borderStyle = new CustomBorderContainerPrivate(style);
	borderStyle->p = this;
	updateIcons();
	setLayoutMargins();
}

CustomBorderContainer::~CustomBorderContainer()
{
	delete borderStyle;
	setWidget(NULL);
}

QWidget * CustomBorderContainer::widget() const
{
	return containedWidget;
}

void CustomBorderContainer::setWidget(QWidget * widget)
{
	if (containedWidget)
	{
		releaseWidget()->deleteLater();
	}
	if (widget)
	{
		if (!qobject_cast<Menu*>(widget))
			initMenu();
		containedWidget = widget;
		setAttribute(Qt::WA_WindowPropagation, false);
		containedWidget->setAttribute(Qt::WA_DeleteOnClose, false);
		containedWidget->setAutoFillBackground(true);
		containedWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
		containerLayout->addWidget(containedWidget);
		childsRecursive(containedWidget,this,true);
		containedWidget->setMouseTracking(true);
		containedWidget->setAttribute(Qt::WA_WindowPropagation, false);
		setMinimumSize(containedWidget->minimumSize());
		setWindowTitle(containedWidget->windowTitle());
		connect(containedWidget, SIGNAL(destroyed(QObject*)), SLOT(onContainedWidgetDestroyed(QObject*)));
		containedWidget->setVisible(true);
		installEventFilter(containedWidget);
		adjustSize();
	}
}

QWidget * CustomBorderContainer::releaseWidget()
{
	if (containedWidget)
	{
		removeEventFilter(containedWidget);
		childsRecursive(containedWidget,this,false);
		containedWidget->removeEventFilter(this);
		disconnect(containedWidget, SIGNAL(destroyed(QObject*)), this, SLOT(onContainedWidgetDestroyed(QObject*)));
		containerLayout->removeWidget(containedWidget);
		QWidget * w = containedWidget;
		containedWidget = NULL;
		return w;
	}
	else
		return NULL;
}

void CustomBorderContainer::loadFile(const QString & fileName)
{
	borderStyle->parseFile(fileName);
	updateIcons();
	repaint();
	setLayoutMargins();
	updateShape();
}

bool CustomBorderContainer::isMovable() const
{
	return movable;
}

void CustomBorderContainer::setMovable(bool movable)
{
	this->movable = movable;
	if (!movable)
	{
		canMove = false;
		setGeometryState(None);
	}
}

bool CustomBorderContainer::isResizable() const
{
	return resizable;
}

void CustomBorderContainer::setResizable(bool resizable)
{
	this->resizable = resizable;
	if (!resizable)
	{
		resizeBorder = NoneBorder;
		updateCursor(QApplication::widgetAt(lastMousePosition));
		setGeometryState(None);
	}
}

void CustomBorderContainer::changeEvent(QEvent *e)
{
	QWidget::changeEvent(e);
}

void CustomBorderContainer::resizeEvent(QResizeEvent * event)
{
	QWidget::resizeEvent(event);
}

void CustomBorderContainer::mousePressEvent(QMouseEvent * event)
{
	if (event->button() == Qt::LeftButton)
		mousePress(event->pos(), this);
	QWidget::mousePressEvent(event);
}

void CustomBorderContainer::mouseMoveEvent(QMouseEvent * event)
{
	mouseMove(event->globalPos(), this);
	QWidget::mouseMoveEvent(event);
}

void CustomBorderContainer::mouseReleaseEvent(QMouseEvent * event)
{
	mouseRelease(event->pos(), this, event->button());
	QWidget::mouseReleaseEvent(event);
}

void CustomBorderContainer::mouseDoubleClickEvent(QMouseEvent * event)
{
	if (event->button() == Qt::LeftButton)
		mouseDoubleClick(event->pos(), this);
	QWidget::mouseDoubleClickEvent(event);
}

void CustomBorderContainer::moveEvent(QMoveEvent * event)
{
	QWidget::moveEvent(event);
}

void CustomBorderContainer::paintEvent(QPaintEvent * event)
{
	// painter
	QPainter painter;
	painter.begin(this);
	painter.setClipRect(event->rect());

	// header
	drawHeader(&painter);
	// borders
	drawBorders(&painter);
	// corners
	drawCorners(&painter);
	// buttons
	drawButtons(&painter);

	painter.end();
}

void CustomBorderContainer::enterEvent(QEvent * event)
{
	QWidget::enterEvent(event);
}

void CustomBorderContainer::leaveEvent(QEvent * event)
{
	lastMousePosition = QPoint(-1, -1);
	if (isVisible())
	{
		repaintHeaderButtons();
		setGeometryState(None);
		resizeBorder = NoneBorder;
		updateCursor();
	}
	QWidget::leaveEvent(event);
}

void CustomBorderContainer::focusInEvent(QFocusEvent * event)
{
	QWidget::focusInEvent(event);
}

void CustomBorderContainer::focusOutEvent(QFocusEvent * event)
{
	QWidget::focusOutEvent(event);
}

void CustomBorderContainer::contextMenuEvent(QContextMenuEvent * event)
{
	QWidget::contextMenuEvent(event);
}

bool CustomBorderContainer::event(QEvent * evt)
{
	if (evt->type() == QEvent::ToolTip)
	{
		QHelpEvent * helpEvent = (QHelpEvent *)evt;
		if (headerButtonRect(MinimizeButton).contains(helpEvent->pos()))
		{
			setToolTip(tr("Minimize"));
		}
		else if (headerButtonRect(MaximizeButton).contains(helpEvent->pos()))
		{
			setToolTip((_isMaximized || isFullScreen()) ? tr("Restore") : tr("Maximize"));
		}
		else if (headerButtonRect(CloseButton).contains(helpEvent->pos()))
		{
			setToolTip(tr("Close"));
		}
		else
			setToolTip("");
		if (toolTip().isEmpty())
		{
			helpEvent->ignore();
			return false;
		}
	}
	return QWidget::event(evt);
}

#ifdef Q_WS_WIN
bool CustomBorderContainer::winEvent(MSG *message, long *result)
{
	if (message->message == 0x0313)
		showWindowMenu(QCursor::pos());
	return QWidget::winEvent(message, result);
}
#endif

// TODO: make it less buggy...
bool CustomBorderContainer::eventFilter(QObject * object, QEvent * event)
{
	QWidget *widget = qobject_cast<QWidget*>(object);
	bool handled = false;
	switch (event->type())
	{
	case QEvent::MouseMove:
		mouseMove(((QMouseEvent*)event)->globalPos(), widget);
		break;
	case QEvent::MouseButtonPress:
	{
		if (((QMouseEvent*)event)->button() == Qt::LeftButton)
			handled = mousePress(((QMouseEvent*)event)->pos(), widget);

#ifdef DEBUG_ENABLED
		qDebug() << "handled = " << handled << " " << widget->objectName()
			 << " of class " << widget->metaObject()->className()
			 << " " << (qobject_cast<QPushButton*>(widget) ? ((qobject_cast<QPushButton*>(widget))->isDefault() ? "default" : " NOT default!") : "");
		QStringList hierarchy;
		QWidget * parent = widget->parentWidget();
		while (parent)
		{
			hierarchy << QString("%1 (%2)").arg(parent->objectName(), parent->metaObject()->className());
			parent = parent->parentWidget();
		}
		qDebug() << "hierarchy: " << hierarchy.join(" -> ");
#endif
	}
	break;
	case QEvent::MouseButtonRelease:
		mouseRelease(((QMouseEvent*)event)->pos(), widget, ((QMouseEvent*)event)->button());
		break;
	case QEvent::MouseButtonDblClick:
		if (shouldFilterEvents(object))
			if (((QMouseEvent*)event)->button() == Qt::LeftButton)
				handled = mouseDoubleClick(((QMouseEvent*)event)->pos(), widget);
		break;
	case QEvent::Paint:
	{
		object->removeEventFilter(this);
		QApplication::sendEvent(object,event);
		object->installEventFilter(this);

		QPoint point = widget->pos();
		while (widget && (widget->parentWidget() != this) && widget->parentWidget())
		{
			widget = widget->parentWidget();
			point += widget->pos();
		}

		widget = qobject_cast<QWidget*>(object);
		QRect r = widget->rect().translated(point);

		QPainter p(widget);
		p.setWindow(r);
		drawButtons(&p);
		drawCorners(&p);
		return true;
	}
	break;
	case QEvent::ChildAdded:
	case QEvent::ChildRemoved:
		childsRecursive(widget, this, true);
		break;
	case QEvent::WindowTitleChange:
		if (widget == containedWidget)
			setWindowTitle(containedWidget->windowTitle());
		break;
	case QEvent::Resize:
		if (object == containedWidget)
		{
			handled = QWidget::eventFilter(object, event);
			layout()->update();
			updateShape();
			return handled;
		}
		break;
	default:
		break;
	}
	handled = (handled || (geometryState() != None)) && (event->type() != QEvent::Resize);
	return handled ? handled : QWidget::eventFilter(object, event);
}

// use only for mouse events
bool CustomBorderContainer::shouldFilterEvents(QObject* obj)
{
	if (obj->property("ignoreFilter").toBool())
		return false;

	bool filter = true;

	//static QStringList exceptions;
	// TODO: make this list customizable
//	if (exceptions.isEmpty())
//		exceptions << "QAbstractButton"
//			   << "QLineEdit"
//			   << "QTextEdit"
//			   << "QScrollBar"
//			   << "QWebView"
//			   << "QAbstractItemView";
//	foreach (QString item, exceptions)
//	{
//		if (obj->inherits(item.toLatin1()))
//		{
//			filter = false;
//			break;
//		}
//	}
	// still not working... =(
	if (qobject_cast<QAbstractButton*>(obj) ||
			qobject_cast<QLineEdit*>(obj) ||
			qobject_cast<QComboBox*>(obj) ||
			qobject_cast<QSplitter*>(obj) ||
			qobject_cast<QTextEdit*>(obj->parent()) ||
			qobject_cast<QScrollBar*>(obj) ||
			qobject_cast<QWebView*>(obj) ||
			qobject_cast<QAbstractItemView*>(obj->parent()))
	{
		QWidget * w = qobject_cast<QWidget*>(obj);
		filter = !w->isEnabled();
	}
	return filter;
}

void CustomBorderContainer::init()
{
	windowMenu = NULL;
	// vars
	containedWidget = NULL;
	resizeBorder = NoneBorder;
	canMove = false;
	movable = true;
	resizable = true;
	buttonsFlags = MinimizeVisible | MaximizeVisible | CloseVisible | MinimizeEnabled | MaximizeEnabled | CloseEnabled;
	pressedHeaderButton = NoneButton;
	_isMaximized = false;
	_isFullscreen = false;
	// window props
	setWindowFlags(Qt::FramelessWindowHint);
	setAttribute(Qt::WA_TranslucentBackground);
	setFocusPolicy(Qt::NoFocus);
	setMouseTracking(true);
	setAttribute(Qt::WA_DeleteOnClose, false);
	// layout
	containerLayout = new QVBoxLayout;
	containerLayout->setContentsMargins(0, 0, 0, 0);
	setLayout(containerLayout);
	setMinimumWidth(100);
	setMinimumHeight(100);
	setGeometryState(None);
	// actions
	minimizeAction = new Action(this);
	maximizeAction = new Action(this);
	closeAction = new Action(this);
	restoreAction = new Action(this);
	addAction(minimizeAction);
	addAction(maximizeAction);
	addAction(closeAction);
	minimizeAction->setText(tr("Minimize"));
	minimizeAction->setIcon(style()->standardIcon(QStyle::SP_TitleBarMinButton));
	maximizeAction->setText(tr("Maximize"));
	maximizeAction->setIcon(style()->standardIcon(QStyle::SP_TitleBarMaxButton));
	closeAction->setText(tr("Close"));
	closeAction->setIcon(style()->standardIcon(QStyle::SP_TitleBarCloseButton));
	restoreAction->setText(tr("Restore"));
	restoreAction->setIcon(style()->standardIcon(QStyle::SP_TitleBarNormalButton));
	connect(minimizeAction, SIGNAL(triggered()), SIGNAL(minimizeClicked()));
	connect(maximizeAction, SIGNAL(triggered()), SIGNAL(maximizeClicked()));
	connect(closeAction, SIGNAL(triggered()), SIGNAL(closeClicked()));
	connect(restoreAction, SIGNAL(triggered()), SIGNAL(restoreClicked()));
	// connections
	connect(this, SIGNAL(minimizeClicked()), SLOT(minimizeWidget()));
	connect(this, SIGNAL(maximizeClicked()), SLOT(maximizeWidget()));
	connect(this, SIGNAL(closeClicked()), SLOT(closeWidget()));
	connect(this, SIGNAL(restoreClicked()), SLOT(restoreWidget()));
}

void CustomBorderContainer::initMenu()
{
	// window menu
	windowMenu = new Menu(this);
	windowMenu->addAction(restoreAction);
	windowMenu->addAction(minimizeAction);
	windowMenu->addAction(maximizeAction);
	windowMenu->addAction(closeAction);
}

CustomBorderContainer::GeometryState CustomBorderContainer::geometryState() const
{
	return currentGeometryState;
}

void CustomBorderContainer::setGeometryState(GeometryState newGeometryState)
{
	currentGeometryState = newGeometryState;
}

void CustomBorderContainer::updateGeometry(const QPoint & p)
{
	int dx, dy;
	switch (geometryState())
	{
	case Resizing:
		switch(resizeBorder)
		{
		case TopLeftCorner:
			oldGeometry.setTopLeft(p - QPoint(borderStyle->topLeft.resizeLeft, borderStyle->topLeft.resizeTop));
			break;
		case TopRightCorner:
			oldGeometry.setTopRight(p - QPoint(-borderStyle->topRight.resizeRight, borderStyle->topRight.resizeTop));
			break;
		case BottomLeftCorner:
			oldGeometry.setBottomLeft(p - QPoint(borderStyle->bottomLeft.resizeLeft, -borderStyle->bottomLeft.resizeBottom));
			break;
		case BottomRightCorner:
			oldGeometry.setBottomRight(p + QPoint(borderStyle->bottomRight.resizeRight, borderStyle->bottomRight.resizeBottom));
			break;
		case LeftBorder:
			oldGeometry.setLeft(p.x() - borderStyle->left.resizeMargin);
			break;
		case RightBorder:
			oldGeometry.setRight(p.x() + borderStyle->right.resizeMargin);
			break;
		case TopBorder:
			oldGeometry.setTop(p.y() - borderStyle->top.resizeMargin);
			break;
		case BottomBorder:
			oldGeometry.setBottom(p.y() + borderStyle->bottom.resizeMargin);
			break;
		default:
			break;
		}
		break;
	case Moving:
		dx = p.x() - oldPressPoint.x();
		dy = p.y() - oldPressPoint.y();
		oldPressPoint = p;
		oldGeometry.moveTo(oldGeometry.left() + dx, oldGeometry.top() + dy);
		break;
	case None:
		oldGeometry = QRect();
		break;
	}
	if (oldGeometry.isValid())
	{
		int bordersWidth = borderStyle->left.width + borderStyle->right.width;
		int bordersHeight = borderStyle->top.width + borderStyle->bottom.width;
		int minWidth = containedWidget->minimumWidth() + bordersWidth;
		int minHeight = containedWidget->minimumHeight() + bordersHeight;
		int maxWidth = containedWidget->maximumWidth() + bordersWidth;
		int maxHeight = containedWidget->maximumHeight() + bordersHeight;
		// left border is changing
		if (resizeBorder == LeftBorder || resizeBorder == TopLeftCorner || resizeBorder == BottomLeftCorner)
		{
			if (oldGeometry.width() < minWidth)
				oldGeometry.setLeft(oldGeometry.right() - minWidth);
			if (oldGeometry.width() > maxWidth)
				oldGeometry.setLeft(oldGeometry.right() - maxWidth);
		}
		// top border is changing
		if (resizeBorder == TopBorder || resizeBorder == TopLeftCorner || resizeBorder == TopRightCorner)
		{
			if (oldGeometry.height() < minHeight)
				oldGeometry.setTop(oldGeometry.bottom() - minHeight);
			if (oldGeometry.height() > maxHeight)
				oldGeometry.setTop(oldGeometry.bottom() - maxHeight);
		}
		// right border is changing
		if (resizeBorder == RightBorder || resizeBorder == TopRightCorner || resizeBorder == BottomRightCorner)
		{
			if (oldGeometry.width() < minWidth)
				oldGeometry.setWidth(minWidth);
			if (oldGeometry.width() > maxWidth)
				oldGeometry.setHeight(maxWidth);
		}
		// bottom border is changing
		if (resizeBorder == BottomBorder || resizeBorder == BottomLeftCorner || resizeBorder == BottomRightCorner)
		{
			if (oldGeometry.height() < minHeight)
				oldGeometry.setHeight(minHeight);
			if (oldGeometry.height() > maxHeight)
				oldGeometry.setHeight(maxHeight);
		}
		setGeometry(oldGeometry);
		QApplication::flush();
	}
}

CustomBorderContainer::HeaderButtonsFlags CustomBorderContainer::headerButtonsFlags() const
{
	return buttonsFlags;
}

void CustomBorderContainer::setHeaderButtonFlags(HeaderButtonsFlag flags)
{
	buttonsFlags = HeaderButtonsFlags(flags);
	repaintHeaderButtons();
}

bool CustomBorderContainer::isMinimizeButtonVisible() const
{
	return buttonsFlags.testFlag(MinimizeVisible);
}

void CustomBorderContainer::setMinimizeButtonVisible(bool visible)
{
	if (visible)
		addHeaderButtonFlag(MinimizeVisible);
	else
		removeHeaderButtonFlag(MinimizeVisible);
}

bool CustomBorderContainer::isMinimizeButtonEnabled() const
{
	return buttonsFlags.testFlag(MinimizeEnabled);
}

void CustomBorderContainer::setMinimizeButtonEnabled(bool enabled)
{
	if (enabled)
		addHeaderButtonFlag(MinimizeEnabled);
	else
		removeHeaderButtonFlag(MinimizeEnabled);
}

bool CustomBorderContainer::isMaximizeButtonVisible() const
{
	return buttonsFlags.testFlag(MaximizeVisible);
}

void CustomBorderContainer::setMaximizeButtonVisible(bool visible)
{
	if (visible)
		addHeaderButtonFlag(MaximizeVisible);
	else
		removeHeaderButtonFlag(MaximizeVisible);
}

bool CustomBorderContainer::isMaximizeButtonEnabled() const
{
	return buttonsFlags.testFlag(MaximizeEnabled);
}

void CustomBorderContainer::setMaximizeButtonEnabled(bool enabled)
{
	if (enabled)
		addHeaderButtonFlag(MaximizeEnabled);
	else
		removeHeaderButtonFlag(MaximizeEnabled);
}

bool CustomBorderContainer::isCloseButtonVisible() const
{
	return buttonsFlags.testFlag(CloseVisible);
}

void CustomBorderContainer::setCloseButtonVisible(bool visible)
{
	if (visible)
		addHeaderButtonFlag(CloseVisible);
	else
		removeHeaderButtonFlag(CloseVisible);
}

bool CustomBorderContainer::isCloseButtonEnabled() const
{
	return buttonsFlags.testFlag(CloseEnabled);
}

void CustomBorderContainer::setCloseButtonEnabled(bool enabled)
{
	if (enabled)
		addHeaderButtonFlag(CloseEnabled);
	else
		removeHeaderButtonFlag(CloseEnabled);
}

int CustomBorderContainer::headerMoveLeft() const
{
	return borderStyle->header.moveLeft;
}

void CustomBorderContainer::setHeaderMoveLeft(int left)
{
	borderStyle->header.moveLeft = left;
}

int CustomBorderContainer::headerMoveRight() const
{
	return borderStyle->header.moveRight;
}

void CustomBorderContainer::setHeaderMoveRight(int right)
{
	borderStyle->header.moveRight = right;
}

int CustomBorderContainer::headerMoveTop() const
{
	return borderStyle->header.moveTop;
}

void CustomBorderContainer::setHeaderMoveTop(int top)
{
	borderStyle->header.moveTop = top;
}

int CustomBorderContainer::headerMoveHeight() const
{
	return borderStyle->header.moveHeight;
}

void CustomBorderContainer::setHeaderMoveHeight(int height)
{
	borderStyle->header.moveHeight = height;
}

int CustomBorderContainer::leftBorderWidth() const
{
	return borderStyle->left.width;
}

int CustomBorderContainer::rightBorderWidth() const
{
	return borderStyle->right.width;
}

int CustomBorderContainer::topBorderWidth() const
{
	return borderStyle->top.width;
}

int CustomBorderContainer::bottomBorderWidth() const
{
	return borderStyle->bottom.width;
}

bool CustomBorderContainer::canDragAnywhere() const
{
	return borderStyle->dragAnywhere;
}

void CustomBorderContainer::setCanDragAnywhere(bool on)
{
	borderStyle->dragAnywhere = on;
}

void CustomBorderContainer::addHeaderButtonFlag(HeaderButtonsFlag flag)
{
	buttonsFlags |= flag;
}

void CustomBorderContainer::removeHeaderButtonFlag(HeaderButtonsFlag flag)
{
	if (buttonsFlags.testFlag(flag))
		buttonsFlags ^= flag;
}

int CustomBorderContainer::headerButtonsCount() const
{
	return (isMinimizeButtonVisible() ? 1 : 0) + (isMaximizeButtonVisible() ? 1 : 0) + (isCloseButtonVisible() ? 1 : 0);
}

QRect CustomBorderContainer::headerButtonRect(HeaderButtons button) const
{
	if (isFullScreen())
		return QRect();
	int numButtons = headerButtonsCount();
	int startX = width() - (_isMaximized ? 0 : borderStyle->right.width) - borderStyle->header.margins.right() - (numButtons - 1) * borderStyle->controls.spacing;
	startX -= (isMinimizeButtonVisible() ? borderStyle->minimize.width : 0) + (isMaximizeButtonVisible() ? borderStyle->maximize.width : 0) + (isCloseButtonVisible() ? borderStyle->close.width : 0);
	int startY = (_isMaximized ? 0 : borderStyle->top.width) + borderStyle->header.margins.top();
	int dx = 0;
	QRect buttonRect;
	switch (button)
	{
	case MinimizeButton:
		if (isMinimizeButtonVisible())
			buttonRect = QRect(startX, startY, borderStyle->minimize.width, borderStyle->minimize.height);
		break;
	case MaximizeButton:
		if (isMaximizeButtonVisible())
		{
			if (isMinimizeButtonVisible())
				dx = borderStyle->minimize.width + borderStyle->controls.spacing;
			buttonRect = QRect(startX + dx, startY, borderStyle->maximize.width, borderStyle->maximize.height);
		}
		break;
	case CloseButton:
		if (isCloseButtonVisible())
		{
			if (isMinimizeButtonVisible())
				dx = borderStyle->minimize.width + borderStyle->controls.spacing;
			if (isMaximizeButtonVisible())
				dx += borderStyle->maximize.width + borderStyle->controls.spacing;
			buttonRect = QRect(startX + dx, startY, borderStyle->close.width, borderStyle->close.height);
		}
		break;
	default:
		break;
	}
	return buttonRect;
}

bool CustomBorderContainer::minimizeButtonUnderMouse() const
{
	return isMinimizeButtonVisible() && headerButtonRect(MinimizeButton).contains(lastMousePosition);
}

bool CustomBorderContainer::maximizeButtonUnderMouse() const
{
	return isMaximizeButtonVisible() && headerButtonRect(MaximizeButton).contains(lastMousePosition);
}

bool CustomBorderContainer::closeButtonUnderMouse() const
{
	return isCloseButtonVisible() && headerButtonRect(CloseButton).contains(lastMousePosition);
}

CustomBorderContainer::HeaderButtons CustomBorderContainer::headerButtonUnderMouse() const
{
	if (minimizeButtonUnderMouse())
		return MinimizeButton;
	else if (maximizeButtonUnderMouse())
		return MaximizeButton;
	else if (closeButtonUnderMouse())
		return CloseButton;
	else
		return NoneButton;
}

QRect CustomBorderContainer::headerButtonsRect() const
{
	if (isFullScreen())
		return QRect();
	int tb = (_isMaximized ? 0 : borderStyle->top.width) + borderStyle->header.margins.top(), rb = (_isMaximized ? 0 : borderStyle->right.width) + borderStyle->header.margins.right();
	int numButtons = headerButtonsCount();
	int lb = width() - (_isMaximized ? 0 : borderStyle->right.width) - borderStyle->header.margins.right() - (numButtons - 1) * borderStyle->controls.spacing;
	lb -= (isMinimizeButtonVisible() ? borderStyle->minimize.width : 0) + (isMaximizeButtonVisible() ? borderStyle->maximize.width : 0) + (isCloseButtonVisible() ? borderStyle->close.width : 0);
	int h = qMax(qMax((isMinimizeButtonVisible() ? borderStyle->minimize.height: 0), (isMaximizeButtonVisible() ? borderStyle->maximize.height: 0)), (isCloseButtonVisible() ? borderStyle->close.height: 0)) + 1;
	return QRect(lb, tb, width() - lb - rb, h);
}

void CustomBorderContainer::repaintHeaderButtons()
{
	QRect rect = headerButtonsRect();
	repaint(rect);
	rect.moveTopLeft(mapToGlobal(rect.topLeft()));
	repaintRecursive(containedWidget, rect);
}

QRect CustomBorderContainer::windowIconRect() const
{
	if (isFullScreen())
		return QRect();
	int x = (_isMaximized ? 0 : borderStyle->left.width) + borderStyle->header.margins.left();
	int y = (_isMaximized ? 0 : borderStyle->top.width) + borderStyle->header.margins.top();
	return QRect(x, y, borderStyle->icon.width, borderStyle->icon.height);
}

void CustomBorderContainer::showWindowMenu(const QPoint & p)
{
	if (isFullScreen() && !windowMenu)
		return;
	minimizeAction->setEnabled(isMinimizeButtonVisible() && isMinimizeButtonEnabled() && !isMinimized());
	maximizeAction->setEnabled(isMaximizeButtonVisible() && isMaximizeButtonEnabled() && !_isMaximized && !isMinimized());
	closeAction->setEnabled(isCloseButtonVisible() && isCloseButtonEnabled());
	restoreAction->setEnabled(isMinimized() || _isMaximized);
	windowMenu->adjustSize();
	QPoint popupPoint = p;
	QRect screen = QApplication::desktop()->availableGeometry(p);
	if (popupPoint.y() + windowMenu->geometry().height() > screen.bottom())
		popupPoint.setY(popupPoint.y() - windowMenu->geometry().height());
	if (windowMenu->geometry().width() + popupPoint.x() > screen.right())
		popupPoint.setX(screen.right() - windowMenu->geometry().width());
	windowMenu->move(popupPoint);
	windowMenu->show();
}

void CustomBorderContainer::mouseMove(const QPoint & point, QWidget * widget)
{
	bool needToRepaintHeaderButtons = (!headerButtonsRect().contains(mapFromGlobal(point))) && headerButtonsRect().contains(lastMousePosition);
	lastMousePosition = mapFromGlobal(point);
	if (needToRepaintHeaderButtons)
		repaintHeaderButtons();
	if (geometryState() != None)
	{
		updateGeometry(point);
		return;
	}
	else
	{
		if (geometryState() != Resizing)
		{
			checkResizeCondition(mapFromGlobal(point));
		}
		if (geometryState() != Moving)
		{
			if (shouldFilterEvents(widget))
				checkMoveCondition(mapFromGlobal(point));
		}
	}
}

bool CustomBorderContainer::mousePress(const QPoint & p, QWidget * widget)
{
	bool handled = false;
	pressedHeaderButton = headerButtonUnderMouse();
	if (pressedHeaderButton == NoneButton)
	{
		if (resizeBorder != NoneBorder)
		{
			setGeometryState(Resizing);
			handled = true;
		}
		else if (windowIconRect().contains(mapFromWidget(widget, p)) && headerRect().contains(mapFromWidget(widget, p)))
		{
			showWindowMenu(mapToGlobal(windowIconRect().bottomLeft()));
			handled = true;
		}
		else if (canMove && shouldFilterEvents(widget))
		{
			oldPressPoint = widget->mapToGlobal(p);
			setGeometryState(Moving);
			handled = true;
		}
		oldGeometry = geometry();
	}
	return handled;
}

void CustomBorderContainer::mouseRelease(const QPoint & p, QWidget * widget, Qt::MouseButton button)
{
	if (button == Qt::LeftButton)
	{
		if (pressedHeaderButton == NoneButton)
		{
			setGeometryState(None);
			resizeBorder = NoneBorder;
			canMove = false;
			updateCursor(widget);
		}
		else
		{
			if (headerButtonUnderMouse() == pressedHeaderButton)
			{
				switch (pressedHeaderButton)
				{
				case MinimizeButton:
					if (isMinimizeButtonEnabled())
						emit minimizeClicked();
					break;
				case MaximizeButton:
					if (isMaximizeButtonEnabled())
						emit maximizeClicked();
					break;
				case CloseButton:
					if (isCloseButtonEnabled())
						emit closeClicked();
					break;
				default:
					break;
				}
			}
		}
	}
	else if ((button == Qt::RightButton) && shouldFilterEvents(widget))
	{
		if (headerMoveRect().contains(mapFromWidget(widget, p)) && !headerButtonsRect().contains(mapFromWidget(widget, p)))
		{
			showWindowMenu(widget->mapToGlobal(p));
		}
	}
}

bool CustomBorderContainer::mouseDoubleClick(const QPoint & p, QWidget * widget)
{
	if (windowIconRect().contains(mapFromWidget(widget, p)) && headerRect().contains(mapFromWidget(widget, p)))
	{
		emit closeClicked();
		return true;
	}
	else if (headerMoveRect().contains(mapFromWidget(widget, p)) && headerButtonUnderMouse() == NoneButton && isMaximizeButtonVisible() && isMaximizeButtonEnabled())
	{
		emit maximizeClicked();
		return true;
	}
	return false;
}

bool CustomBorderContainer::pointInBorder(BorderType border, const QPoint & p)
{
	// NOTE: it is suggested that point is local for "this" widget
	Corner c;
	Border b;
	QRect cornerRect;
	int x, y, h, w;
	switch(border)
	{
	case TopLeftCorner:
		c = borderStyle->topLeft;
		x = c.resizeLeft;
		y = c.resizeTop;
		w = c.resizeWidth;
		h = c.resizeHeight;
		cornerRect = QRect(x, y, w, h);
		return cornerRect.contains(p);
		break;
	case TopRightCorner:
		c = borderStyle->topRight;
		x = width() - c.resizeRight - c.resizeWidth;
		y = c.resizeTop;
		w = c.resizeWidth;
		h = c.resizeHeight;
		cornerRect = QRect(x, y, w, h);
		return cornerRect.contains(p);
		break;
	case BottomLeftCorner:
		c = borderStyle->bottomLeft;
		x = c.resizeLeft;
		y = height() - c.resizeBottom - c.resizeHeight;
		w = c.resizeWidth;
		h = c.resizeHeight;
		cornerRect = QRect(x, y, w, h);
		return cornerRect.contains(p);
		break;
	case BottomRightCorner:
		c = borderStyle->bottomRight;
		x = width() - c.resizeRight - c.resizeWidth;
		y = height() - c.resizeBottom - c.resizeHeight;
		w = c.resizeWidth;
		h = c.resizeHeight;
		cornerRect = QRect(x, y, w, h);
		return cornerRect.contains(p);
		break;
	case LeftBorder:
		b = borderStyle->left;
		return (p.x() <= (b.resizeWidth + b.resizeMargin)) && (p.x() > b.resizeMargin) && (p.y() > borderStyle->top.resizeMargin && p.y() < (height() - borderStyle->bottom.resizeMargin));
		break;
	case RightBorder:
		b = borderStyle->right;
		return (p.x() > (width() - b.resizeWidth - b.resizeMargin)) && (p.x() < (width() - b.resizeMargin)) && (p.y() > borderStyle->top.resizeMargin && p.y() < (height() - borderStyle->bottom.resizeMargin));
		break;
	case TopBorder:
		b = borderStyle->top;
		return (p.y() < (b.resizeWidth + b.resizeMargin)) && (p.y() > b.resizeMargin) && (p.x() > borderStyle->left.resizeMargin && p.x() < (width() - borderStyle->right.resizeMargin));
		break;
	case BottomBorder:
		b = borderStyle->bottom;
		return (p.y() > (height() - b.resizeWidth - b.resizeMargin)) && (p.y() < (height() - b.resizeMargin)) && (p.x() > borderStyle->left.resizeMargin && p.x() < (width() - borderStyle->right.resizeMargin));
		break;
	default:
		break;
	}
	return false;
}

bool CustomBorderContainer::pointInHeader(const QPoint & p)
{
	int lb = borderStyle->left.resizeWidth, tb = borderStyle->top.resizeWidth, rb = borderStyle->right.resizeWidth;
	QRect headerRect(lb, tb, width() - lb - rb, borderStyle->header.moveHeight);
	return headerRect.contains(p);
}

void CustomBorderContainer::checkResizeCondition(const QPoint & p)
{
	resizeBorder = NoneBorder;
	if (!_isMaximized && !isFullScreen() && resizable)
		for (int b = TopLeftCorner; b <= BottomBorder; b++)
			if (pointInBorder((BorderType)b, p))
			{
				resizeBorder = (BorderType)b;
				break;
			}
	updateCursor(QApplication::widgetAt(mapToGlobal(p)));
}

void CustomBorderContainer::checkMoveCondition(const QPoint & p)
{
	if (headerButtonsRect().contains(p))
	{
		repaintHeaderButtons();
	}
	if (_isMaximized || isFullScreen() || !movable)
		canMove = false;
	else
		canMove = headerMoveRect().contains(p);
}

void CustomBorderContainer::updateCursor(QWidget * widget)
{
	Q_UNUSED(widget)
	static bool overrideCursorSet = false;
	QCursor newCursor;
	switch(resizeBorder)
	{
	case TopLeftCorner:
	case BottomRightCorner:
		newCursor.setShape(Qt::SizeFDiagCursor);
		break;
	case TopRightCorner:
	case BottomLeftCorner:
		newCursor.setShape(Qt::SizeBDiagCursor);
		break;
	case LeftBorder:
	case RightBorder:
		newCursor.setShape(Qt::SizeHorCursor);
		break;
	case TopBorder:
	case BottomBorder:
		newCursor.setShape(Qt::SizeVerCursor);
		break;
	case NoneBorder:
	default:
		if (overrideCursorSet)
		{
			QApplication::restoreOverrideCursor();
			overrideCursorSet = false;
		}
		return;
		break;
	}
	if (!overrideCursorSet || (QApplication::overrideCursor() && (QApplication::overrideCursor()->shape() != newCursor.shape())))
	{
		if (overrideCursorSet)
			QApplication::restoreOverrideCursor();
		QApplication::setOverrideCursor(newCursor);
		overrideCursorSet = true;
	}
}

void CustomBorderContainer::updateShape()
{
	// cached masks
	QPixmap topLeftMask, topRightMask, bottomLeftMask, bottomRightMask;
	if (!containedWidget)
		return;
	if (!(_isMaximized || isFullScreen()))
	{
		if (borderStyle->topLeft.mask.isEmpty())
		{
#ifdef Q_WS_X11 // i don't know why this works well on X11 and bad on WIN...
			// base rect
			QRegion shape(0, 0, containedWidget->width(), containedWidget->height());
			QRegion rect, circle;
			int rad;
			QRect g = containedWidget->geometry();
			int w = g.width();
			int h = g.height();
			// for each corner we substract rect and add circle
			// top-left
			rad = borderStyle->topLeft.radius;
			rect = QRegion(0, 0, rad, rad);
			circle = QRegion(0, 0, 2 * rad + 1, 2 * rad + 1, QRegion::Ellipse);
			shape -= rect;
			shape |= circle;
			// top-right
			rad = borderStyle->topRight.radius;
			rect = QRegion(w - rad, 0, rad, rad);
			circle = QRegion(w - 2 * rad - 1, 0, 2 * rad, 2 * rad, QRegion::Ellipse);
			shape -= rect;
			shape |= circle;
			// bottom-left
			rad = borderStyle->bottomLeft.radius;
			rect = QRegion(0, h - rad, rad, rad);
			circle = QRegion(1, h - 2 * rad - 1, 2 * rad, 2 * rad, QRegion::Ellipse);
			shape -= rect;
			shape |= circle;
			// bottom-right
			rad = borderStyle->bottomRight.radius;
			rect = QRegion(w - rad, h - rad, rad, rad);
			circle = QRegion(w - 2 * rad - 1, h - 2 * rad - 1, 2 * rad, 2 * rad, QRegion::Ellipse);
			shape -= rect;
			shape |= circle;
			// setting mask
			containedWidget->setMask(shape);
#else
			QPixmap pixmap(containedWidget->geometry().size());
			pixmap.fill(Qt::transparent);
			QPainter p(&pixmap);
			p.setBrush(QBrush(Qt::black));
			p.setPen(QPen());
			QRect rect;
			QRect g = containedWidget->geometry();
			int w = g.width();
			int h = g.height();
			w += (1 - w % 2);
			h += (1 - h % 2);
			int dx = w % 2;
			int dy = h % 2;
			int rad;
			// top-left
			rad = borderStyle->topLeft.radius;
			rect = QRect(0, 0, w / 2 + rad, h / 2 + rad);
			p.drawRoundedRect(rect, rad, rad);
			// top-right
			rad = borderStyle->topRight.radius;
			rect = QRect(w / 2 - rad - dx, 0, w / 2 + rad, h / 2 + rad);
			p.drawRoundedRect(rect, rad, rad);
			// bottom-left
			rad = borderStyle->bottomLeft.radius;
			rect = QRect(0, h / 2 - rad - dy, w / 2 + rad, h / 2 + rad);
			p.drawRoundedRect(rect, rad, rad);
			// bottom-right
			rad = borderStyle->bottomRight.radius;
			rect = QRect(w / 2 - rad - dx, h / 2 - rad - dy, w / 2 + rad, h / 2 + rad);
			p.drawRoundedRect(rect, rad, rad);
			p.end();
			containedWidget->setMask(pixmap.mask());
#endif // Q_WS_X11
		}
		else
		{
			// loading mask images
			topLeftMask = loadPixmap(borderStyle->topLeft.mask);
			topRightMask = loadPixmap(borderStyle->topRight.mask);
			bottomLeftMask = loadPixmap(borderStyle->bottomLeft.mask);
			bottomRightMask = loadPixmap(borderStyle->bottomRight.mask);
			QPixmap pixmap(containedWidget->geometry().size());
			pixmap.fill(Qt::transparent);
			QPainter p(&pixmap);
			p.setBrush(QBrush(Qt::black));
			p.setPen(QPen());
			QRect cornerRectTL, cornerRectTR, cornerRectBL, cornerRectBR;
			QRect g = containedWidget->geometry();
			g.moveTopLeft(QPoint(0, 0));
			int w = g.width();
			int h = g.height();
			cornerRectTL = QRect(0,
					     0,
					     borderStyle->topLeft.width - borderStyle->topLeft.resizeLeft,
					     borderStyle->topLeft.height - borderStyle->topLeft.resizeTop);
			cornerRectTR = QRect(w - (borderStyle->topRight.width - borderStyle->topRight.resizeRight),
					     0,
					     borderStyle->topRight.width - borderStyle->topRight.resizeRight,
					     borderStyle->topRight.height - borderStyle->topRight.resizeTop);
			cornerRectBL = QRect(0,
					     h - (borderStyle->bottomLeft.height - borderStyle->bottomLeft.resizeBottom),
					     borderStyle->bottomLeft.width - borderStyle->bottomLeft.resizeLeft,
					     borderStyle->bottomLeft.height - borderStyle->bottomLeft.resizeBottom);
			cornerRectBR = QRect(w - (borderStyle->bottomRight.width - borderStyle->bottomRight.resizeRight),
					     h - (borderStyle->bottomRight.height - borderStyle->bottomRight.resizeBottom),
					     borderStyle->bottomRight.width - borderStyle->bottomRight.resizeRight,
					     borderStyle->bottomRight.height - borderStyle->bottomRight.resizeBottom);
			QRegion reg(g);
			reg -= cornerRectTL;
			reg -= cornerRectTR;
			reg -= cornerRectBL;
			reg -= cornerRectBR;
			QPainterPath path;
			path.addRegion(reg);
			p.drawPath(path);
			// top-left mask
			p.drawPixmap(-borderStyle->topLeft.resizeLeft, -borderStyle->topLeft.resizeTop, topLeftMask);
			// top-right mask
			p.drawPixmap(w - cornerRectTR.width(), -borderStyle->topRight.resizeTop, topRightMask);
			// bottom-left mask
			p.drawPixmap(-borderStyle->bottomLeft.resizeLeft, h - cornerRectBL.height(), bottomLeftMask);
			// bottom-right mask
			p.drawPixmap(w - cornerRectBR.width(), h - cornerRectBR.height(), bottomRightMask);
			p.end();
			containedWidget->setMask(pixmap.mask());
		}
	}
	else
	{
		// clearing window mask if it is maximized or fullscreen
		containedWidget->clearMask();
	}
}

void CustomBorderContainer::updateIcons()
{
	setWindowIcon(loadIcon(borderStyle->icon.icon));
	minimizeAction->setIcon(loadIcon(borderStyle->minimize.imageNormal));
	maximizeAction->setIcon(loadIcon(borderStyle->maximize.imageNormal));
	closeAction->setIcon(loadIcon(borderStyle->close.imageNormal));
	restoreAction->setIcon(loadIcon(borderStyle->restore.imageNormal));
}

void CustomBorderContainer::setLayoutMargins()
{
	if (_isMaximized)
		layout()->setContentsMargins(0, borderStyle->header.height, 0, 0);
	else if (isFullScreen())
		layout()->setContentsMargins(0, 0, 0, 0);
	else
		layout()->setContentsMargins(borderStyle->left.width, borderStyle->top.width + borderStyle->header.height, borderStyle->right.width, borderStyle->bottom.width);
}

QRect CustomBorderContainer::headerRect() const
{
	if (isFullScreen())
		return QRect();
	if (_isMaximized)
		return QRect(0, 0, width(), borderStyle->header.height);
	else
		return QRect(borderStyle->left.width, borderStyle->top.width, width() - borderStyle->right.width, borderStyle->header.height);
}

QRect CustomBorderContainer::headerMoveRect() const
{
	if (isFullScreen())
		return QRect();
	int moveHeight = borderStyle->dragAnywhere ? (height() - (_isMaximized ? 0 : (borderStyle->top.width + borderStyle->bottom.width))) : borderStyle->header.moveHeight;
	if (_isMaximized)
		return QRect(borderStyle->header.moveLeft, borderStyle->header.moveTop, width() - borderStyle->header.moveRight, moveHeight);
	else
		return QRect(borderStyle->left.width + borderStyle->header.moveLeft, borderStyle->top.width + borderStyle->header.moveTop, width() - borderStyle->right.width - borderStyle->left.width - borderStyle->header.moveRight, moveHeight);
}

void CustomBorderContainer::drawHeader(QPainter * p)
{
	QPainterPath path;
	if (!mask().isEmpty())
		path.addRegion(mask() & headerRect());
	else
		path.addRegion(headerRect());
	if (borderStyle->header.gradient)
		p->fillPath(path, QBrush(*(borderStyle->header.gradient)));
	drawIcon(p);
	drawTitle(p);
}

void CustomBorderContainer::drawButton(HeaderButton & button, QPainter * p, HeaderButtonState state)
{
	QImage img;
	switch(state)
	{
	case Normal:
	case Disabled:
		img = loadImage(button.imageNormal);
		if (img.isNull())
			p->fillRect(0, 0, button.width, button.height, QColor::fromRgb(0, 0, 0));
		else
			p->drawImage(0, 0, img);
		break;
	default:
		img = loadImage(button.imageHover);
		if (img.isNull())
			p->fillRect(0, 0, button.width, button.height, QColor::fromRgb(50, 50, 50));
		else
			p->drawImage(0, 0, img);
		break;
	}
}

void CustomBorderContainer::drawButtons(QPainter * p)
{
	HeaderButtonState state;
	// minimize button
	if (isMinimizeButtonVisible())
	{
		p->save();
		p->translate(headerButtonRect(MinimizeButton).topLeft());
		state = minimizeButtonUnderMouse() ? NormalHover : Normal;
		drawButton(borderStyle->minimize, p, state);
		p->restore();
	}
	// maximize button
	if (isMaximizeButtonVisible())
	{
		p->save();
		p->translate(headerButtonRect(MaximizeButton).topLeft());
		state = maximizeButtonUnderMouse() ? NormalHover : Normal;
		drawButton(_isMaximized ? borderStyle->restore : borderStyle->maximize, p, state);
		p->restore();
	}
	// close button
	if (isCloseButtonVisible())
	{
		p->save();
		p->translate(headerButtonRect(CloseButton).topLeft());
		state = closeButtonUnderMouse() ? NormalHover : Normal;
		drawButton(borderStyle->close, p, state);
		p->restore();
	}
}

void CustomBorderContainer::drawIcon(QPainter * p)
{
	QIcon icon = windowIcon().isNull() ? qApp->windowIcon() : windowIcon();
	p->save();
	p->setClipRect(headerRect());
	icon.paint(p, windowIconRect());
	p->restore();
}

void CustomBorderContainer::drawTitle(QPainter * p)
{
	QTextDocument doc;
	doc.setHtml(QString("<font size=+1 color=%1><b>%2</b></font>").arg(borderStyle->title.color.name(), windowTitle()));
	// center title in header rect
	p->save();
	p->setClipRect(headerRect());
	qreal dx, dy;
	dx = (headerRect().width() - doc.size().width()) / 2.0 + headerRect().left();
	dy = (headerRect().height() - doc.size().height()) / 2.0 + headerRect().top();
	if (dx < 0.0)
		dx = 0.0;
	if (dy < 0.0)
		dy = 0.0;
	p->translate(dx, dy);
	doc.drawContents(p, QRectF(headerRect().translated(-headerRect().topLeft())));
	p->restore();
}

void CustomBorderContainer::drawBorders(QPainter * p)
{
	// note: image is preferred to draw borders
	// only stretch image is supported for now

	if (!(_isMaximized || isFullScreen()))
	{
		int dx = 0, dy = 0;
#ifdef Q_WS_WIN
		// angry hack
		if (containedWidget && borderStyle->topLeft.mask.isEmpty())
		{
			dx = containedWidget->width() % 2;
			dy = containedWidget->height() % 2;
		}
#endif
		QRect borderRect;
		// left
		borderRect = QRect(0, borderStyle->topLeft.height, borderStyle->left.width, height() - borderStyle->bottomLeft.height - borderStyle->topLeft.height - dy);
		if (borderStyle->left.image.isEmpty())
			p->fillRect(borderRect, QBrush(*(borderStyle->left.gradient)));
		else
			p->drawImage(borderRect, loadImage(borderStyle->left.image));
		// right
		borderRect = QRect(width() - borderStyle->right.width, borderStyle->topRight.height, borderStyle->right.width, height() - borderStyle->bottomRight.height - borderStyle->topRight.height - dy);
		if (borderStyle->right.image.isEmpty())
			p->fillRect(borderRect, QBrush(*(borderStyle->right.gradient)));
		else
			//p->fillRect(borderRect, QBrush(QColor::fromRgb(255, 0, 0, 200)));
			p->drawImage(borderRect, loadImage(borderStyle->right.image));
		// top
		borderRect = QRect(borderStyle->topLeft.width, 0, width() - borderStyle->topRight.width - borderStyle->topLeft.width - dx, borderStyle->top.width);
		if (borderStyle->top.image.isEmpty())
			p->fillRect(borderRect, QBrush(*(borderStyle->top.gradient)));
		else
			p->drawImage(borderRect, loadImage(borderStyle->top.image));
		// bottom
		borderRect = QRect(borderStyle->bottomLeft.width, height() - borderStyle->bottom.width, width() - borderStyle->bottomRight.width - borderStyle->bottomLeft.width - dx, borderStyle->bottom.width);
		if (borderStyle->bottom.image.isEmpty())
			p->fillRect(borderRect, QBrush(*(borderStyle->bottom.gradient)));
		else
			//p->fillRect(borderRect, QBrush(QColor::fromRgb(255, 0, 0, 200)));
			p->drawImage(borderRect, loadImage(borderStyle->bottom.image));
	}
}

void CustomBorderContainer::drawCorners(QPainter * p)
{
	// note: image is preferred to draw corners
	// only stretch image is supported for now

	if (!(_isMaximized || isFullScreen()))
	{
		int dx = 0, dy = 0;
#ifdef Q_WS_WIN
		// angry hack
		if (containedWidget && borderStyle->topLeft.mask.isEmpty())
		{
			dx = containedWidget->width() % 2;
			dy = containedWidget->height() % 2;
		}
#endif
		QRect cornerRect;
		// top-left
		cornerRect = QRect(0, 0, borderStyle->topLeft.width, borderStyle->topLeft.height);
		if (borderStyle->topLeft.image.isEmpty())
			p->fillRect(cornerRect, QBrush(*(borderStyle->topLeft.gradient)));
		else
			p->drawImage(cornerRect, loadImage(borderStyle->topLeft.image));
		// top-right
		cornerRect = QRect(width() - borderStyle->topRight.width - dx, 0, borderStyle->topRight.width, borderStyle->topRight.height);
		if (borderStyle->topRight.image.isEmpty())
			p->fillRect(cornerRect, QBrush(*(borderStyle->topRight.gradient)));
		else
			p->drawImage(cornerRect, loadImage(borderStyle->topRight.image));
		// bottom-left
		cornerRect = QRect(0, height() - borderStyle->bottomLeft.height - dy, borderStyle->bottomLeft.width, borderStyle->bottomLeft.height);
		if (borderStyle->bottomLeft.image.isEmpty())
			p->fillRect(cornerRect, QBrush(*(borderStyle->bottomLeft.gradient)));
		else
			p->drawImage(cornerRect, loadImage(borderStyle->bottomLeft.image));
		// bottom-right
		cornerRect = QRect(width() - borderStyle->bottomRight.width - dx, height() - borderStyle->bottomRight.height - dy, borderStyle->bottomRight.width, borderStyle->bottomRight.height);
		if (borderStyle->bottomRight.image.isEmpty())
			p->fillRect(cornerRect, QBrush(*(borderStyle->bottomRight.gradient)));
		else
			p->drawImage(cornerRect, loadImage(borderStyle->bottomRight.image));
	}
}

QPoint CustomBorderContainer::mapFromWidget(QWidget * widget, const QPoint &point)
{
	if (widget == this)
		return point;
	else
		return mapFromGlobal(widget->mapToGlobal(point));
}

QImage CustomBorderContainer::loadImage(const QString & key)
{
	QStringList list = key.split("/");
	if (list.count() != 2)
		return QImage();
	QString storage = list[0];
	QString imageKey = list[1];
	return IconStorage::staticStorage(storage)->getImage(imageKey);
}

QIcon CustomBorderContainer::loadIcon(const QString & key)
{
	QStringList list = key.split("/");
	if (list.count() != 2)
		return QIcon();
	QString storage = list[0];
	QString iconKey = list[1];
	return IconStorage::staticStorage(storage)->getIcon(iconKey);
}

QPixmap CustomBorderContainer::loadPixmap(const QString & key)
{
	return QPixmap::fromImage(loadImage(key));
}

bool CustomBorderContainer::isMaximized() const
{
	return _isMaximized;
}

bool CustomBorderContainer::isFullScreen() const
{
	return _isFullscreen;
}

void CustomBorderContainer::showMaximized()
{
	maximizeWidget();
}

void CustomBorderContainer::showFullScreen()
{
	if (!isVisible())
		show();
	lastMousePosition = QPoint(-1, -1);
	_isFullscreen = !_isFullscreen;
	if (!_isFullscreen)
	{
		setLayoutMargins();
		setGeometry(normalGeometry);
	}
	else
	{
		normalGeometry = geometry();
		setLayoutMargins();
		setGeometry(qApp->desktop()->screenGeometry(this));
	}
}

void CustomBorderContainer::minimizeWidget()
{
	lastMousePosition = QPoint(-1, -1);
	repaintHeaderButtons();
	showMinimized();
}

void CustomBorderContainer::maximizeWidget()
{
	lastMousePosition = QPoint(-1, -1);
	_isMaximized = !_isMaximized;
	if (!_isMaximized)
	{
		setLayoutMargins();
		setGeometry(normalGeometry);
	}
	else
	{
		normalGeometry = geometry();
		setLayoutMargins();
		setGeometry(qApp->desktop()->availableGeometry(this));
		//updateShape();
	}
}

void CustomBorderContainer::closeWidget()
{
	close();
}

void CustomBorderContainer::restoreWidget()
{
	if (isMinimized())
		showNormal();
	if (_isMaximized)
		maximizeWidget();
	if (_isFullscreen)
		showFullScreen();
}

void CustomBorderContainer::onContainedWidgetDestroyed(QObject* obj)
{
	if (obj == containedWidget)
	{
		containedWidget = NULL;
		deleteLater();
	}
}
