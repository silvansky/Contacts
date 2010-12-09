#ifndef BORDERSTRUCTS_H
#define BORDERSTRUCTS_H

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

struct Border
{
	int width;
	QGradient * gradient;
	QString image;
	ImageFillingStyle imageFillingStyle;
	int resizeWidth;
};

struct Corner
{
	int width;
	int height;
	QGradient * gradient;
	QString image;
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
	int height;
	QMargins margins;
	QGradient * gradient;
	QString image;
	QGradient * gradientInactive;
	QString imageInactive;
	ImageFillingStyle imageFillingStyle;
	int spacing;
	int moveHeight;
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
