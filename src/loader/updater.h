#ifndef UPDATER_H
#define UPDATER_H

#include <QObject>
#include <QDomDocument>
#include <QNetworkReply>
#include "downloader.h"

class Updater : public QObject
{
	Q_OBJECT

public:
	Updater(QObject *parent);
	~Updater();

	// Проверить обновление
	bool checkUpdate();

	// Параметры обновления
	QString getVersion() const { return version; }
	int getSize() const { return size; }
	QUrl getUrl() const { return url; }
	QString getDescription() const { return description; }
	QString getUpdateFilename() const;


signals:
	void forceUpdate(); // Сигнал принудительного обновления
	void updateFinished(QString, bool);
	//void updateCanceled(QString reason);

public slots:
	void update(); // Обновление
	void cancelUpdate(QString reason); // Отмена обновления

protected slots:
	void downloadUpdate(); // Скачать обновление с сервера
	bool setUpdate(); // Установить обновление

private:
	QString version;
	int size;
	QUrl url;
	QString description;
	bool isForceUpdate;

private:
	QNetworkReply *reply;
	Downloader* downloader;

protected slots:
	void readyReadReply();
	void getReplyFinished();
	void replyFinished(QNetworkReply*);
};

#endif // UPDATER_H
