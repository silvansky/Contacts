#ifndef BORDERSTRUCTS_H
#define BORDERSTRUCTS_H

/*
  NOTE: this file shouldn't be included directly anywhere outside utils. Use customcorderstorage.h.
*/

#include <QGradient>
#include <QString>
#include <QMargins>

enum ImageFillingStyle
{
	Stretch,
	Keep,
	TileHor,
	TileVer,
	Tile
};

// HINT: only linear gradients are supported by now
inline QGradient * copyGradient(const QGradient * g)
{
	QGradient * gradient = NULL;
	if (g)
	{
		switch(g->type())
		{
		case QGradient::LinearGradient:
		{
			const QLinearGradient * lg = static_cast<const QLinearGradient*>(g);
			if (lg)
			{
				gradient = new QLinearGradient(lg->start(), lg->finalStop());
				foreach(QGradientStop stop, lg->stops())
					gradient->stops().append(stop);
			}
			break;
		}
		default:
			break;
		}
	}
	return gradient;
}

struct Border
{
	Border()
	{
		width = 0;
		gradient = NULL;
		image = QString::null;
		imageFillingStyle = Stretch;
		resizeWidth = 0;
		resizeMargin = 0;
	}
	Border(const Border & other)
	{
		width = other.width;
		gradient = copyGradient(other.gradient);
		image = other.image;
		imageFillingStyle = other.imageFillingStyle;
		resizeWidth = other.resizeWidth;
		resizeMargin = other.resizeMargin;
	}
	int width;
	QGradient * gradient;
	QString image;
	ImageFillingStyle imageFillingStyle;
	int resizeWidth;
	int resizeMargin;
};

struct Corner
{
	Corner()
	{
		width = 0;
		height = 0;
		gradient = NULL;
		image = QString::null;
		mask = QString::null;
		imageFillingStyle = Stretch;
		radius = 0;
		resizeLeft = 0;
		resizeRight = 0;
		resizeTop = 0;
		resizeBottom = 0;
		resizeWidth = 0;
		resizeHeight = 0;
	}
	Corner(const Corner & other)
	{
		width = other.width;
		height = other.height;
		gradient = copyGradient(other.gradient);
		image = other.image;
		mask = other.mask;
		imageFillingStyle = other.imageFillingStyle;
		radius = other.radius;
		resizeLeft = other.resizeLeft;
		resizeRight = other.resizeRight;
		resizeTop = other.resizeTop;
		resizeBottom = other.resizeBottom;
		resizeWidth = other.resizeWidth;
		resizeHeight = other.resizeHeight;
	}
	int width;
	int height;
	QGradient * gradient;
	QString image;
	QString mask;
	ImageFillingStyle imageFillingStyle;
	int radius;
	int resizeLeft;
	int resizeRight;
	int resizeTop;
	int resizeBottom;
	int resizeWidth;
	int resizeHeight;
};

struct Header
{
	Header()
	{
		height = 0;
		margins = QMargins();
		gradient = NULL;
		image = QString::null;
		gradientInactive = NULL;
		imageInactive = QString::null;
		imageFillingStyle = Stretch;
		spacing = 0;
		moveHeight = 0;
		moveLeft = 0;
		moveRight = 0;
		moveTop = 0;
	}
	Header(const Header & other)
	{
		height = other.height;
		margins = other.margins;
		gradient = copyGradient(other.gradient);
		image = other.image;
		gradientInactive = copyGradient(other.gradientInactive);
		imageInactive = other.imageInactive;
		imageFillingStyle = other.imageFillingStyle;
		spacing = other.spacing;
		moveHeight = other.moveHeight;
		moveLeft = other.moveLeft;
		moveRight = other.moveRight;
		moveTop = other.moveTop;
	}
	int height;
	QMargins margins;
	QGradient * gradient;
	QString image;
	QGradient * gradientInactive;
	QString imageInactive;
	ImageFillingStyle imageFillingStyle;
	int spacing;
	int moveHeight;
	int moveLeft;
	int moveRight;
	int moveTop;
};

struct HeaderTitle
{
	QString text;
	QColor color;
};

struct WindowIcon
{
	QString icon;
	int width;
	int height;
};

struct WindowControls
{
	int spacing;
};

struct HeaderButton
{
	HeaderButton()
	{
		width = 0;
		height = 0;
		gradientNormal = NULL;
		gradientHover = NULL;
		gradientPressed = NULL;
		gradientDisabled = NULL;
		gradientHoverDisabled = NULL;
		gradientPressedDisabled = NULL;
		imageNormal = QString::null;
		imageHover = QString::null;
		imagePressed = QString::null;
		imageDisabled = QString::null;
		imageHoverDisabled = QString::null;
		imagePressedDisabled = QString::null;
		borderWidth = 0;
		borderRadius = 0;
		borderImage = QString::null;
		borderColor = QColor();
	}
	HeaderButton(const HeaderButton & other)
	{
		width = other.width;
		height = other.height;
		gradientNormal = copyGradient(other.gradientNormal);
		gradientHover = copyGradient(other.gradientHover);
		gradientPressed = copyGradient(other.gradientPressed);
		gradientDisabled = copyGradient(other.gradientDisabled);
		gradientHoverDisabled = copyGradient(other.gradientHoverDisabled);
		gradientPressedDisabled = copyGradient(other.gradientPressedDisabled);
		imageNormal = other.imageNormal;
		imageHover = other.imageHover;
		imagePressed = other.imagePressed;
		imageDisabled = other.imageDisabled;
		imageHoverDisabled = other.imageHoverDisabled;
		imagePressedDisabled = other.imagePressedDisabled;
		borderWidth = other.borderWidth;
		borderRadius = other.borderRadius;
		borderImage = other.borderImage;
		borderColor = other.borderColor;
	}
	int width;
	int height;
	QGradient * gradientNormal;
	QGradient * gradientHover;
	QGradient * gradientPressed;
	QGradient * gradientDisabled;
	QGradient * gradientHoverDisabled;
	QGradient * gradientPressedDisabled;
	QString imageNormal;
	QString imageHover;
	QString imagePressed;
	QString imageDisabled;
	QString imageHoverDisabled;
	QString imagePressedDisabled;
	int borderWidth;
	int borderRadius;
	QString borderImage;
	QColor borderColor;
};

#endif // BORDERSTRUCTS_H
