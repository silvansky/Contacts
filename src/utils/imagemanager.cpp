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
	qDebug() << "size: " << size << " image: " << image.size();
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
	Q_UNUSED(radius)
	qDebug() << "ImageManager::roundSquared() not implemented or stable yet!" << size << " " << radius;
	QImage roundSquaredImage(size, size, QImage::Format_ARGB32);
	QPainter p(&roundSquaredImage);
	QImage squaredImage = squared(image, size);
	QBitmap bmp(size, size);
	QPainter bp(&bmp);
	bp.fillRect(0, 0, size, size, Qt::white);
	bp.setPen(QPen(QColor(0, 0, 0)));
	bp.setBrush(QBrush(QColor(255, 0, 0)));
	bp.drawRoundedRect(QRect(0, 0, size - 1, size - 1), radius, radius);
	bp.end();
	QRegion shape(bmp);
	p.setClipRegion(shape);
	p.drawImage(0, 0, squaredImage);
	//p.drawPixmap(0, 0, bmp);
	//QPen pen;
//	pen.setWidth(0);
//	p.fillRect(0, 0, size, size, Qt::white);
//	p.setPen(pen);
//	p.setBrush(QBrush(QColor(255, 0, 0)));
//	p.drawRoundedRect(QRect(0, 0, size - 1, size - 1), radius, radius);
	p.end();
	return roundSquaredImage;
}

QImage ImageManager::addShadow(const QImage & image, QColor color, QPoint offset, bool canResize)
{
	return image;
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
