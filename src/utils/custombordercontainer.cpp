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
#include "iconstorage.h"

// internal functions

static void childsRecursive(QObject *object, QWidget *watcher, bool install)
{
	if (object->isWidgetType())
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
	foreach(QObject *child, children) {
		childsRecursive(child, watcher, install);
	}
}

static void repaintRecursive(QWidget *widget, const QRect & globalRect)
{
	if (widget)
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
				gradient->stops().append(QGradientStop(0.0, QColor(element.attribute("color"))));
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
				QGradientStop stop;
				stop.first = gradientStop.attribute("at").toFloat();
				stop.second = QColor(gradientStop.attribute("color"));
				gradient->stops().append(stop);
				gradientStop = gradientStop.nextSiblingElement("gradient-stop");
			}
			return gradient;
		}
	}
	QLinearGradient * lg = new QLinearGradient(0.0, 0.0, 1.0, 0.0);
	lg->stops().append(QGradientStop(0.0, QColor::fromRgb(0, 0, 0)));
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
		qDebug() << QString("parsing border...");
		QDomElement width = borderElement.firstChildElement("width");
		if (!width.isNull())
		{
			border.width = width.text().toInt();
			qDebug() << border.width;
		}
		QDomElement resizeWidth = borderElement.firstChildElement("resize-width");
		if (!resizeWidth.isNull())
		{
			border.resizeWidth = resizeWidth.text().toInt();
			qDebug() << border.resizeWidth;
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
		qDebug() << "parsing corner";
		QDomElement width = cornerElement.firstChildElement("width");
		if (!width.isNull())
		{
			corner.width = width.text().toInt();
			qDebug() << corner.width;
		}
		QDomElement height = cornerElement.firstChildElement("height");
		if (!height.isNull())
		{
			corner.height = height.text().toInt();
			qDebug() << corner.height;
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
			qDebug() << corner.resizeLeft;
		}
		QDomElement resizeRight = cornerElement.firstChildElement("resize-right");
		if (!resizeRight.isNull())
		{
			corner.resizeRight = resizeRight.text().toInt();
			qDebug() << corner.resizeRight;
		}
		QDomElement resizeTop = cornerElement.firstChildElement("resize-top");
		if (!resizeTop.isNull())
		{
			corner.resizeTop = resizeTop.text().toInt();
			qDebug() << corner.resizeTop;
		}
		QDomElement resizeBottom = cornerElement.firstChildElement("resize-bottom");
		if (!resizeBottom.isNull())
		{
			corner.resizeBottom = resizeBottom.text().toInt();
			qDebug() << corner.resizeBottom;
		}
		QDomElement resizeWidth = cornerElement.firstChildElement("resize-width");
		if (!resizeWidth.isNull())
		{
			corner.resizeWidth = resizeWidth.text().toInt();
			qDebug() << corner.resizeWidth;
		}
		QDomElement resizeHeight = cornerElement.firstChildElement("resize-height");
		if (!resizeHeight.isNull())
		{
			corner.resizeHeight = resizeHeight.text().toInt();
			qDebug() << corner.resizeHeight;
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
	title.text = "Sample title";
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
			button.borderColor = QColor(border.attribute("color"));
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
	setLayoutMargins();
}

CustomBorderContainer::~CustomBorderContainer()
{
	setWidget(NULL);
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
		//containedWidget->setAttribute(Qt::WA_WindowPropagation, false);
		//containedWidget->setParent(this);
		setAttribute(Qt::WA_WindowPropagation, false);
		containedWidget->setAutoFillBackground(true);
		containedWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
		//containedWidget->setAttribute(Qt::WA_WindowPropagation, false);
		containerLayout->addWidget(containedWidget);
		//containedWidget->installEventFilter(this);
		childsRecursive(containedWidget,this,true);
		containedWidget->setMouseTracking(true);
		QPalette pal = containedWidget->palette();
		pal.setColor(QPalette::Base, Qt::white);
		//containedWidget->setPalette(pal);
		containedWidget->setAttribute(Qt::WA_WindowPropagation, false);
		//adjustSize();
		setMinimumSize(containedWidget->minimumSize());
		setWindowTitle(containedWidget->windowTitle());
		//connect(containedWidget,SIGNAL(destroyed(QObject *)),SLOT(deleteLater()));
	}
}

