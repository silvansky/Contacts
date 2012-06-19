#include "networking.h"

#include <QNetworkRequest>
#include <QPixmap>
#include <QImageReader>
#include <QFile>
#include <QVariant>
#include <QApplication>
#include <definitions/resources.h>
#include "iconstorage.h"
#include "log.h"

// internal header
#include "networking_p.h"

// class CookieJar - cookie storage
CookieJar::CookieJar() : QNetworkCookieJar()
{

}

CookieJar::~CookieJar()
{

}

void CookieJar::loadCookies(const QString & cookiePath)
{
	QFile file(cookiePath + "/cookies.dat");
	if (file.open(QIODevice::ReadOnly))
	{
		QByteArray cookies = file.readAll();
		setAllCookies(QNetworkCookie::parseCookies(cookies));
	}
	else
	{
		LogError("[CookieJar] Failed to load cookies: " + file.errorString());
	}
}

void CookieJar::saveCookies(const QString & cookiePath)
{
	QFile file(cookiePath + "/cookies.dat");
	if (file.open(QIODevice::WriteOnly | QIODevice::Truncate))
	{
		QByteArray cookies;
		foreach (QNetworkCookie cookie, allCookies())
			cookies += cookie.toRawForm();
		file.write(cookies);
	}
	else
	{
		LogError("[CookieJar] Failed to save cookies: " + file.errorString());
	}
}

// private class for real manipulations
NetworkingPrivate::NetworkingPrivate()
{
	nam = new QNetworkAccessManager();
	jar = new CookieJar();
	nam->setCookieJar(jar);
	loop = new QEventLoop();
	connect(nam, SIGNAL(finished(QNetworkReply*)), loop, SLOT(quit()));
	connect(nam, SIGNAL(finished(QNetworkReply*)), SLOT(onFinished(QNetworkReply*)));
}

NetworkingPrivate::~NetworkingPrivate()
{
	jar->saveCookies(_cookiePath);
	nam->deleteLater();
	loop->deleteLater();
}

void NetworkingPrivate::httpGetAsync(NetworkingPrivate::RequestProperties::Type type, const QUrl &src, QObject *receiver, const char *slot)
{
	QNetworkRequest request;
	request.setUrl(src);
	RequestProperties props;
	props.type = type;
	props.url = src;
	props.receiver = receiver;
	props.slot = slot;
	QNetworkReply * reply = nam->get(request);
	requests.insert(reply, props);
}

QImage NetworkingPrivate::httpGetImage(const QUrl& src) const
{
	QNetworkRequest request;
	request.setUrl(src);
	QNetworkReply * reply = nam->get(request);
	loop->exec();
	QVariant redirectedUrl = reply->attribute(QNetworkRequest::RedirectionTargetAttribute);
	QUrl redirectedTo = redirectedUrl.toUrl();
	if (redirectedTo.isValid())
	{
		// guard from infinite redirect loop
		if (redirectedTo != reply->request().url())
		{
			return httpGetImage(redirectedTo);
		}
		else
		{
			LogError("[NetworkingPrivate] Infinite redirect loop at " + redirectedTo.toString());
			return QImage();
		}
	}
	else
	{
		QImage img;
		QImageReader reader(reply);
		if (reply->error() == QNetworkReply::NoError)
			reader.read(&img);
		else
			LogError(QString("[NetworkingPrivate] Reply error: %1").arg(reply->error()));
		reply->deleteLater();
		return img;
	}
}

void NetworkingPrivate::httpGetImageAsync(const QUrl& src, QObject * receiver, const char * slot)
{
	httpGetAsync(RequestProperties::Image, src, receiver, slot);
}

