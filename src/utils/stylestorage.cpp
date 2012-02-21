#include "stylestorage.h"

#include "imagemanager.h"
#include <QDir>
#include <QFile>
#include <QWidget>
#include <QVariant>
#include <QFileInfo>
#include <QApplication>
#include <QtXml>

#define FOLDER_DEFAULT         "images"
#define IMAGES_FOLDER_PATH     "%IMAGES_PATH%"
#define STYLEVALUES_FILE       "stylevalues.xml"

QHash<QString, StyleStorage *> StyleStorage::FStaticStorages;
QHash<QObject *, StyleStorage *> StyleStorage::FObjectStorage;
QStringList StyleStorage::_systemStyleSuffixes;

struct StyleStorage::StyleUpdateParams
{
	QString key;
	int index;
};

StyleStorage::StyleStorage(const QString &AStorage, const QString &ASubStorage, QObject *AParent) : FileStorage(AStorage,ASubStorage,AParent)
{
	FStyleValuesLoaded = false;
	connect(this,SIGNAL(storageChanged()),SLOT(onStorageChanged()));
}

StyleStorage::~StyleStorage()
{
	foreach(QObject *object, FUpdateParams.keys()) {
		removeObject(object); }
}

QString StyleStorage::getStyle(const QString &AKey, int AIndex) const
{
	QFile file(fileFullName(AKey, AIndex));
	if (file.open(QFile::ReadOnly))
	{
		QString folder = fileOption(AKey, STYLE_STORAGE_OPTION_IMAGES_FOLDER);
		if (folder.isEmpty())
			folder = FOLDER_DEFAULT;
		folder = QFileInfo(file.fileName()).absoluteDir().absoluteFilePath(folder);
		QString resultingStyle = QString::fromUtf8(file.readAll());
		foreach (QString suffix, systemStyleSuffixes())
		{
			QString sFileName = fileFullName(AKey, AIndex, suffix);
			QFile sFile(sFileName);
			if (sFile.open(QFile::ReadOnly))
			{
				resultingStyle += "\n";
				resultingStyle += QString::fromUtf8(sFile.readAll());
			}
		}
		return resultingStyle.replace(IMAGES_FOLDER_PATH,folder);;
	}
	return QString::null;
}

void StyleStorage::insertAutoStyle(QObject *AObject, const QString &AKey, int AIndex)
{
	StyleStorage *oldStorage = FObjectStorage.value(AObject);
	if (oldStorage!=NULL && oldStorage!=this)
		oldStorage->removeAutoStyle(AObject);

	if (AObject && !AKey.isEmpty())
	{
		StyleUpdateParams *params;
		if (oldStorage != this)
		{
			params = new StyleUpdateParams;
			FObjectStorage.insert(AObject,this);
			FUpdateParams.insert(AObject,params);
		}
		else
		{
			params = FUpdateParams.value(AObject);
		}
		params->key = AKey;
		params->index = AIndex;
		updateObject(AObject);
		connect(AObject,SIGNAL(destroyed(QObject *)),SLOT(onObjectDestroyed(QObject *)));
	}
	else if (AObject != NULL)
	{
		removeAutoStyle(AObject);
	}
}

void StyleStorage::removeAutoStyle(QObject *AObject)
{
	if (FUpdateParams.contains(AObject))
	{
		AObject->setProperty("styleSheet",QString());
		removeObject(AObject);
		disconnect(AObject,SIGNAL(destroyed(QObject *)),this,SLOT(onObjectDestroyed(QObject *)));
	}
}

QString StyleStorage::fileFullName(const QString AKey, int AIndex) const
{
	return FileStorage::fileFullName(AKey, AIndex);
}

QString StyleStorage::fileFullName(const QString AKey, int AIndex, const QString & suffix) const
{
	QString filename = fileFullName(AKey, AIndex);
	QFileInfo finfo(filename);
	return finfo.absoluteDir().absolutePath() + "/" +finfo.baseName() + suffix + "." + finfo.completeSuffix();
}

