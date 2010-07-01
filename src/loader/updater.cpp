#include "updater.h"

#include <QFileInfo>
#include <QUrl>
#include <QApplication>
#include <Qdir>
#include <QProcess>

#include "../utils/unzipfile.h"

Updater::Updater(QObject *parent)
: QObject(parent)
{
	size = 0;
	isForceUpdate = false;
	downloader = NULL;
}

Updater::~Updater()
{

}

QString Updater::getUpdateFilename() const
{
	QFileInfo fileInfo(url.path());
	QString fileName = fileInfo.fileName();
	return fileName;
}

// ПОПОВ проверка обновления
bool Updater::checkUpdate()
{
	//QNetworkAccessManager* manager = new QNetworkAccessManager(this);

	////apiUrl = "http://nakarte.rambler.ru/locationxml/location.xml";
	//QUrl apiUrl = "file://c:/updateVirtus.xml";
	////requestString = "method=getQuote&format=xml";
	//QNetworkRequest request(apiUrl);

	//reply = manager->get(request);
	//connect(reply, SIGNAL(finished()),this, SLOT(getReplyFinished()));
	//connect(reply, SIGNAL(readyRead()), this, SLOT(readyReadReply()));


	QNetworkAccessManager *manager = new QNetworkAccessManager(this);
	connect(manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(replyFinished(QNetworkReply*)));
	manager->get(QNetworkRequest(QUrl("http://ie.rambler.ru/livestream/virtusupdate/update.xml")));


	//QString errMsg; int errLine=0, errCol=0;

	//QFile updateInfoFile("c:\\updateVirtus.xml");
	//if (updateInfoFile.exists() && updateInfoFile.open(QIODevice::ReadOnly))
	//{
	//	QDomDocument xml;
	//	if (xml.setContent(&updateInfoFile, &errMsg, &errLine, &errCol))
	//	{
	//		QDomElement updateElem = xml.firstChildElement("update");
	//		if (!updateElem.isNull())// && updateElem.attribute("streamJid")==streamJid().pBare())
	//		{
	//			isForceUpdate = updateElem.attribute("forceUpdate", "false").toLower() == "true" ? true : false;

	//			QDomElement node;
	//			node = updateElem.namedItem("version").toElement();
	//			if(!node.isNull())
	//			{
	//				version = node.text();
	//			}
	//			node = updateElem.namedItem("size").toElement();
	//			if(!node.isNull())
	//			{
	//				size = node.text().toInt();
	//			}
	//			node = updateElem.namedItem("url").toElement();
	//			if(!node.isNull())
	//			{
	//				url.setUrl(node.text());
	//			}
	//			node = updateElem.namedItem("description").toElement();
	//			if(!node.isNull())
	//			{
	//				description = node.text();
	//			}
	//		}
	//	}
	//	updateInfoFile.close();
	//}

	//if(isForceUpdate)
	//	emit forceUpdate();

	return true;
}

void Updater::readyReadReply()
{
	//QString answer = QString::fromUtf8(reply->readAll());
}
void Updater::getReplyFinished()
{
	//reply->deleteLater();
}
void Updater::replyFinished(QNetworkReply* reply)
{
	QString answer = QString::fromUtf8(reply->readAll());
	//QMessageBox::information(NULL, "", answer);

	QString errMsg; int errLine=0, errCol=0;

	QDomDocument xml;
	if (xml.setContent(answer, &errMsg, &errLine, &errCol))
	{
		QDomElement updateElem = xml.firstChildElement("update");
		if (!updateElem.isNull())
		{
			isForceUpdate = updateElem.attribute("forceUpdate", "false").toLower() == "true" ? true : false;

			QDomElement node;
			node = updateElem.namedItem("version").toElement();
			if(!node.isNull())
			{
				version = node.text();
			}
			node = updateElem.namedItem("size").toElement();
			if(!node.isNull())
			{
				size = node.text().toInt();
			}
			node = updateElem.namedItem("url").toElement();
			if(!node.isNull())
			{
				url.setUrl(node.text());
			}
			node = updateElem.namedItem("description").toElement();
			if(!node.isNull())
			{
				description = node.text();
			}
		}
	}

	if(isForceUpdate)
		emit forceUpdate();
}


void Updater::update()
{
	downloadUpdate();
}

void Updater::downloadUpdate()
{
	//QString updatePath = qApp->applicationDirPath() + "/update";
	QString updatePath = QDir::tempPath() + "\\virtus\\update\\";
	downloader = new Downloader(url, updatePath, this);
	connect(downloader, SIGNAL(downloadFinished()), this, SLOT(setUpdate()));
	//connect(downloader, SIGNAL(downloadFinished()), this, SIGNAL(updateFinished()));
	connect(downloader, SIGNAL(downloadCanceled(QString)), this, SLOT(cancelUpdate(QString)));
	downloader->downloadFile();
}
bool Updater::setUpdate()
{
	QString uFileName = getUpdateFilename();
	if(uFileName.isNull() || uFileName.isEmpty())
	{
		emit updateFinished(tr("Update filename is empty!"), false);
		return false;
	}

	//uFileName = "virtus.zip";

	QString updatePath = QDir::tempPath() + "\\virtus\\update\\";
	//UnzipFile* uz = new UnzipFile(qApp->applicationDirPath() + "/update/virtus.zip");
	//UnzipFile* uz = new UnzipFile(qApp->applicationDirPath() + "/update/" + uFileName);
	UnzipFile* uz = new UnzipFile(updatePath + uFileName);
	QList<QString> files = uz->fileNames();

	QString updaterName;
	foreach(QString fName, files)
	{
		if(fName.contains("UpdateVirtus.exe", Qt::CaseInsensitive))
		{
			updaterName = fName;
			break;
		}
	}

	if(updaterName.isEmpty())
	{
		emit updateFinished(tr("UpdateVirtus.exe not found!"), false);
		//emit cancelUpdate(tr("UpdateVirtus.exe not found!"));
		return false;
	}

	//QString updaterFullName = qApp->applicationDirPath() + "/update/" + updaterName;
	QString updaterFullName = updatePath + updaterName;

	if (QFile::exists(updaterFullName))
	{
		QFile::remove(updaterFullName);
	}

	QFile* updaterFile = new QFile(updaterFullName);
	if (!updaterFile->open(QIODevice::WriteOnly))
	{
		delete updaterFile;
		updaterFile = 0;
		emit cancelUpdate(tr("UpdateVirtus.exe is broken!"));
		return false;
	}

	updaterFile->write(uz->fileData(updaterName));
	updaterFile->flush();

	updaterFile->close();
	delete updaterFile;
	updaterFile = 0;


  // Скачали обновление, вытащили из архива обнвитель и отправляем сигнал.
	// По этому сигналу закрывается загрузчик Виртуса и запускается обновитель
	emit updateFinished(tr("Update in progress!"), true);
	return true;
}

void Updater::cancelUpdate(QString reason)
{
	emit updateFinished(reason, false);
	//updateCanceled(reason);
}