QString NetworkingPrivate::httpGetString(const QUrl& src) const
{
	QNetworkRequest request;
	request.setUrl(src);
	QNetworkReply * reply = nam->get(request);
	loop->exec();
	QVariant redirectedUrl = reply->attribute(QNetworkRequest::RedirectionTargetAttribute);
	QUrl redirectedTo = redirectedUrl.toUrl();
	if (redirectedTo.isValid())
	{
		// guard from infinite redirect loop
		if (redirectedTo != reply->request().url())
		{
			return httpGetString(redirectedTo);
		}
		else
		{
			LogError("[NetworkingPrivate] Infinite redirect loop at " + redirectedTo.toString());
			return QString::null;
		}
	}
	else
	{
		QString answer;
		if (reply->error() == QNetworkReply::NoError)
			answer = QString::fromUtf8(reply->readAll());
		else
			LogError(QString("[NetworkingPrivate] Reply error: %1").arg(reply->error()));
		reply->deleteLater();
		return answer;
	}
}

void NetworkingPrivate::httpGetStringAsync(const QUrl& src, QObject * receiver, const char * slot)
{
	httpGetAsync(RequestProperties::String, src, receiver, slot);
}

void NetworkingPrivate::setCookiePath(const QString & path)
{
	_cookiePath = path;
	jar->loadCookies(_cookiePath);
}

QString NetworkingPrivate::cookiePath() const
{
	return _cookiePath;
}

void NetworkingPrivate::onFinished(QNetworkReply* reply)
{
	if (requests.contains(reply))
	{
		RequestProperties props = requests.value(reply);
		QVariant redirectedUrl = reply->attribute(QNetworkRequest::RedirectionTargetAttribute);
		QUrl redirectedTo = redirectedUrl.toUrl();
		if (redirectedTo.isValid())
		{
			// guard from infinite redirect loop
			if (redirectedTo != reply->request().url())
			{
				httpGetImageAsync(redirectedTo, props.receiver, props.slot);
			}
			else
			{
				LogError("[NetworkingPrivate] Infinite redirect loop at " + redirectedTo.toString());
			}
		}
		else
		{
			switch (props.type)
			{
			case RequestProperties::Image:
				{
					QImage img;
					QImageReader reader(reply);
					if (reply->error() == QNetworkReply::NoError)
					{
						reader.read(&img);
						if (props.receiver && props.slot)
							QMetaObject::invokeMethod(props.receiver, props.slot, Qt::DirectConnection, Q_ARG(QUrl, props.url), Q_ARG(QImage, img));
					}
					else
						LogError(QString("[NetworkingPrivate] Reply error: %1").arg(reply->error()));
					break;
				}
			case RequestProperties::String:
				{
					QString result;
					if (reply->error() == QNetworkReply::NoError)
					{
						result = QString::fromUtf8(reply->readAll().constData());
						if (props.receiver && props.slot)
							QMetaObject::invokeMethod(props.receiver, props.slot, Qt::DirectConnection, Q_ARG(QUrl, props.url), Q_ARG(QString, result));
					}
					else
						LogError(QString("[NetworkingPrivate] Reply error: %1").arg(reply->error()));
					break;
				}
			default:
				break;
			}


		}
		requests.remove(reply);
		reply->deleteLater();
		jar->saveCookies(_cookiePath);
	}
}

// Networking class

NetworkingPrivate *Networking::networkingPrivate = 0;

QImage Networking::httpGetImage(const QUrl& src)
{
	return p()->httpGetImage(src);
}

void Networking::httpGetImageAsync(const QUrl& src, QObject * receiver, const char * slot)
{
	p()->httpGetImageAsync(src, receiver, slot);
}

bool Networking::insertPixmap(const QUrl& src, QObject* target, const QString& property)
{
	QImage img = httpGetImage(src);
	if (!img.isNull())
	{
		IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->removeAutoIcon(target);
		return target->setProperty(property.toLatin1(), QVariant(QPixmap::fromImage(img)));
	}
	return false;
}

QString Networking::httpGetString(const QUrl& src)
{
	return p()->httpGetString(src);
}

void Networking::httpGetStringAsync(const QUrl &src, QObject *receiver, const char *slot)
{
	p()->httpGetStringAsync(src, receiver, slot);
}

QString Networking::cookiePath()
{
	return p()->cookiePath();
}

void Networking::setCookiePath(const QString & path)
{
	p()->setCookiePath(path);
}

NetworkingPrivate *Networking::p()
{
	if (!networkingPrivate)
		networkingPrivate = new NetworkingPrivate;
	return networkingPrivate;
}
