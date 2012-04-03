#include "imagemanager.h"
#include "log.h"

#include <QPainter>
#include <QBitmap>

QImage ImageManager::grayscaled(const QImage & image)
{
	QImage img = image;
	if (!image.isNull())
	{
		int pixels = img.width() * img.height();
		if (pixels*(int)sizeof(QRgb) <= img.byteCount())
		{
			QRgb *data = (QRgb *)img.bits();
			for (int i = 0; i < pixels; i++)
			{
				int val = qGray(data[i]);
				data[i] = qRgba(val, val, val, qAlpha(data[i]));
			}
		}
	}
	return img;
}

QImage ImageManager::squared(const QImage & image, int size)
{
	if (!image.isNull())
	{
		if ((image.width() == size) && (image.height() == size))
			return image;
		QImage squaredImage(size, size, QImage::Format_ARGB32);
		squaredImage.fill(QColor(0, 0, 0, 0).rgba());
		int w = image.width(), h = image.height();
		QPainter p(&squaredImage);
		QPoint offset(0,0);
		QImage copy = (w < h) ? ((h == size) ? image : image.scaledToHeight(size, Qt::SmoothTransformation)) : ((w == size) ? image : image.scaledToWidth(size, Qt::SmoothTransformation));
		w = copy.width();
		h = copy.height();
		if (w > h)
			offset.setY((size - h) / 2);
		else if (h > w)
			offset.setX((size - w) / 2);
		p.drawImage(offset, copy);
		p.end();
		return squaredImage;
	}
	return QImage();
}

QImage ImageManager::roundSquared(const QImage & image, int size, int radius)
{
	if (!image.isNull())
	{
		QImage shapeImg(QSize(size, size), QImage::Format_ARGB32_Premultiplied);
		shapeImg.fill(QColor(0, 0, 0, 0).rgba());
		QPainter sp(&shapeImg);
		sp.setRenderHint(QPainter::Antialiasing);
		sp.fillRect(0, 0, size, size, Qt::transparent);
		sp.setPen(QPen(Qt::color1));
		sp.setBrush(QBrush(Qt::color1));
		sp.drawRoundedRect(QRect(0, 0, size, size), radius + 1, radius + 1);
		sp.end();
		QImage roundSquaredImage(size, size, QImage::Format_ARGB32_Premultiplied);
		roundSquaredImage.fill(QColor(0, 0, 0, 0).rgba());
		QPainter p(&roundSquaredImage);
		p.fillRect(0, 0, size, size, Qt::transparent);
		p.drawImage(0, 0, shapeImg);
		p.setCompositionMode(QPainter::CompositionMode_SourceIn);
		p.drawImage(0, 0, squared(image, size));
		p.end();
		return roundSquaredImage;
	}
	return QImage();
}

QImage ImageManager::addShadow(const QImage & image, QColor color, QPoint offset, bool canResize)
{
	Q_UNUSED(canResize)
	if (!image.isNull())
	{
		QImage shadowed(image.size(), image.format());
		shadowed.fill(QColor(0, 0, 0, 0).rgba());
		QPainter p(&shadowed);

		QImage tmp(image.size(), QImage::Format_ARGB32_Premultiplied);
		tmp.fill(0);
		QPainter tmpPainter(&tmp);
		tmpPainter.setCompositionMode(QPainter::CompositionMode_Source);
		tmpPainter.drawPixmap(offset, QPixmap::fromImage(image));
		tmpPainter.setCompositionMode(QPainter::CompositionMode_SourceIn);
		tmpPainter.fillRect(tmp.rect(), color);
		tmpPainter.end();

		p.drawImage(0, 0, tmp);

		p.drawPixmap(0, 0, QPixmap::fromImage(image));
		p.end();
		return shadowed;
	}
	return QImage();
}

QImage ImageManager::colorized(const QImage & image, QColor color)
{
	if (!image.isNull())
	{
		QImage resultImage(image.size(), QImage::Format_ARGB32_Premultiplied);
		QPainter painter(&resultImage);
		painter.drawImage(0, 0, grayscaled(image));
		painter.setCompositionMode(QPainter::CompositionMode_Screen);
		painter.fillRect(resultImage.rect(), color);
		painter.end();
		resultImage.setAlphaChannel(image.alphaChannel());
		return resultImage;
	}
	return QImage();
}

QImage ImageManager::opacitized(const QImage & image, double opacity)
{
	if (!image.isNull())
	{
		QImage resultImage(image.size(), QImage::Format_ARGB32);
		resultImage.fill(QColor::fromRgb(0, 0, 0, 0).rgba());
		QPainter painter(&resultImage);
		painter.setOpacity(opacity);
		painter.drawImage(0, 0, image);
		painter.end();
		resultImage.setAlphaChannel(image.alphaChannel());
		return resultImage;
	}
	return QImage();
}

