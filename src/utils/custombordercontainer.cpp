#include "custombordercontainer.h"
#include <QEvent>
#include <QMouseEvent>
#include <QVBoxLayout>
#include "custombordercontainer_p.h"
#include <QDebug>
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
#include "iconstorage.h"

// internal functions

static void childsRecursive(QObject *object, QWidget *watcher, bool install)
{
	// ensure object is widget but not a menu
	if (object->isWidgetType() && !qobject_cast<QMenu*>(object))
	{
		if (install)
			object->installEventFilter(watcher);
		else
			object->removeEventFilter(watcher);
		QWidget * widget = qobject_cast<QWidget*>(object);
#if 0
		//Тут надо как-то доработать, чтобы возвращать оригинальную настройку этого параметра при снятии фильтра
#endif
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

CustomBorderContainerPrivate::CustomBorderContainerPrivate(const CustomBorderContainerPrivate& other)
{
	setAllDefaults();
	topLeft = other.topLeft;
	topRight = other.topRight;
	bottomLeft = other.bottomLeft;
	bottomRight = other.bottomRight;
	left = other.left;
	right = other.right;
	top = other.top;
	bottom = other.bottom;
	header = other.header;
	title = other.title;
	icon = other.icon;
	controls = other.controls;
	minimize = other.minimize;
	maximize = other.maximize;
	close = other.close;
	headerButtons = other.headerButtons;
	p = NULL;
}

CustomBorderContainerPrivate::~CustomBorderContainerPrivate()
{

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
			}
			else
			{
				qDebug() << QString("Can\'t parse file %1! Unknown root element.").arg(fileName);
			}
		}
		else
		{
			qDebug() << QString("Can\'t parse file %1!").arg(fileName);
		}
	}
	else
	{
		qDebug() << QString("Can\'t open file %1!").arg(fileName);
	}
}

void CustomBorderContainerPrivate::setAllDefaults()
{
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
		qDebug() << "Can\'t parse color: " << name;
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
	border.image = QString::null;
	border.imageFillingStyle = Stretch;
	border.gradient = new QLinearGradient(0.0, 0.0, 1.0, 0.0);
	border.gradient->stops().append(QGradientStop(0.0, QColor::fromRgb(0, 0, 0)));
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
	corner.gradient = new QLinearGradient(0.0, 0.0, 1.0, 0.0);
	corner.gradient->stops().append(QGradientStop(0.0, QColor::fromRgb(0, 0, 0)));
	corner.image = QString::null;
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
	header.gradient = new QLinearGradient(0.0, 0.0, 1.0, 0.0);
	header.gradient->stops().append(QGradientStop(0.0, QColor::fromRgb(0, 0, 0)));
	header.gradient->stops().append(QGradientStop(1.0, QColor::fromRgb(100, 100, 100)));
	header.gradientInactive = new QLinearGradient(0.0, 0.0, 1.0, 0.0);
	header.gradientInactive->stops().append(QGradientStop(0.0, QColor::fromRgb(50, 50, 50)));
	header.gradientInactive->stops().append(QGradientStop(1.0, QColor::fromRgb(100, 100, 100)));
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
	QLinearGradient * g = new QLinearGradient(0.0, 0.0, 0.0, 1.0);
	g->stops().append(QGradientStop(0.0, QColor::fromRgb(100, 100, 100)));
	g->stops().append(QGradientStop(1.0, QColor::fromRgb(50, 50, 50)));
	button.gradientNormal = g;
	g = new QLinearGradient(0.0, 0.0, 0.0, 1.0);
	g->stops().append(QGradientStop(0.0, QColor::fromRgb(100, 100, 100)));
	g->stops().append(QGradientStop(1.0, QColor::fromRgb(80, 80, 80)));
	button.gradientHover = g;
	g = new QLinearGradient(0.0, 0.0, 0.0, 1.0);
	g->stops().append(QGradientStop(0.0, QColor::fromRgb(120, 120, 120)));
	g->stops().append(QGradientStop(1.0, QColor::fromRgb(100, 100, 100)));
	button.gradientPressed = g;
	g = new QLinearGradient(0.0, 0.0, 0.0, 1.0);
	g->stops().append(QGradientStop(0.0, QColor::fromRgb(80, 80, 80)));
	g->stops().append(QGradientStop(1.0, QColor::fromRgb(30, 30, 30)));
	button.gradientDisabled = g;
	g = new QLinearGradient(0.0, 0.0, 0.0, 1.0);
	g->stops().append(QGradientStop(0.0, QColor::fromRgb(80, 80, 80)));
	g->stops().append(QGradientStop(1.0, QColor::fromRgb(30, 30, 30)));
	button.gradientHoverDisabled = g;
	g = new QLinearGradient(0.0, 0.0, 0.0, 1.0);
	g->stops().append(QGradientStop(0.0, QColor::fromRgb(80, 80, 80)));
	g->stops().append(QGradientStop(1.0, QColor::fromRgb(50, 50, 50)));
	button.gradientPressedDisabled = g;
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
	myPrivate = new CustomBorderContainerPrivate(this);
	setWidget(widgetToContain);
}

