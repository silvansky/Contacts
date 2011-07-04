#include "networking.h"

#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QEventLoop>
#include <QPixmap>
#include <QImageReader>
#include <QVariant>
#include <definitions/resources.h>
#include "iconstorage.h"
#include "log.h"

// private class for real manipulations

class Networking::NetworkingPrivate : public QObject
{
public:
	NetworkingPrivate()
	{
		nam = new QNetworkAccessManager();
		loop = new QEventLoop();
		connect(nam, SIGNAL(finished(QNetworkReply*)), loop, SLOT(quit()));
	}
	~NetworkingPrivate()
	{
		nam->deleteLater();
		loop->deleteLater();
	}
	QImage httpGetImage(const QUrl& src) const
	{
		QNetworkRequest request;
		request.setUrl(src);
		QNetworkReply * reply = nam->get(request);
		loop->exec();
		QImage img;
		QImageReader reader(reply);
		if (reply->error() == QNetworkReply::NoError)
			reader.read(&img);
		else
			Log(QString("reply->error() == %1").arg(reply->error()));
		reply->deleteLater();
		return img;
	}
	QString httpGetString(const QUrl& src) const
	{
		QNetworkRequest request;
		request.setUrl(src);
		QNetworkReply * reply = nam->get(request);
		loop->exec();
		QString answer;
		if (reply->error() == QNetworkReply::NoError)
			answer = QString::fromUtf8(reply->readAll());
		else
			Log(QString("reply->error() == %1").arg(reply->error()));
		reply->deleteLater();
		return answer;
	}
private:
	QNetworkAccessManager * nam;
	QEventLoop * loop;
};

// Networking class

Networking::NetworkingPrivate Networking::networkingPrivate;

QImage Networking::httpGetImage(const QUrl& src)
{
	return networkingPrivate.httpGetImage(src);
}

bool Networking::insertPixmap(const QUrl& src, QObject* target, const QString& property)
{
	QImage img = httpGetImage(src);
	if (!img.isNull())
	{
		IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->removeAutoIcon(target);
		return target->setProperty(property.toLatin1(), QVariant(QPixmap::fromImage(img)));
	}
	else
		return false;
}

QString Networking::httpGetString(const QUrl& src)
{
	return networkingPrivate.httpGetString(src);
}
