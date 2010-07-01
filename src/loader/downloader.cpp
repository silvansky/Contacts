#include "downloader.h"

#include <QDir>

Downloader::Downloader(const QUrl& url, QString downloadDirectory, QObject *parent) : QObject(parent)
{
	this->url = url;
	directory = downloadDirectory;
	QDir dir(directory);
	if(!dir.exists())
	{
		dir.mkpath(directory);
	}
	if(!directory.endsWith("\\"))
		directory += "\\";

	manager = new QNetworkAccessManager(this);
	//connect(ui.pbtDownload, SIGNAL(clicked()), this, SLOT(downloadFile()));
	progressDialog = new QProgressDialog(NULL);
	progressDialog->setCancelButton(0);
}

Downloader::~Downloader()
{
	if(progressDialog != NULL)
	{
		delete progressDialog;
		progressDialog = NULL;
	}
	if(manager != NULL)
	{
		delete manager;
		manager = NULL;
	}
}

void Downloader::downloadFile()
{
	//QUrl url(ui.lineUrl->text());
	QFileInfo fileInfo(url.path());
	QString fileName = fileInfo.fileName();
	if (fileName.isEmpty())
	{
		emit errorDownload(tr("Filename of update is empty!"));
		return;
		//fileName = "index.html";
	}

	if (QFile::exists(directory + fileName))
	{
		if (QMessageBox::question(NULL, tr("Update already downloaded"),
			tr("There already exists a file called %1 in the current directory. Overwrite?").arg(fileName),
			QMessageBox::Yes|QMessageBox::No, QMessageBox::No)
			== QMessageBox::No)
		{
			emit downloadFinished();
			return;
		}
		QFile::remove(directory + fileName);
	}

	downloadedFile = new QFile(directory + fileName);
	if (!downloadedFile->open(QIODevice::WriteOnly))
	{
		QMessageBox::information(NULL, tr("Download error"),
			tr("Unable to save the file %1: %2.")
			.arg(fileName).arg(downloadedFile->errorString()));
		delete downloadedFile;
		downloadedFile = 0;

		emit errorDownload(tr("Unable to save the file %1: %2.").arg(directory + fileName).arg(downloadedFile->errorString()));
		return;
	}

	QNetworkRequest req(url);

	reply = manager->get(req);
	connect(reply, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(downloadError(QNetworkReply::NetworkError)));
	connect(reply, SIGNAL(finished()),this, SLOT(getReplyFinished()));
	connect(reply, SIGNAL(readyRead()), this, SLOT(readyReadReply()));
	connect(reply, SIGNAL(downloadProgress(qint64, qint64)), this, SLOT(downloadProgress(qint64, qint64)));

	progressDialog->setWindowTitle(tr("Download progress..."));
	progressDialog->setLabelText(tr("Downloading %1.").arg(fileName));
}

void Downloader::readyReadReply()
{
	downloadedFile->write(reply->readAll());
	downloadedFile->flush();
}
void Downloader::getReplyFinished()
{
	if(reply->error())
	{
		if(downloadedFile != NULL)
		{
			downloadedFile->flush();
			downloadedFile->remove();
			downloadedFile->close();
			delete downloadedFile;
			downloadedFile = 0;
		}
		progressDialog->hide();
		reply->deleteLater();
		emit downloadCanceled(reply->errorString());
	}
	else
	{
		if(downloadedFile != NULL)
		{
			downloadedFile->flush();
			downloadedFile->close();
			delete downloadedFile;
			downloadedFile = 0;
		}
		progressDialog->hide();
		reply->deleteLater();
		emit downloadFinished();
	}
}

void Downloader::downloadError(QNetworkReply::NetworkError err)
{
	//QString reason = tr("Update canceled! ") + reply->errorString();
	//emit downloadCanceled(reason);
}
void Downloader::downloadProgress(qint64 bytesReceived, qint64 bytesTotal)
{
	progressDialog->setMaximum(bytesTotal);
	progressDialog->setValue(bytesReceived);
}