QWidget * CustomBorderContainer::releaseWidget()
{
	if (containedWidget)
	{
		//disconnect(containedWidget, SIGNAL(destroyed(QObject*)), this, SLOT(deleteLater()));
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
	repaint();
	setLayoutMargins();
	updateShape();
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
	mouseRelease(event->pos(), this);
	QWidget::mouseReleaseEvent(event);
}

void CustomBorderContainer::mouseDoubleClickEvent(QMouseEvent * event)
{
	mouseDoubleClick(event->pos(), this);
	QWidget::mouseDoubleClickEvent(event);
}

void CustomBorderContainer::moveEvent(QMoveEvent * event)
{
	QWidget::moveEvent(event);
}

void CustomBorderContainer::paintEvent(QPaintEvent * event)
{
	QWidget::paintEvent(event);

	// painter
	QPainter painter;
	painter.begin(this);
	painter.setClipRegion(mask());
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
	switch (event->type())
	{
	case QEvent::MouseMove:
		mouseMove(((QMouseEvent*)event)->globalPos(), widget);
		break;
	case QEvent::MouseButtonPress:
		if (((QMouseEvent*)event)->button() == Qt::LeftButton)
			mousePress(((QMouseEvent*)event)->pos(), widget);
		break;
	case QEvent::MouseButtonRelease:
		mouseRelease(((QMouseEvent*)event)->pos(), widget);
		break;
	case QEvent::Paint:
	{
		//widget->setAutoFillBackground(false);
		object->removeEventFilter(this);
		QApplication::sendEvent(object,event);
		object->installEventFilter(this);
		//widget->setAutoFillBackground(true);

		QPoint point = widget->pos();
		while(widget && (widget->parentWidget() != this)) {
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
	default:
		break;
	}
	return QWidget::eventFilter(object, event);
}

void CustomBorderContainer::init()
{
	// vars
	containedWidget = NULL;
	resizeBorder = NoneBorder;
	canMove = false;
	buttonsFlags = MinimizeVisible | MaximizeVisible | CloseVisible | MinimizeEnabled | MaximizeEnabled | CloseEnabled;
	// window props
	setWindowFlags(Qt::FramelessWindowHint);
	setAttribute(Qt::WA_TranslucentBackground);
	setMouseTracking(true);
	// layout
	containerLayout = new QVBoxLayout;
	containerLayout->setContentsMargins(0, 0, 0, 0);
	setLayout(containerLayout);
	setMinimumWidth(100);
	setMinimumHeight(100);
	setGeometryState(None);
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
		break;
	}
	if (oldGeometry.isValid())
	{
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

void CustomBorderContainer::mouseMove(const QPoint & point, QWidget * widget)
{
	lastMousePosition = mapFromGlobal(point);
	if (geometryState() != None)
	{
		//QPoint p = widget->mapToGlobal(point);
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
			int lb = myPrivate->left.width, tb = myPrivate->top.width, rb = myPrivate->right.width;
			QRect headerRect(lb, tb, width() - lb - rb, myPrivate->header.moveHeight);
			repaint(headerRect);
			headerRect.moveTopLeft(mapToGlobal(headerRect.topLeft()));
			repaintRecursive(containedWidget, headerRect);
		}
	}
}

void CustomBorderContainer::mousePress(const QPoint & p, QWidget * widget)
{
	if (resizeBorder != NoneBorder)
		setGeometryState(Resizing);
	else if (canMove)
	{
		oldPressPoint = widget->mapToGlobal(p);
		setGeometryState(Moving);
	}
	oldGeometry = geometry();
}

void CustomBorderContainer::mouseRelease(const QPoint & p, QWidget * widget)
{
	setGeometryState(None);
	resizeBorder = NoneBorder;
	canMove = false;
	updateCursor(widget);
}

void CustomBorderContainer::mouseDoubleClick(const QPoint & p, QWidget * widget)
{

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
	int lb = myPrivate->left.resizeWidth, tb = myPrivate->top.resizeWidth, rb = myPrivate->right.resizeWidth;
	QRect headerRect(lb, tb, width() - lb - rb, myPrivate->header.moveHeight);
	canMove = headerRect.contains(p);
}

void CustomBorderContainer::updateCursor(QWidget * widget)
{
	if (!widget)
		widget = this;
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
	default:
		newCursor.setShape((Qt::CursorShape)widget->property("defaultCursorShape").toInt());
		break;
	}
	widget->setCursor(newCursor);
}

void CustomBorderContainer::updateShape()
{
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
	circle = QRegion(0, 0, 2 * rad, 2 * rad, QRegion::Ellipse);
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
	circle = QRegion(0, h - 2 * rad - 1, 2 * rad, 2 * rad, QRegion::Ellipse);
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
}

void CustomBorderContainer::setLayoutMargins()
{
	layout()->setContentsMargins(myPrivate->left.width, myPrivate->top.width + myPrivate->header.height, myPrivate->right.width, myPrivate->bottom.width);
}

void CustomBorderContainer::drawHeader(QPainter * p)
{
	QPainterPath path;
	QRect headerRect = QRect(myPrivate->left.width, myPrivate->top.width, width() - myPrivate->right.width, myPrivate->header.height);
	path.addRegion(mask() & headerRect);
	QLinearGradient lg(headerRect.left(), headerRect.top(), headerRect.left(), headerRect.bottom());
	lg.setColorAt(0.0, QColor(0, 255, 0, 50));
	lg.setColorAt(0.5, QColor(150, 255, 0, 150));
	lg.setColorAt(1.0, QColor(0, 255, 0, 50));
	p->fillPath(path, QBrush(lg));
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
	// drawing all buttons for debug
	int numButtons = 3;
	int startX = width() - myPrivate->right.width - myPrivate->header.margins.right() - (numButtons - 1) * myPrivate->controls.spacing - myPrivate->minimize.width - myPrivate->maximize.width - myPrivate->close.width;
	int startY = myPrivate->top.width + myPrivate->header.margins.top();
	QRect buttonRect;
	HeaderButtonState state;
	p->save();
	p->translate(startX, startY);
	// minimize button
	buttonRect = QRect(startX, startY, myPrivate->minimize.width, myPrivate->minimize.height);
	state = buttonRect.contains(lastMousePosition) ? NormalHover : Normal;
	drawButton(myPrivate->minimize, p, state);
	// maximize button
	buttonRect = QRect(startX + myPrivate->minimize.width + myPrivate->controls.spacing, startY, myPrivate->maximize.width, myPrivate->maximize.height);
	state = buttonRect.contains(lastMousePosition) ? NormalHover : Normal;
	p->translate(myPrivate->minimize.width + myPrivate->controls.spacing, 0);
	drawButton(myPrivate->maximize, p, state);
	// close button
	buttonRect = QRect(startX + myPrivate->minimize.width + myPrivate->maximize.width + 2 * myPrivate->controls.spacing, startY, myPrivate->close.width, myPrivate->close.height);
	state = buttonRect.contains(lastMousePosition) ? NormalHover : Normal;
	p->translate(myPrivate->maximize.width + myPrivate->controls.spacing, 0);
	drawButton(myPrivate->close, p, state);
	p->restore();
}

void CustomBorderContainer::drawIcon(QPainter * p)
{
	// TODO: load and draw icon here
}

void CustomBorderContainer::drawTitle(QPainter * p)
{
	QTextDocument doc;
	doc.setHtml(QString("<font size=+1 color=%1><b>%2</b></font>").arg(myPrivate->title.color.name(), windowTitle()));
	// center title in header rect
	QRect headerRect(0, 0, width(), myPrivate->header.height);
	p->save();
	p->setClipRect(headerRect);
	qreal dx, dy;
	dx = (headerRect.width() - doc.size().width()) / 2;
	dy = (headerRect.height() - doc.size().height()) / 2;
	if (dx < 0.0)
		dx = 0.0;
	if (dy < 0.0)
		dy = 0.0;
	p->translate(dx, dy);
	doc.drawContents(p, QRectF(headerRect));
	p->restore();
}

void CustomBorderContainer::drawBorders(QPainter * p)
{
	QRect borderRect;
	borderRect = QRect(0, 0, myPrivate->left.width, height());
	p->fillRect(borderRect, QBrush(*(myPrivate->left.gradient)));
	borderRect = QRect(width() - myPrivate->right.width, 0, myPrivate->right.width, height());
	p->fillRect(borderRect, Qt::black);
	borderRect = QRect(0, 0, width(), myPrivate->top.width);
	p->fillRect(borderRect, Qt::black);
	borderRect = QRect(0, height() - myPrivate->bottom.width, width(), myPrivate->bottom.width);
	p->fillRect(borderRect, Qt::black);
}

void CustomBorderContainer::drawCorners(QPainter * p)
{
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
