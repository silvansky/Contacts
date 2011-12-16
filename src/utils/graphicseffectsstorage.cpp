#include "graphicseffectsstorage.h"
#include <QDomDocument>
#include <QGraphicsDropShadowEffect>
#include <QSet>
#include <QFile>
#include <QWidget>
#include <QApplication>
#include <QCheckBox>
#include <QRadioButton>
#include <QStyleOptionButton>
#include <QPainter>

static QColor parseColor(const QString & name)
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
	return color;
}

class TextDropShadowEffect : public QGraphicsDropShadowEffect
{
public:
	TextDropShadowEffect(QObject* parent) : QGraphicsDropShadowEffect(parent) { }
protected:
	void draw(QPainter *painter)
	{
		if (parent())
		{
			if (qobject_cast<QCheckBox*>(parent()) || qobject_cast<QRadioButton*>(parent()))
			{
				QWidget * w = qobject_cast<QWidget*>(parent());
				QStyle * style = w->style() ? w->style() : qApp->style();
				QStyleOptionButton * opt = new QStyleOptionButton;
				opt->rect = w->rect();
				int indicatorWidth = style->subElementRect(QStyle::SE_CheckBoxIndicator, opt, w).width() + xOffset() + 2; // 2 is a magic namba!
				painter->setClipRect(0, 0, indicatorWidth, w->height());
				QPoint offset;
				const QPixmap pixmap = sourcePixmap(Qt::DeviceCoordinates, &offset);
				QTransform restoreTransform = painter->worldTransform();
				painter->setWorldTransform(QTransform());
				painter->drawPixmap(offset, pixmap);
				painter->setWorldTransform(restoreTransform);
				painter->setClipRect(indicatorWidth, 0, w->width() - indicatorWidth, w->height());
			}
		}
		if (painter->isActive())
			QGraphicsDropShadowEffect::draw(painter);
	}
};

GraphicsEffectsStorage::GraphicsEffectsStorage(const QString &AStorage, const QString &ASubStorage, QObject *AParent) :
	FileStorage(AStorage, ASubStorage, AParent)
{
}

GraphicsEffectsStorage::~GraphicsEffectsStorage()
{

}

bool GraphicsEffectsStorage::installGraphicsEffect(QWidget * widget, const QString & key)
{
	QList<EffectMask> masks = keyMaskCache.values(key);
	if (masks.isEmpty())
	{
		parseFile(key);
		masks = keyMaskCache.values(key);
	}
	if (widget && !masks.isEmpty())
	{
		foreach (EffectMask mask, masks)
			if (widetMatchesTheMask(widget, mask))
				widget->setGraphicsEffect(effectForMask(mask, widget));
		foreach (QObject * child, widget->children())
			if (child->isWidgetType())
				installGraphicsEffect(qobject_cast<QWidget*>(child), key);
		return true;
	}
	return false;
}

bool GraphicsEffectsStorage::installGraphicsEffect(const QString & key)
{
	QList<EffectMask> masks = keyMaskCache.values(key);
	if (masks.isEmpty())
	{
		parseFile(key);
		masks = keyMaskCache.values(key);
	}
	foreach (QWidget* widget, qApp->allWidgets())
		foreach (EffectMask mask, masks)
			if (widetMatchesTheMask(widget, mask))
				widget->setGraphicsEffect(effectForMask(mask, widget));
	return true;
}

bool GraphicsEffectsStorage::uninstallGraphicsEffect(QWidget * widget, const QString & key)
{
	QList<EffectMask> masks = keyMaskCache.values(key);
	if (masks.isEmpty())
	{
		parseFile(key);
		masks = keyMaskCache.values(key);
	}
	if (widget && !masks.isEmpty())
	{
		foreach (EffectMask mask, masks)
			if (widetMatchesTheMask(widget, mask))
				widget->setGraphicsEffect(NULL);
		foreach (QObject * child, widget->children())
			if (child->isWidgetType())
				uninstallGraphicsEffect(qobject_cast<QWidget*>(child), key);
		return true;
	}
	return false;
}

bool GraphicsEffectsStorage::uninstallGraphicsEffect(const QString & key)
{
	QList<EffectMask> masks = keyMaskCache.values(key);
	if (masks.isEmpty())
	{
		parseFile(key);
		masks = keyMaskCache.values(key);
	}
	foreach (QWidget* widget, qApp->allWidgets())
		foreach (EffectMask mask, masks)
			if (widetMatchesTheMask(widget, mask))
				widget->setGraphicsEffect(NULL);
	return true;
}

QList<QGraphicsEffect*> GraphicsEffectsStorage::getEffects(const QString & key)
{
	QList<EffectMask> masks = keyMaskCache.values(key);
	if (masks.isEmpty())
	{
		parseFile(key);
		masks = keyMaskCache.values(key);
	}
	QList<QGraphicsEffect*> effects;
	foreach (EffectMask mask, masks)
			effects << effectForMask(mask, NULL);
	return effects;
}

