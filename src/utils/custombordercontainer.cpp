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


static void childsRecursive(QObject *object, QWidget *watcher, bool install)
{
	if (object->isWidgetType()) {
		if (install) 
			object->installEventFilter(watcher);
		else 
			object->removeEventFilter(watcher);
		QWidget *widget = qobject_cast<QWidget*>(object);
#if 0
		//Тут надо как-то доработать, чтобы возвращать оригинальную настройку этого параметра при снятии фильтра
#endif
		widget->setAutoFillBackground(true);
	}
	QObjectList children = object->children();
	foreach(QObject *child, children) {
		childsRecursive(child, watcher, install);
	}
}

/**************************************
 * class CustomBorderContainerPrivate *
 **************************************/

CustomBorderContainerPrivate::CustomBorderContainerPrivate(CustomBorderContainer *parent)
{
	p = parent;
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
				button = root.firstChildElement("maximize-button");
				parseHeaderButton(button, maximize);
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
	setDefaultBorder(border);
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
	// zero-sized corner
	corner.width = 10;
	corner.height = 10;
	corner.gradient = new QLinearGradient(0.0, 0.0, 1.0, 0.0);
	corner.gradient->stops().append(QGradientStop(0.0, QColor::fromRgb(0, 0, 0)));
	corner.image = QString::null;
	corner.imageFillingStyle = Stretch;
	corner.radius = 10;
	corner.resizeLeft = corner.resizeRight = corner.resizeTop = corner.resizeBottom = 0;
	corner.resizeWidth = corner.resizeHeight = 5;
}

void CustomBorderContainerPrivate::parseCorner(const QDomElement & cornerElement, Corner & corner)
{
	setDefaultCorner(corner);
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
	setDefaultHeader(header);
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
	title.text = QString::null;
}

