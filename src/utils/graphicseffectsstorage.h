#ifndef GRAPHICSEFFECTSSTORAGE_H
#define GRAPHICSEFFECTSSTORAGE_H

#include "filestorage.h"
#include <QGraphicsEffect>

class QDomElement;

class UTILS_EXPORT GraphicsEffectsStorage : public FileStorage
{
	Q_OBJECT
public:
	GraphicsEffectsStorage(const QString &AStorage, const QString &ASubStorage = QString::null, QObject *AParent = NULL);
	~GraphicsEffectsStorage();
	// this will install effect for all child widgets that matches the mask (class and/or object name)
	bool installGraphicsEffect(QWidget * widget, const QString & key);
	// this will install effect for all application's widgets that matches the mask (class and/or object name)
	bool installGraphicsEffect(const QString & key);
	// this will uninstall effect for all child widgets that matches the mask (class and/or object name)
	bool uninstallGraphicsEffect(QWidget * widget, const QString & key);
	// this will uninstall effect for all application's widgets that matches the mask (class and/or object name)
	bool uninstallGraphicsEffect(const QString & key);
	// returns all effects for the specifyed key
	QList<QGraphicsEffect*> getEffects(const QString & key);
	// returns first effect for the specifyed key
	QGraphicsEffect * getFirstEffect(const QString & key);
public:
	static GraphicsEffectsStorage * staticStorage(const QString & storage);
protected:
	struct EffectMask
	{
		QString key;
		QStringList classNames;
		QStringList objectNames;
		bool operator ==(const EffectMask & other) const
		{
			return key == other.key && classNames == other.classNames && objectNames == other.objectNames;
		}
	};
	friend uint qHash(const GraphicsEffectsStorage::EffectMask &mask);
	void parseFile(const QString & key);
	QGraphicsEffect * parseGraphicEffect(const QDomElement & element);
	QGraphicsEffect * copyEffect(const QGraphicsEffect * effect) const;
	QGraphicsEffect * effectForMask(const EffectMask & mask, QObject * parent) const;
	bool widetMatchesTheMask(QWidget* widget, const EffectMask & mask) const;
private:
	static QMultiHash<QString, EffectMask> keyMaskCache;
	static QHash<EffectMask, QGraphicsEffect *> effectCache;
	static QHash<QString, GraphicsEffectsStorage *> staticStorages;
	static QSet<QString> loadedKeysCache;
};

inline uint qHash(const GraphicsEffectsStorage::EffectMask &mask)
{
	return qHash(mask.key + " | "+ mask.classNames.join(";") + " | " + mask.objectNames.join(";"));
}

#endif // GRAPHICSEFFECTSSTORAGE_H
