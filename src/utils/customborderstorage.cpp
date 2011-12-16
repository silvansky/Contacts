#include "customborderstorage.h"
#include "custombordercontainer_p.h"
#include <QApplication>

CustomBorderStorage::CustomBorderStorage(const QString &AStorage, const QString &ASubStorage, QObject *AParent) : FileStorage(AStorage,ASubStorage,AParent)
{
}

CustomBorderStorage::~CustomBorderStorage()
{
}

CustomBorderContainer * CustomBorderStorage::addBorder(QWidget *widget, const QString &key)
{
#ifndef Q_WS_MAC
	CustomBorderContainerPrivate * style = borderStyleCache.value(key, NULL);
	if (!style)
	{
		QString fileKey = fileCacheKey(key);
		if (!fileKey.isEmpty())
		{
			QString filename = fileFullName(key);
			if (!filename.isEmpty())
			{
				style = new CustomBorderContainerPrivate(NULL);
				style->parseFile(filename);
				borderStyleCache.insert(key, style);
			}
		}
	}
	if (style)
	{
		CustomBorderContainer * container = new CustomBorderContainer(*style);
		container->setWidget(widget);
		borderCache.insert(widget, container);
		return container;
	}
	else
		return NULL;
#else
	Q_UNUSED(widget)
	Q_UNUSED(key)
	return NULL;
#endif
}

void CustomBorderStorage::removeBorder(QWidget *widget)
{
	CustomBorderContainer * container = borderCache.value(widget, NULL);
	if (container)
	{
		container->releaseWidget();
		borderCache.remove(widget);
		container->deleteLater();
	}
}

CustomBorderStorage * CustomBorderStorage::staticStorage(const QString & storage)
{
	CustomBorderStorage * _storage = staticStorages.value(storage, NULL);
	if (!_storage)
	{
		_storage = new CustomBorderStorage(storage, STORAGE_SHARED_DIR, qApp);
		staticStorages.insert(storage, _storage);
	}
	return _storage;
}

// static vars

QHash<QString, CustomBorderContainerPrivate *> CustomBorderStorage::borderStyleCache;
QHash<QWidget *, CustomBorderContainer *> CustomBorderStorage::borderCache;
QHash<QString, CustomBorderStorage *> CustomBorderStorage::staticStorages;