void CustomBorderContainerPrivate::parseHeaderTitle(const QDomElement & titleElement, HeaderTitle & title)
{
	setDefaultHeaderTitle(title);
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
	setDefaultWindowIcon(windowIcon);
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
	setDefaultWindowControls(windowControls);
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
	setDefaultHeaderButton(button);
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
	QWidget(0)
{
	containedWidget = NULL;
	setWindowFlags(Qt::FramelessWindowHint);
	setAttribute(Qt::WA_TranslucentBackground);
	containerLayout = new QVBoxLayout;
	setLayout(containerLayout);
	//setAutoFillBackground(true);
	setMouseTracking(true);
	setMinimumWidth(100);
	setMinimumHeight(100);
	resizeBorder = NoneBorder;
	canMove = false;
	myPrivate = new CustomBorderContainerPrivate(this);
	setWidget(widgetToContain);
	QPalette pal = palette();
	pal.setColor(QPalette::Base, Qt::transparent);
	//setPalette(pal);
}

CustomBorderContainer::~CustomBorderContainer()
{
}

void CustomBorderContainer::setWidget(QWidget * widget)
{
	qDebug() << QString::number((int)widget, 16);
	if (containedWidget)
	{
		childsRecursive(containedWidget,this,false);
		containedWidget->removeEventFilter(this);
		containerLayout->removeWidget(containedWidget);
		containedWidget->deleteLater();
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
		connect(containedWidget,SIGNAL(destroyed(QObject *)),SLOT(deleteLater()));
	}
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
	{
		if (resizeBorder != NoneBorder && !canMove)
			setGeometryState(Resizing);
		else if (canMove)
		{
			oldPressPoint = mapToGlobal(event->pos());
			setGeometryState(Moving);
		}
		oldGeometry = geometry();
	}
	QWidget::mousePressEvent(event);
}

void CustomBorderContainer::mouseMoveEvent(QMouseEvent * event)
{
	mouseMove(event->pos());
	QWidget::mouseMoveEvent(event);
}

void CustomBorderContainer::mouseReleaseEvent(QMouseEvent * event)
{
	setGeometryState(None);
	resizeBorder = NoneBorder;
	canMove = false;
	updateCursor();
	QWidget::mouseReleaseEvent(event);
}

void CustomBorderContainer::mouseDoubleClickEvent(QMouseEvent * event)
{
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
	// borders (black for debug)
	drawBorders(&painter);
	// corners (red for debug)
	drawCorners(&painter);

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
	//if (object == containedWidget)
	{
		switch (event->type())
		{
		case QEvent::MouseMove:
			mouseMove(containedWidget->mapToParent(((QMouseEvent*)event)->pos()));
			break;
		case QEvent::Paint:
			{
				QWidget *widget = qobject_cast<QWidget*>(object);
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
				drawCorners(&p);
				return true;
			}
			break;
		default:
			break;
		}
	}
	return QWidget::eventFilter(object, event);
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
	if (oldGeometry.isValid() && (oldGeometry.width() >= minimumWidth()) && (oldGeometry.height() >= minimumHeight()) && (oldGeometry.width() <= maximumWidth()) && (oldGeometry.height() <= maximumHeight()))
	{
		setGeometry(oldGeometry);
		QApplication::flush();
		//updateShape();
	}
}

void CustomBorderContainer::mouseMove(const QPoint & point)
{
	if (geometryState() != None)
	{
		QPoint p = mapToGlobal(point);
		updateGeometry(p);
		return;
	}
	else
	{
		if (geometryState() != Resizing)
		{
			checkResizeCondition(point);
		}
		if (geometryState() != Moving)
		{
			checkMoveCondition(point);
		}
	}
}

bool CustomBorderContainer::pointInBorder(BorderType border, const QPoint & p)
{
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

void CustomBorderContainer::checkResizeCondition(const QPoint & p)
{
	resizeBorder = NoneBorder;
	for (int b = TopLeftCorner; b <= BottomBorder; b++)
		if (pointInBorder((BorderType)b, p))
		{
			resizeBorder = (BorderType)b;
			break;
		}
	updateCursor();
}

void CustomBorderContainer::checkMoveCondition(const QPoint & p)
{
	int lb = myPrivate->left.resizeWidth, tb = myPrivate->top.resizeWidth, rb = myPrivate->right.resizeWidth;
	QRect headerRect(lb, tb, width() - lb - rb, myPrivate->header.height);
	canMove = headerRect.contains(p);
}

void CustomBorderContainer::updateCursor()
{
	switch(resizeBorder)
	{
	case TopLeftCorner:
	case BottomRightCorner:
		setCursor(QCursor(Qt::SizeFDiagCursor));
		break;
	case TopRightCorner:
	case BottomLeftCorner:
		setCursor(QCursor(Qt::SizeBDiagCursor));
		break;
	case LeftBorder:
	case RightBorder:
		setCursor(QCursor(Qt::SizeHorCursor));
		break;
	case TopBorder:
	case BottomBorder:
		setCursor(QCursor(Qt::SizeVerCursor));
		break;
	default:
		setCursor(QCursor(Qt::ArrowCursor));
		break;
	}
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
}

void CustomBorderContainer::drawBorders(QPainter * p)
{
	QRect borderRect;
	borderRect = QRect(0, 0, myPrivate->left.width, height());
	p->fillRect(borderRect, Qt::black);
	borderRect = QRect(width() - myPrivate->right.width, 0, myPrivate->right.width, height());
	p->fillRect(borderRect, Qt::black);
	borderRect = QRect(0, 0, width(), myPrivate->top.width);
	p->fillRect(borderRect, Qt::black);
	borderRect = QRect(0, height() - myPrivate->bottom.width, width(), myPrivate->bottom.width);
	p->fillRect(borderRect, Qt::black);
}

void CustomBorderContainer::drawCorners(QPainter * p)
{
	QRect cornerRect;
	cornerRect = QRect(0, 0, myPrivate->topLeft.width, myPrivate->topLeft.height);
	p->fillRect(cornerRect, Qt::red);
	cornerRect = QRect(width() - myPrivate->topRight.width, 0, myPrivate->topRight.width, myPrivate->topRight.height);
	p->fillRect(cornerRect, Qt::red);
	cornerRect = QRect(0, height() - myPrivate->bottomLeft.height, myPrivate->bottomLeft.width, myPrivate->bottomLeft.height);
	p->fillRect(cornerRect, Qt::red);
	cornerRect = QRect(width() - myPrivate->bottomRight.width, height() - myPrivate->bottomRight.height, myPrivate->bottomRight.width, myPrivate->bottomRight.height);
	p->fillRect(cornerRect, Qt::red);
}