QVariant StyleStorage::getStyleValue(const QString & AKey)
{
	if (!FStyleValuesLoaded)
		loadStyleValues();

	QStringList keys;

	foreach(QString suffix, systemStyleSuffixes())
	{
		keys += AKey + suffix;
	}

	keys += AKey;

	foreach(QString key, keys)
	{
		if (FStyleValues.contains(key))
			return FStyleValues.value(key);
	}

	return QVariant();
}

QColor StyleStorage::getStyleColor(const QString & AKey)
{
	return getStyleValue(AKey).value<QColor>();
}

int StyleStorage::getStyleInt(const QString & AKey)
{
	return getStyleValue(AKey).toInt();
}

bool StyleStorage::getStyleBool(const QString & AKey)
{
	return getStyleValue(AKey).toBool();
}

void StyleStorage::previewReset()
{
	onStorageChanged();
	emit stylePreviewReset();
}

void StyleStorage::previewStyle(const QString &AStyleSheet, const QString &AKey, int AIndex)
{
	for (QHash<QObject *, StyleUpdateParams *>::iterator it=FUpdateParams.begin(); it!=FUpdateParams.end(); it++)
	{
		if (it.value()->key==AKey && it.value()->index==AIndex)
			it.key()->setProperty("styleSheet", AStyleSheet);
	}
}

StyleStorage *StyleStorage::staticStorage(const QString &AStorage)
{
	StyleStorage *styleStorage = FStaticStorages.value(AStorage,NULL);
	if (!styleStorage)
	{
		styleStorage = new StyleStorage(AStorage,STORAGE_SHARED_DIR,qApp);
		FStaticStorages.insert(AStorage,styleStorage);
	}
	return styleStorage;
}

void StyleStorage::updateStyle(QObject * object)
{
	if (QWidget * w = qobject_cast<QWidget*>(object))
	{
		w->setStyleSheet(w->styleSheet());
	}
	else
	{
		object->setProperty("styleSheet", object->property("styleSheet"));
	}
}

QStringList StyleStorage::systemStyleSuffixes()
{
	if (_systemStyleSuffixes.isEmpty())
	{
#if defined Q_WS_MAC
		_systemStyleSuffixes += "_mac";
#elif defined Q_WS_WIN
		_systemStyleSuffixes += "_win";
#elif defined Q_WS_X11
		_systemStyleSuffixes += "_x11";
#elif defined Q_WS_S60
		_systemStyleSuffixes += "_s60";
#endif
	}
	return _systemStyleSuffixes;
}

void StyleStorage::loadStyleValues()
{
	if (FStyleValuesLoaded)
		return;

	QStringList dirs = subStorageDirs(storage(), subStorage());

	if (!dirs.count())
		return;

	QString dir = dirs.value(0);
	QFile f(QString("%1/%2").arg(dir).arg(STYLEVALUES_FILE));

	if (!f.open(QFile::ReadOnly))
		return;

	QDomDocument doc;
	doc.setContent(f.readAll());
	f.close();

	QDomElement valueEl = doc.documentElement().firstChildElement("stylevalue");

	QString key, type, value;

	// TODO: add more types
	while (!valueEl.isNull())
	{
		key = valueEl.attribute("key");
		type = valueEl.attribute("type");
		value = valueEl.attribute("value");
		if (type == "color")
		{
			// parsing color
			QColor c = ImageManager::resolveColor(value);
			FStyleValues.insert(key, c);
		}
		else
		{
			// save as string
			FStyleValues.insert(key, value);
		}
		valueEl = valueEl.nextSiblingElement("stylevalue");
	}

	FStyleValuesLoaded = true;
}

void StyleStorage::updateObject(QObject *AObject)
{
	StyleUpdateParams *params = FUpdateParams.value(AObject);
	QString style = getStyle(params->key,params->index);
	AObject->setProperty("styleSheet",style);
}

void StyleStorage::removeObject(QObject *AObject)
{
	FObjectStorage.remove(AObject);
	StyleUpdateParams *params = FUpdateParams.take(AObject);
	delete params;
}

void StyleStorage::onStorageChanged()
{
	for (QHash<QObject *, StyleUpdateParams *>::iterator it=FUpdateParams.begin(); it!=FUpdateParams.end(); it++)
		updateObject(it.key());
}

void StyleStorage::onObjectDestroyed(QObject *AObject)
{
	removeObject(AObject);
}
