#ifndef DOWNLOADER_H
#define DOWNLOADER_H

#include <QObject>
#include <QUrl>
#include <QFileInfo>
#include <QMessageBox>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QProgressDialog>

class Downloader : public QObject
{
	Q_OBJECT

public:
	Downloader(const QUrl& url, QString downloadDirectory, QObject *parent);
	~Downloader();

signals:
	void errorDownload(QString errString);
	void downloadFinished();
	void downloadCanceled(QString reason);


public slots:
	void downloadFile();

protected slots:
	void readyReadReply();
	void getReplyFinished();
	void downloadError(QNetworkReply::NetworkError err);
	void downloadProgress(qint64 bytesReceived, qint64 bytesTotal);

private:
	QFile* downloadedFile;
	QNetworkAccessManager* manager;
	QNetworkReply* reply;
	QProgressDialog *progressDialog;
	QUrl url;
	QString directory;
};

#endif // DOWNLOADER_H
