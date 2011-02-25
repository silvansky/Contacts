#ifndef IMAGEMANAGER_H
#define IMAGEMANAGER_H

#include "utilsexport.h"
#include <QImage>

class UTILS_EXPORT ImageManager
{
public:
	static QImage grayscaled(const QImage & image);
	static QImage squared(const QImage & image, int size);
	static QImage roundSquared(const QImage & image, int size, int radius);
	static QImage addShadow(const QImage & image, QColor color, QPoint offset, bool canResize = false);
	static QImage colorized(const QImage & image, QColor color);
};

#endif // IMAGEMANAGER_H
