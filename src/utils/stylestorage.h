#ifndef STYLESTORAGE_H
#define STYLESTORAGE_H

#include "filestorage.h"

#define STYLE_STORAGE_OPTION_IMAGES_FOLDER "folder"

class UTILS_EXPORT StyleStorage :
			public FileStorage
{
	Q_OBJECT
	struct StyleUpdateParams;
public:
	StyleStorage(const QString &AStorage, const QString &ASubStorage = STORAGE_SHARED_DIR, QObject *AParent = NULL);
	virtual ~StyleStorage();
	QString getStyle(const QString &AKey, int AIndex = 0) const;
	void insertAutoStyle(QObject *AObject, const QString &AKey, int AIndex = 0);
	void removeAutoStyle(QObject *AObject);
public slots:
	void previewReset();
	void previewStyle(const QString &AStyleSheet, const QString &AKey, int AIndex);
signals:
	void stylePreviewReset();
public:
	static StyleStorage *staticStorage(const QString &AStorage);
	static void updateStyle(QObject * object);
protected:
	void updateObject(QObject *AObject);
	void removeObject(QObject *AObject);
protected slots:
	void onStorageChanged();
	void onObjectDestroyed(QObject *AObject);
private:
	QHash<QObject *, StyleUpdateParams *> FUpdateParams;
private:
	static QHash<QString, StyleStorage *> FStaticStorages;
	static QHash<QObject *, StyleStorage *> FObjectStorage;
};

#endif // STYLESTORAGE_H