QGraphicsEffect * GraphicsEffectsStorage::getFirstEffect(const QString & key)
{
	QList<QGraphicsEffect*> effects = getEffects(key);
	QGraphicsEffect * effect = effects.isEmpty() ? NULL : effects.first();
	if (effect)
	{
		effects.removeFirst();
		qDeleteAll(effects);
	}
	return effect;
}

GraphicsEffectsStorage * GraphicsEffectsStorage::staticStorage(const QString & storage)
{
	GraphicsEffectsStorage * _storage = staticStorages.value(storage, NULL);
	if (!_storage)
	{
		_storage = new GraphicsEffectsStorage(storage, STORAGE_SHARED_DIR, qApp);
		staticStorages.insert(storage, _storage);
	}
	return _storage;
}

void GraphicsEffectsStorage::parseFile(const QString & key)
{
	if (!loadedKeysCache.contains(key))
	{
		QString fileKey = fileCacheKey(key);
		if (!fileKey.isEmpty())
		{
			QString fileName = fileFullName(key);
			if (!fileName.isEmpty())
			{
				QFile file(fileName);
				if (file.open(QFile::ReadOnly))
				{
					QString s = QString::fromUtf8(file.readAll());
					QDomDocument doc;
					if (doc.setContent(s))
					{
						QDomElement root = doc.firstChildElement("graphics-effects");
						if (!root.isNull())
						{
							QDomElement effect = root.firstChildElement("effect");
							while (!effect.isNull())
							{
								EffectMask mask;
								mask.key = key;
								QDomElement classes = effect.firstChildElement("classes");
								if (!classes.isNull())
								{
									QDomElement classEl = classes.firstChildElement("class");
									while (!classEl.isNull())
									{
										mask.classNames.append(classEl.text());
										classEl = classEl.nextSiblingElement("class");
									}
								}
								QDomElement names = effect.firstChildElement("names");
								if (!names.isNull())
								{
									QDomElement nameEl = names.firstChildElement("name");
									while (!nameEl.isNull())
									{
										mask.objectNames.append(nameEl.text());
										nameEl = nameEl.nextSiblingElement("name");
									}
								}
								keyMaskCache.insert(key, mask);
								QGraphicsEffect * newEffect = parseGraphicEffect(effect);
								if (newEffect)
								{
									effectCache.insert(mask, newEffect);
								}
								effect = effect.nextSiblingElement("effect");
							}
						}
					}
				}
			}
		}
		loadedKeysCache.insert(key);
	}
}

QGraphicsEffect * GraphicsEffectsStorage::parseGraphicEffect(const QDomElement & element)
{
	if (element.attribute("type") == "shadow")
	{
		TextDropShadowEffect * effect = new TextDropShadowEffect(NULL);
		QDomElement color = element.firstChildElement("color");
		if (!color.isNull())
		{
			effect->setColor(parseColor(color.text()));
		}
		QDomElement offset = element.firstChildElement("offset");
		if (!offset.isNull())
		{
			effect->setXOffset(offset.attribute("x").toInt());
			effect->setYOffset(offset.attribute("y").toInt());
		}
		QDomElement blur = element.firstChildElement("blur");
		if (!blur.isNull())
		{
			// we won't need it i think...
			//effect->setBlurRadius(blur.attribute("radius").toDouble());
		}
		return effect;
	}
	return NULL;
}

QGraphicsEffect * GraphicsEffectsStorage::copyEffect(const QGraphicsEffect * effect) const
{
	// only QGraphicsDropShadowEffect for now
	if (const QGraphicsDropShadowEffect* shadowEffect = qobject_cast<const QGraphicsDropShadowEffect*>(effect))
	{
		TextDropShadowEffect * copy = new TextDropShadowEffect(effect->parent());
		copy->setOffset(shadowEffect->offset());
		copy->setBlurRadius(shadowEffect->blurRadius());
		copy->setColor(shadowEffect->color());
		return copy;
	}
	return NULL;
}

QGraphicsEffect * GraphicsEffectsStorage::effectForMask(const GraphicsEffectsStorage::EffectMask & mask, QObject * parent) const
{
	// graphics effects work bad on Qt 4.8
#if (QT_VERSION < 0x040800)
	QGraphicsEffect * effect = copyEffect(effectCache.value(mask, NULL));
	effect->setParent(parent);
	return effect;
#else
	return NULL;
#endif
}

bool GraphicsEffectsStorage::widetMatchesTheMask(QWidget* widget, const GraphicsEffectsStorage::EffectMask & mask) const
{
	return (mask.objectNames.contains(widget->objectName()) || mask.classNames.contains(widget->metaObject()->className()));
}

// static vars

QMultiHash<QString, GraphicsEffectsStorage::EffectMask> GraphicsEffectsStorage::keyMaskCache;
QHash<GraphicsEffectsStorage::EffectMask, QGraphicsEffect*> GraphicsEffectsStorage::effectCache;
QHash<QString, GraphicsEffectsStorage *> GraphicsEffectsStorage::staticStorages;
QSet<QString> GraphicsEffectsStorage::loadedKeysCache;
