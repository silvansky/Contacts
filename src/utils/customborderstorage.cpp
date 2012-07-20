#include "customborderstorage.h"

#ifdef Q_WS_X11
#	include <QX11Info>
#endif

#include <QApplication>
#include "custombordercontainer_p.h"

CustomBorderStorage::CustomBorderStorage(const QString &AStorage, const QString &ASubStorage, QObject *AParent) : FileStorage(AStorage,ASubStorage,AParent)
{
}

CustomBorderStorage::~CustomBorderStorage()
{
}

CustomBorderContainer * CustomBorderStorage::addBorder(QWidget *widget, const QString &key)
{
	if (isBordersAvail() && isBordersEnabled() && widget->isWindow() && !isBordered(widget))
	{
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
	}
	return NULL;
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

bool CustomBorderStorage::isBordersAvail()
{
#ifdef Q_WS_X11
	return QX11Info::isCompositingManagerRunning();
#endif
	return true;
}

bool CustomBorderStorage::isBordersEnabled()
{
	return bordersEnabled;
}

void CustomBorderStorage::setBordersEnabled(bool enabled)
{
	bordersEnabled = enabled;
}

bool CustomBorderStorage::isBordered(QWidget *widget)
{
	return widgetBorder(widget)!=NULL;
}

CustomBorderContainer * CustomBorderStorage::widgetBorder(QWidget *widget)
{
	return qobject_cast<CustomBorderContainer *>(widget->window());
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
bool CustomBorderStorage::bordersEnabled = false;
QHash<QString, CustomBorderContainerPrivate *> CustomBorderStorage::borderStyleCache;
QHash<QWidget *, CustomBorderContainer *> CustomBorderStorage::borderCache;
QHash<QString, CustomBorderStorage *> CustomBorderStorage::staticStorages;

