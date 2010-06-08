#include "stylestorage.h"

#include <QDir>
#include <QFile>
#include <QWidget>
#include <QVariant>
#include <QFileInfo>
#include <QApplication>

#define FOLDER_DEFAULT         "images"
#define IMAGES_FOLDER_PATH     "%IMAGES_PATH%"

QHash<QString, StyleStorage *> StyleStorage::FStaticStorages;

StyleStorage::StyleStorage(const QString &AStorage, const QString &ASubStorage, QObject *AParent) : FileStorage(AStorage,ASubStorage,AParent)
{

}

StyleStorage::~StyleStorage()
{

}

QString StyleStorage::getStyle(const QString &AKey, int AIndex) const
{
	QFile file(fileFullName(AKey,AIndex));
	if (file.open(QFile::ReadOnly))
	{
		QString folder = fileOption(AKey, STYLE_STORAGE_OPTION_IMAGES_FOLDER);
		if (folder.isEmpty())
			folder = FOLDER_DEFAULT;
		folder = QFileInfo(file.fileName()).absoluteDir().absoluteFilePath(folder);
		return QString::fromUtf8(file.readAll()).replace(IMAGES_FOLDER_PATH,folder);
	}
	return QString::null;
}

void StyleStorage::insertAutoStyle(QObject *AObject, const QString &AKey, int AIndex)
{
	if (AObject)
	{
		QString style = getStyle(AKey,AIndex);
		AObject->setProperty("styleSheet",style);
	}
}

void StyleStorage::removeAutoStyle(QObject *AObject)
{
	if (AObject)
	{
		QString style;
		AObject->setProperty("styleSheet",style);
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
