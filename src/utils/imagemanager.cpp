#include "imagemanager.h"

#include <QPainter>
#include <QDebug>
#include <QBitmap>

QImage ImageManager::grayscaled(const QImage & image)
{
	// TODO: test speed of both methods
	/*static QVector<QRgb> monoTable;
	if (monoTable.isEmpty())
	{
		for (int i = 0; i <= 255; i++)
			monoTable.append(qRgb(i, i, i));
	}
	QImage gray(image.size(), QImage::Format_ARGB32);
	QPainter p(&gray);
	p.drawImage(0, 0, image.convertToFormat(QImage::Format_Indexed8, monoTable));
	p.end();
	return gray;*/
	QImage img = image;
	int pixels = img.width() * img.height();
	unsigned int *data = (unsigned int *)img.bits();
	for (int i = 0; i < pixels; ++i) {
		int val = qGray(data[i]);
		data[i] = qRgba(val, val, val, qAlpha(data[i]));
	}
	return img;
}

QImage ImageManager::squared(const QImage & image, int size)
{
	if ((image.width() == size) && (image.height() == size))
		return image;
	QImage squaredImage(size, size, QImage::Format_ARGB32);
	squaredImage.fill(QColor(0, 0, 0, 0).rgba());
	int w = image.width(), h = image.height();
	QPainter p(&squaredImage);
	QPoint offset;
	QImage copy = (w > h) ? ((h == size) ? image : image.scaledToHeight(size, Qt::SmoothTransformation)) : ((w == size) ? image : image.scaledToWidth(size, Qt::SmoothTransformation));
	w = copy.width();
	h = copy.height();
	offset.setX((w > h) ? (size - w) / 2 : 0);
	offset.setY((w > h) ? 0 : (size - h) / 2);
	p.drawImage(offset, copy);
	p.end();
	return squaredImage;
}

QImage ImageManager::roundSquared(const QImage & image, int size, int radius)
{
	QBitmap bmp(size, size);
	QPainter bp(&bmp);
	bp.fillRect(0, 0, size, size, Qt::color0);
	bp.setPen(QPen(Qt::color1));
	bp.setBrush(QBrush(Qt::color1));
	bp.drawRoundedRect(QRect(0, 0, size - 1, size - 1), radius, radius);
	bp.end();
	QRegion shape(bmp);
	QImage roundSquaredImage(size, size, QImage::Format_ARGB32);
	roundSquaredImage.fill(QColor(0, 0, 0, 0).rgba());
	QPainter p(&roundSquaredImage);
	p.fillRect(0, 0, size, size, Qt::transparent);
	p.setClipRegion(shape);
	p.drawImage(0, 0, squared(image, size));
	p.end();
	return roundSquaredImage;
}

QImage ImageManager::addShadow(const QImage & image, QColor color, QPoint offset, bool canResize)
{
	Q_UNUSED(canResize)

	if (image.isNull())
		return image;

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

QImage ImageManager::colorized(const QImage & image, QColor color)
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

void ImageManager::drawNinePartImage(const QImage &image, QRectF paintRect, qreal border, QPainter * painter)
{
	// TODO: make flexible borders
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
	qreal sx = (tw - 2.0 * border) / (w - 2.0 * border), sy = (th - 2.0 * border) / (h - 2.0 * border);
	qDebug() << paintRect << bg.size() << sx << sy;
	qreal hb = border / 2.0;
	qreal tb = border * 2.0;
	fragments[0] = QPainter::PixmapFragment::create(QPointF(hb, hb),
							QRectF(0, 0, border, border));
	fragments[1] = QPainter::PixmapFragment::create(QPointF(border + (tw - tb) / 2.0, hb),
							QRectF(border, 0, w - 2.0 * border, border),
							sx);
	fragments[2] = QPainter::PixmapFragment::create(QPointF(tw - hb, hb),
							QRectF(w - border, 0, border, border));
	fragments[3] = QPainter::PixmapFragment::create(QPointF(hb, border + (th - tb) / 2.0),
							QRectF(0, border, border, h - 2.0 * border),
							1,
							sy);
	fragments[4] = QPainter::PixmapFragment::create(QPointF(border + (tw - tb) / 2.0, border + (th - tb) / 2.0),
							QRectF(border, border, w - 2.0 * border, h - 2.0 * border),
							sx,
							sy);
	fragments[5] = QPainter::PixmapFragment::create(QPointF(tw - hb, border + (th - tb) / 2.0),
							QRectF(w - border, border, border, h - 2.0 * border),
							1,
							sy);
	fragments[6] = QPainter::PixmapFragment::create(QPointF(hb, th - hb),
							QRectF(0, h - border, border, border));
	fragments[7] = QPainter::PixmapFragment::create(QPointF(border + (tw - tb) / 2.0, th - hb),
							QRectF(border, h - border, w - 2.0 * border, border),
							sx);
	fragments[8] = QPainter::PixmapFragment::create(QPointF(tw - hb, th - hb),
							QRectF(w - border, h - border, border, border));


	painter->drawPixmapFragments(fragments, 9, bg);
}
