#include "networking.h"

#include <QNetworkRequest>
#include <QPixmap>
#include <QImageReader>
#include <QVariant>
#include <definitions/resources.h>
#include "iconstorage.h"
#include "log.h"

// private class for real manipulations

#include "networking_p.h"

NetworkingPrivate::NetworkingPrivate()
{
	nam = new QNetworkAccessManager();
	loop = new QEventLoop();
	connect(nam, SIGNAL(finished(QNetworkReply*)), loop, SLOT(quit()));
	connect(nam, SIGNAL(finished(QNetworkReply*)), SLOT(onFinished(QNetworkReply*)));
}

NetworkingPrivate::~NetworkingPrivate()
{
	nam->deleteLater();
	loop->deleteLater();
}

QImage NetworkingPrivate::httpGetImage(const QUrl& src) const
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

void NetworkingPrivate::httpGetImageAsync(const QUrl& src, QObject * receiver, const char * slot)
{
	QNetworkRequest request;
	request.setUrl(src);
	QPair<QObject*, QPair<QUrl, const char *> > obj;
	obj.first = receiver;
	obj.second.first = src;
	obj.second.second = slot;
	QNetworkReply * reply = nam->get(request);
	requests.insert(reply, obj);
}

QString NetworkingPrivate::httpGetString(const QUrl& src) const
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

void NetworkingPrivate::onFinished(QNetworkReply* reply)
{
	if (requests.contains(reply))
	{
		QPair<QObject*, QPair<QUrl, const char *> > obj = requests.value(reply);
		QImage img;
		QImageReader reader(reply);
		if (reply->error() == QNetworkReply::NoError)
			reader.read(&img);
		else
			Log(QString("reply->error() == %1").arg(reply->error()));
		if (obj.first)
		{
			QMetaObject::invokeMethod(obj.first, obj.second.second, Qt::DirectConnection, Q_ARG(QUrl, obj.second.first), Q_ARG(QImage, img));
		}
		requests.remove(reply);
		reply->deleteLater();
	}
}

// Networking class

NetworkingPrivate * Networking::networkingPrivate = 0;

QImage Networking::httpGetImage(const QUrl& src)
{
	init();
	return networkingPrivate->httpGetImage(src);
}

void Networking::httpGetImageAsync(const QUrl& src, QObject * receiver, const char * slot)
{
	init();
	networkingPrivate->httpGetImageAsync(src, receiver, slot);
}

bool Networking::insertPixmap(const QUrl& src, QObject* target, const QString& property)
{
	init();
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
	init();
	return networkingPrivate->httpGetString(src);
}

void Networking::init()
{
	if (!networkingPrivate)
		networkingPrivate = new NetworkingPrivate;
}