CustomBorderContainer::CustomBorderContainer(const CustomBorderContainerPrivate &style) :
	QWidget(NULL)
{
	init();
	setWidget(NULL);
	myPrivate = new CustomBorderContainerPrivate(style);
	myPrivate->p = this;
	updateIcons();
	setLayoutMargins();
}

CustomBorderContainer::~CustomBorderContainer()
{
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
		containedWidget = widget;
		setAttribute(Qt::WA_WindowPropagation, false);
		containedWidget->setAutoFillBackground(true);
		containedWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
		containerLayout->addWidget(containedWidget);
		childsRecursive(containedWidget,this,true);
		containedWidget->setMouseTracking(true);
		containedWidget->setAttribute(Qt::WA_WindowPropagation, false);
		setMinimumSize(containedWidget->minimumSize());
		//setMinimumSize(containedWidget->minimumSizeHint());
		setWindowTitle(containedWidget->windowTitle());
	}
}

QWidget * CustomBorderContainer::releaseWidget()
{
	if (containedWidget)
	{
		childsRecursive(containedWidget,this,false);
		containedWidget->removeEventFilter(this);
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
	myPrivate->parseFile(fileName);
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
	updateShape();
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
	if (!mask().isEmpty())
		painter.setClipRegion(mask());
	else
		painter.setClipRect(geometry());
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
	return QWidget::event(evt);
}

bool CustomBorderContainer::eventFilter(QObject * object, QEvent * event)
{
	QWidget *widget = qobject_cast<QWidget*>(object);
	// TODO: make mousePress return bool (true if resize event occurs, then no need to call QWidget::eventFilter())
	bool handled = false;
	switch (event->type())
	{
	case QEvent::MouseMove:
		mouseMove(((QMouseEvent*)event)->globalPos(), widget);
		break;
	case QEvent::MouseButtonPress:
		if (((QMouseEvent*)event)->button() == Qt::LeftButton)
			handled = mousePress(((QMouseEvent*)event)->pos(), widget);
		qDebug() << "handled = " << handled << " " << widget->objectName() << " of class " << widget->metaObject()->className();
		break;
	case QEvent::MouseButtonRelease:
		mouseRelease(((QMouseEvent*)event)->pos(), widget, ((QMouseEvent*)event)->button());
		break;
	case QEvent::MouseButtonDblClick:
		if (((QMouseEvent*)event)->button() == Qt::LeftButton)
			mouseDoubleClick(((QMouseEvent*)event)->pos(), widget);
		break;
	case QEvent::Paint:
	{
		//widget->setAutoFillBackground(false);
		object->removeEventFilter(this);
		QApplication::sendEvent(object,event);
		object->installEventFilter(this);
		//widget->setAutoFillBackground(true);

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
		//p.translate(containedWidget->mapFromParent(QPoint(0, 0)));
		//p.fillRect(0, 0, 100, 100, QColor(255, 255, 0, 255));
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
	default:
		break;
	}
	handled = (handled || (geometryState() != None)) && (event->type() != QEvent::Resize);
	return handled ? handled : QWidget::eventFilter(object, event);
}

void CustomBorderContainer::init()
{
	// vars
	containedWidget = NULL;
	resizeBorder = NoneBorder;
	canMove = false;
	movable = true;
	resizable = true;
	buttonsFlags = MinimizeVisible | MaximizeVisible | CloseVisible | MinimizeEnabled | MaximizeEnabled | CloseEnabled;
	pressedHeaderButton = NoneButton;
	isMaximized = false;
	// window menu
	windowMenu = new Menu();
	minimizeAction = new Action();
	maximizeAction = new Action();
	closeAction = new Action();
	minimizeAction->setText(tr("Minimize"));
	minimizeAction->setIcon(style()->standardIcon(QStyle::SP_TitleBarMinButton));
	maximizeAction->setText(tr("Maximize"));
	maximizeAction->setIcon(style()->standardIcon(QStyle::SP_TitleBarMaxButton));
	closeAction->setText(tr("Close"));
	closeAction->setIcon(style()->standardIcon(QStyle::SP_TitleBarCloseButton));
	windowMenu->addAction(minimizeAction);
	windowMenu->addAction(maximizeAction);
	windowMenu->addAction(closeAction);
	maximizeAction->setCheckable(true);
	connect(minimizeAction, SIGNAL(triggered()), SIGNAL(minimizeClicked()));
	connect(maximizeAction, SIGNAL(triggered()), SIGNAL(maximizeClicked()));
	connect(closeAction, SIGNAL(triggered()), SIGNAL(closeClicked()));
	// window props
	setWindowFlags(Qt::FramelessWindowHint);
	setAttribute(Qt::WA_TranslucentBackground);
	setFocusPolicy(Qt::NoFocus);
	setMouseTracking(true);
	// layout
	containerLayout = new QVBoxLayout;
	containerLayout->setContentsMargins(0, 0, 0, 0);
	setLayout(containerLayout);
	setMinimumWidth(100);
	setMinimumHeight(100);
	setGeometryState(None);
	// connections
	connect(this, SIGNAL(minimizeClicked()), SLOT(minimizeWidget()));
	connect(this, SIGNAL(maximizeClicked()), SLOT(maximizeWidget()));
	connect(this, SIGNAL(closeClicked()), SLOT(closeWidget()));
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
			oldGeometry.setTopLeft(p);
			break;
		case TopRightCorner:
			oldGeometry.setTopRight(p);
			break;
		case BottomLeftCorner:
			oldGeometry.setBottomLeft(p);
			break;
		case BottomRightCorner:
			oldGeometry.setBottomRight(p);
			break;
		case LeftBorder:
			oldGeometry.setLeft(p.x());
			break;
		case RightBorder:
			oldGeometry.setRight(p.x());
			break;
		case TopBorder:
			oldGeometry.setTop(p.y());
			break;
		case BottomBorder:
			oldGeometry.setBottom(p.y());
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
		// HACK: can't understand why minimumSize is set to (0, 0) sometimes...
		if (minimumSize().width() == 0)
		{
			qDebug() << "hacking minimum size...";
			setMinimumSize(containedWidget->minimumSizeHint());
			qDebug() << minimumSize();
		}
		// left border is changing
		if (resizeBorder == LeftBorder || resizeBorder == TopLeftCorner || resizeBorder == BottomLeftCorner)
		{
			if (oldGeometry.width() < minimumWidth())
				oldGeometry.setLeft(oldGeometry.right() - minimumWidth());
			if (oldGeometry.width() > maximumWidth())
				oldGeometry.setLeft(oldGeometry.right() - maximumWidth());
		}
		// top border is changing
		if (resizeBorder == TopBorder || resizeBorder == TopLeftCorner || resizeBorder == TopRightCorner)
		{
			if (oldGeometry.height() < minimumHeight())
				oldGeometry.setTop(oldGeometry.bottom() - minimumHeight());
			if (oldGeometry.height() > maximumHeight())
				oldGeometry.setTop(oldGeometry.bottom() - maximumHeight());
		}
		// right border is changing
		if (resizeBorder == RightBorder || resizeBorder == TopRightCorner || resizeBorder == BottomRightCorner)
		{
			if (oldGeometry.width() < minimumWidth())
				oldGeometry.setWidth(minimumWidth());
			if (oldGeometry.width() > maximumWidth())
				oldGeometry.setHeight(maximumHeight());
		}
		// bottom border is changing
		if (resizeBorder == BottomBorder || resizeBorder == BottomLeftCorner || resizeBorder == BottomRightCorner)
		{
			if (oldGeometry.height() < minimumHeight())
				oldGeometry.setHeight(minimumHeight());
			if (oldGeometry.height() > maximumHeight())
				oldGeometry.setHeight(maximumHeight());
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
	int numButtons = headerButtonsCount();
	int startX = width() - (isMaximized ? 0 : myPrivate->right.width) - myPrivate->header.margins.right() - (numButtons - 1) * myPrivate->controls.spacing;
	startX -= (isMinimizeButtonVisible() ? myPrivate->minimize.width : 0) + (isMaximizeButtonVisible() ? myPrivate->maximize.width : 0) + (isCloseButtonVisible() ? myPrivate->close.width : 0);
	int startY = (isMaximized ? 0 : myPrivate->top.width) + myPrivate->header.margins.top();
	int dx = 0;
	QRect buttonRect;
	switch (button)
	{
	case MinimizeButton:
		if (isMinimizeButtonVisible())
			buttonRect = QRect(startX, startY, myPrivate->minimize.width, myPrivate->minimize.height);
		break;
	case MaximizeButton:
		if (isMaximizeButtonVisible())
		{
			if (isMinimizeButtonVisible())
				dx = myPrivate->minimize.width + myPrivate->controls.spacing;
			buttonRect = QRect(startX + dx, startY, myPrivate->maximize.width, myPrivate->maximize.height);
		}
		break;
	case CloseButton:
		if (isCloseButtonVisible())
		{
			if (isMinimizeButtonVisible())
				dx = myPrivate->minimize.width + myPrivate->controls.spacing;
			if (isMaximizeButtonVisible())
				dx += myPrivate->maximize.width + myPrivate->controls.spacing;
			buttonRect = QRect(startX + dx, startY, myPrivate->close.width, myPrivate->close.height);
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
	int tb = (isMaximized ? 0 : myPrivate->top.width) + myPrivate->header.margins.top(), rb = (isMaximized ? 0 : myPrivate->right.width) + myPrivate->header.margins.right();
	int numButtons = headerButtonsCount();
	int lb = width() - (isMaximized ? 0 : myPrivate->right.width) - myPrivate->header.margins.right() - (numButtons - 1) * myPrivate->controls.spacing;
	lb -= (isMinimizeButtonVisible() ? myPrivate->minimize.width : 0) + (isMaximizeButtonVisible() ? myPrivate->maximize.width : 0) + (isCloseButtonVisible() ? myPrivate->close.width : 0);
	int h = qMax(qMax((isMinimizeButtonVisible() ? myPrivate->minimize.height: 0), (isMaximizeButtonVisible() ? myPrivate->maximize.height: 0)), (isCloseButtonVisible() ? myPrivate->close.height: 0)) + 1;
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
	int x = (isMaximized ? 0 : myPrivate->left.width) + myPrivate->header.margins.left();
	int y = (isMaximized ? 0 : myPrivate->top.width) + myPrivate->header.margins.top();
	return QRect(x, y, myPrivate->icon.width, myPrivate->icon.height);
}

void CustomBorderContainer::showWindowMenu(const QPoint & p)
{
	minimizeAction->setEnabled(isMinimizeButtonVisible() && isMinimizeButtonEnabled());
	maximizeAction->setEnabled(isMaximizeButtonVisible() && isMaximizeButtonEnabled());
	closeAction->setEnabled(isCloseButtonVisible() && isCloseButtonEnabled());
	windowMenu->popup(p);
}

void CustomBorderContainer::mouseMove(const QPoint & point, QWidget * widget)
{
	Q_UNUSED(widget)
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
		else if (canMove)
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
	else if (button == Qt::RightButton)
	{
		if (headerMoveRect().contains(mapFromWidget(widget, p)) && !headerButtonsRect().contains(mapFromWidget(widget, p)))
		{
			showWindowMenu(widget->mapToGlobal(p));
		}
	}
}

void CustomBorderContainer::mouseDoubleClick(const QPoint & p, QWidget * widget)
{
	if (windowIconRect().contains(mapFromWidget(widget, p)) && headerRect().contains(mapFromWidget(widget, p)))
		emit closeClicked();
	else if (headerMoveRect().contains(mapFromWidget(widget, p)) && headerButtonUnderMouse() == NoneButton && isMaximizeButtonVisible() && isMaximizeButtonEnabled())
		emit maximizeClicked();
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
		c = myPrivate->topLeft;
		x = c.resizeLeft;
		y = c.resizeTop;
		w = c.resizeWidth;
		h = c.resizeHeight;
		cornerRect = QRect(x, y, w, h);
		return cornerRect.contains(p);
		break;
	case TopRightCorner:
		c = myPrivate->topRight;
		x = width() - c.resizeRight - c.resizeWidth;
		y = c.resizeTop;
		w = c.resizeWidth;
		h = c.resizeHeight;
		cornerRect = QRect(x, y, w, h);
		return cornerRect.contains(p);
		break;
	case BottomLeftCorner:
		c = myPrivate->bottomLeft;
		x = c.resizeLeft;
		y = height() - c.resizeBottom - c.resizeHeight;
		w = c.resizeWidth;
		h = c.resizeHeight;
		cornerRect = QRect(x, y, w, h);
		return cornerRect.contains(p);
		break;
	case BottomRightCorner:
		c = myPrivate->bottomRight;
		x = width() - c.resizeRight - c.resizeWidth;
		y = height() - c.resizeBottom - c.resizeHeight;
		w = c.resizeWidth;
		h = c.resizeHeight;
		cornerRect = QRect(x, y, w, h);
		return cornerRect.contains(p);
		break;
	case LeftBorder:
		b = myPrivate->left;
		return (p.x() <= b.resizeWidth);
		break;
	case RightBorder:
		b = myPrivate->right;
		return (p.x() > width() - b.resizeWidth);
		break;
	case TopBorder:
		b = myPrivate->top;
		return (p.y() < b.resizeWidth);
		break;
	case BottomBorder:
		b = myPrivate->bottom;
		return (p.y() > height() - b.resizeWidth);
		break;
	default:
		break;
	}
	return false;
}

bool CustomBorderContainer::pointInHeader(const QPoint & p)
{
	int lb = myPrivate->left.resizeWidth, tb = myPrivate->top.resizeWidth, rb = myPrivate->right.resizeWidth;
	QRect headerRect(lb, tb, width() - lb - rb, myPrivate->header.moveHeight);
	return headerRect.contains(p);
}

void CustomBorderContainer::checkResizeCondition(const QPoint & p)
{
	resizeBorder = NoneBorder;
	if (!isMaximized && resizable)
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
	if (isMaximized || !movable)
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
	if (!isMaximized)
	{
// uncomment this define to enable old version of this method
//#define QREGION_ROUNDING
#ifdef QREGION_ROUNDING
		// base rect
		QRegion shape(0, 0, width(), height());
		QRegion rect, circle;
		int rad;
		QRect g = geometry();
		int w = g.width();
		int h = g.height();
		// for each corner we substract rect and add circle
		// top-left
		rad = myPrivate->topLeft.radius;
		rect = QRegion(0, 0, rad, rad);
		circle = QRegion(0, 0, 2 * rad + 1, 2 * rad + 1, QRegion::Ellipse);
		shape -= rect;
		shape |= circle;
		// top-right
		rad = myPrivate->topRight.radius;
		rect = QRegion(w - rad, 0, rad, rad);
		circle = QRegion(w - 2 * rad - 1, 0, 2 * rad, 2 * rad, QRegion::Ellipse);
		shape -= rect;
		shape |= circle;
		// bottom-left
		rad = myPrivate->bottomLeft.radius;
		rect = QRegion(0, h - rad, rad, rad);
		circle = QRegion(1, h - 2 * rad - 1, 2 * rad, 2 * rad, QRegion::Ellipse);
		shape -= rect;
		shape |= circle;
		// bottom-right
		rad = myPrivate->bottomRight.radius;
		rect = QRegion(w - rad, h - rad, rad, rad);
		circle = QRegion(w - 2 * rad - 1, h - 2 * rad - 1, 2 * rad, 2 * rad, QRegion::Ellipse);
		shape -= rect;
		shape |= circle;
		// setting mask
		setMask(shape);
#else
		QPixmap pixmap(geometry().size());
		pixmap.fill(Qt::transparent);
		QPainter p(&pixmap);
		p.setBrush(QBrush(Qt::black));
		p.setPen(QPen());
		QRect rect;
		QRect g = geometry();
		int w = g.width();
		int h = g.height();
		int rad;
		// top-left
		rad = myPrivate->topLeft.radius;
		rect = QRect(0, 0, w / 2 + rad, h / 2 + rad);
		p.drawRoundedRect(rect, rad, rad);
		// top-right
		rad = myPrivate->topRight.radius;
		rect = QRect(w / 2 - rad - 1, 0, w / 2 + rad, h / 2 + rad);
		p.drawRoundedRect(rect, rad, rad);
		// bottom-left
		rad = myPrivate->bottomLeft.radius;
		rect = QRect(0, h / 2 - rad - 1, w / 2 + rad, h / 2 + rad);
		p.drawRoundedRect(rect, rad, rad);
		// bottom-right
		rad = myPrivate->bottomRight.radius;
		rect = QRect(w / 2 - rad - 1, h / 2 - rad - 1, w / 2 + rad, h / 2 + rad);
		p.drawRoundedRect(rect, rad, rad);
		p.end();
		setMask(pixmap.mask());
#endif
	}
	else
	{
		// clearing window mask if it is maximized
		clearMask();
	}
}

void CustomBorderContainer::updateIcons()
{
	setWindowIcon(loadIcon(myPrivate->icon.icon));
	minimizeAction->setIcon(loadIcon(myPrivate->minimize.imageNormal));
	maximizeAction->setIcon(loadIcon(myPrivate->maximize.imageNormal));
	closeAction->setIcon(loadIcon(myPrivate->close.imageNormal));
}

void CustomBorderContainer::setLayoutMargins()
{
	if (isMaximized)
		layout()->setContentsMargins(0, myPrivate->header.height, 0, 0);
	else
		layout()->setContentsMargins(myPrivate->left.width, myPrivate->top.width + myPrivate->header.height, myPrivate->right.width, myPrivate->bottom.width);
}

QRect CustomBorderContainer::headerRect() const
{
	if (isMaximized)
		return QRect(0, 0, width(), myPrivate->header.height);
	else
		return QRect(myPrivate->left.width, myPrivate->top.width, width() - myPrivate->right.width, myPrivate->header.height);
}

QRect CustomBorderContainer::headerMoveRect() const
{
	if (isMaximized)
		return QRect(myPrivate->header.moveLeft, myPrivate->header.moveTop, width() - myPrivate->header.moveRight, myPrivate->header.moveHeight);
	else
		return QRect(myPrivate->left.width + myPrivate->header.moveLeft, myPrivate->top.width + myPrivate->header.moveTop, width() - myPrivate->right.width - myPrivate->header.moveRight, myPrivate->header.moveHeight);
}

void CustomBorderContainer::drawHeader(QPainter * p)
{
	QPainterPath path;
	if (!mask().isEmpty())
		path.addRegion(mask() & headerRect());
	else
		path.addRegion(headerRect());
	p->fillPath(path, QBrush(*(myPrivate->header.gradient)));
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
		drawButton(myPrivate->minimize, p, state);
		p->restore();
	}
	// maximize button
	if (isMaximizeButtonVisible())
	{
		p->save();
		p->translate(headerButtonRect(MaximizeButton).topLeft());
		state = maximizeButtonUnderMouse() ? NormalHover : Normal;
		drawButton(myPrivate->maximize, p, state);
		p->restore();
	}
	// close button
	if (isCloseButtonVisible())
	{
		p->save();
		p->translate(headerButtonRect(CloseButton).topLeft());
		state = closeButtonUnderMouse() ? NormalHover : Normal;
		drawButton(myPrivate->close, p, state);
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
	doc.setHtml(QString("<font size=+1 color=%1><b>%2</b></font>").arg(myPrivate->title.color.name(), windowTitle()));
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
	if (!isMaximized)
	{
		QRect borderRect;
		// left
		borderRect = QRect(0, myPrivate->topLeft.height, myPrivate->left.width, height() - myPrivate->bottomLeft.height);
		p->fillRect(borderRect, QBrush(*(myPrivate->left.gradient)));
		// right
		borderRect = QRect(width() - myPrivate->right.width, myPrivate->topRight.height, myPrivate->right.width, height() - myPrivate->bottomRight.height);
		p->fillRect(borderRect, QBrush(*(myPrivate->right.gradient)));
		// top
		borderRect = QRect(myPrivate->topLeft.width, 0, width() - myPrivate->topRight.width, myPrivate->top.width);
		p->fillRect(borderRect, QBrush(*(myPrivate->top.gradient)));
		// bottom
		borderRect = QRect(myPrivate->bottomLeft.width, height() - myPrivate->bottom.width, width() - myPrivate->bottomRight.width, myPrivate->bottom.width);
		p->fillRect(borderRect, QBrush(*(myPrivate->bottom.gradient)));
	}
}

void CustomBorderContainer::drawCorners(QPainter * p)
{
	Q_UNUSED(p)
	/*
 QRect cornerRect;
 cornerRect = QRect(0, 0, myPrivate->topLeft.width, myPrivate->topLeft.height);
 p->fillRect(cornerRect, Qt::red);
 cornerRect = QRect(width() - myPrivate->topRight.width, 0, myPrivate->topRight.width, myPrivate->topRight.height);
 p->fillRect(cornerRect, Qt::red);
 cornerRect = QRect(0, height() - myPrivate->bottomLeft.height, myPrivate->bottomLeft.width, myPrivate->bottomLeft.height);
 p->fillRect(cornerRect, Qt::red);
 cornerRect = QRect(width() - myPrivate->bottomRight.width, height() - myPrivate->bottomRight.height, myPrivate->bottomRight.width, myPrivate->bottomRight.height);
 p->fillRect(cornerRect, Qt::red);
 */
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

void CustomBorderContainer::minimizeWidget()
{
	lastMousePosition = QPoint(-1, -1);
	repaintHeaderButtons();
	showMinimized();
}

void CustomBorderContainer::maximizeWidget()
{
	lastMousePosition = QPoint(-1, -1);
	isMaximized = !isMaximized;
	if (!isMaximized)
	{
		setLayoutMargins();
		setGeometry(normalGeometry);
	}
	else
	{
		normalGeometry = geometry();
		setLayoutMargins();
		setGeometry(qApp->desktop()->availableGeometry(this));
	}
	maximizeAction->setChecked(isMaximized);
}

void CustomBorderContainer::closeWidget()
{
	close();
}
