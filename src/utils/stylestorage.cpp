#include "stylestorage.h"

#include <QDir>
#include <QFile>
#include <QWidget>
#include <QVariant>
#include <QFileInfo>
#include <QApplication>

#include <QDebug>

#define FOLDER_DEFAULT         "images"
#define IMAGES_FOLDER_PATH     "%IMAGES_PATH%"

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
#ifdef Q_WS_MAC
		_systemStyleSuffixes += "_mac";
#elif Q_WS_WIN
		systemStyleSuffixes += "_win";
#elif Q_WS_X11
		systemStyleSuffixes += "_x11";
#elif Q_WS_S60
		systemStyleSuffixes += "_s60";
#endif
	}
	return _systemStyleSuffixes;
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