QImage ImageManager::addSpace(const QImage & image, int left, int top, int right, int bottom)
{
    if (!image.isNull())
    {
        QImage resultImage(image.size() + QSize(left + right, top + bottom), QImage::Format_ARGB32);
        resultImage.fill(QColor::fromRgb(0, 0, 0, 0).rgba());
        QPainter painter(&resultImage);
        painter.drawImage(left, top, image);
        painter.end();
        resultImage.setAlphaChannel(image.alphaChannel());
        return resultImage;
    }
	return QImage();
}

QImage ImageManager::rotatedImage(const QImage & image, qreal angle)
{
	if (!image.isNull())
	{
		QImage rotated(image.size(), QImage::Format_ARGB32_Premultiplied);
		rotated.fill(Qt::transparent);
		QPainter p(&rotated);
		p.setRenderHint(QPainter::Antialiasing);
		p.setRenderHint(QPainter::SmoothPixmapTransform);
		qreal dx = image.size().width() / 2.0, dy = image.size().height() / 2.0;
		p.translate(dx, dy);
		p.rotate(angle);
		p.translate(-dx, -dy);
		p.drawImage(0, 0, image);
		p.end();
		return rotated;
	}
	return QImage();
}

void ImageManager::drawNinePartImage(const QImage &image, QRectF paintRect, qreal borderLeft, qreal borderRight, qreal borderTop, qreal borderBottom, QPainter * painter)
{
	if (!image.isNull())
	{
		QPixmap bg = QPixmap::fromImage(image);
		// source size
		qreal w = bg.width();
		qreal h = bg.height();
		// target size
		qreal tw = paintRect.width();
		qreal th = paintRect.height();

		/*
		+-------+-------------------+-------+
		|       |                   |       |
		|   0   |         1         |   2   |
		|       |                   |       |
		+-------+-------------------+-------+
		|       |                   |       |
		|   3   |         4         |   5   |
		|       |                   |       |
		+-------+-------------------+-------+
		|       |                   |       |
		|   6   |         7         |   8   |
		|       |                   |       |
		+-------+-------------------+-------+
		*/

		QPainter::PixmapFragment fragments[9]; // we'll draw 9-part pixmap
		qreal hborders = borderLeft + borderRight;
		qreal vborders = borderTop + borderBottom;
		qreal sx = (tw - hborders) / (w - hborders), sy = (th - vborders) / (h - vborders);
		qreal hbLeft = borderLeft / 2.0;
		qreal hbRight = borderRight / 2.0;
		qreal hbTop = borderTop / 2.0;
		qreal hbBottom = borderBottom / 2.0;
		fragments[0] = QPainter::PixmapFragment::create(QPointF(hbLeft, hbTop),
			QRectF(0, 0, borderLeft, borderTop));
		fragments[1] = QPainter::PixmapFragment::create(QPointF(borderLeft + (tw - hborders) / 2.0, hbTop),
			QRectF(borderLeft, 0, w - hborders, borderTop),
			sx);
		fragments[2] = QPainter::PixmapFragment::create(QPointF(tw - hbRight, hbTop),
			QRectF(w - borderRight, 0, borderRight, borderTop));
		fragments[3] = QPainter::PixmapFragment::create(QPointF(hbLeft, borderTop + (th - hborders) / 2.0),
			QRectF(0, borderTop, borderLeft, h - hborders),
			1.0,
			sy);
		fragments[4] = QPainter::PixmapFragment::create(QPointF(borderLeft + (tw - hborders) / 2.0, borderTop + (th - vborders) / 2.0),
			QRectF(borderLeft, borderTop, w - hborders, h - vborders),
			sx,
			sy);
		fragments[5] = QPainter::PixmapFragment::create(QPointF(tw - hbRight, borderTop + (th - vborders) / 2.0),
			QRectF(w - borderRight, borderTop, borderRight, h - vborders),
			1,
			sy);
		fragments[6] = QPainter::PixmapFragment::create(QPointF(hbLeft, th - hbBottom),
			QRectF(0, h - borderBottom, borderLeft, borderBottom));
		fragments[7] = QPainter::PixmapFragment::create(QPointF(borderLeft + (tw - hborders) / 2.0, th - hbBottom),
			QRectF(borderLeft, h - borderBottom, w - hborders, borderBottom),
			sx);
		fragments[8] = QPainter::PixmapFragment::create(QPointF(tw - hbRight, th - hbBottom),
			QRectF(w - borderRight, h - borderBottom, borderRight, borderBottom));

		painter->drawPixmapFragments(fragments, 9, bg);
	}
}

void ImageManager::drawNinePartImage(const QImage &image, QRectF paintRect, qreal border, QPainter * painter)
{
	drawNinePartImage(image, paintRect, border, border, border, border, painter);
}

QColor ImageManager::resolveColor(const QString & name)
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
		LogError(QString("[ImageManager::resolveColor] Can\'t parse color: %1").arg(name));

	return color;
}
