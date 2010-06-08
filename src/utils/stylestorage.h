#ifndef STYLESTORAGE_H
#define STYLESTORAGE_H

#include "filestorage.h"

#define STYLE_STORAGE_OPTION_IMAGES_FOLDER "folder"

class UTILS_EXPORT StyleStorage :
			public FileStorage
{
	Q_OBJECT;
public:
	StyleStorage(const QString &AStorage, const QString &ASubStorage = STORAGE_SHARED_DIR, QObject *AParent = NULL);
	virtual ~StyleStorage();
	QString getStyle(const QString &AKey, int AIndex = 0) const;
	void insertAutoStyle(QObject *AObject, const QString &AKey, int AIndex = 0);
	void removeAutoStyle(QObject *AObject);
public:
	static StyleStorage *staticStorage(const QString &AStorage);
private:
	static QHash<QString, StyleStorage *> FStaticStorages;
};

#endif // STYLESTORAGE_H
