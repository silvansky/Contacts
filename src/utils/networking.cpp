#include "networking.h"

#include "iconstorage.h"
#include "log.h"

#include <QNetworkRequest>
#include <QPixmap>
#include <QImageReader>
#include <QFile>
#include <QVariant>
#include <QApplication>
#include <QSslError>

#ifdef DEBUG_ENABLED
# include <QDebug>
#endif

#include <definitions/resources.h>

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
	connect(nam, SIGNAL(sslErrors(QNetworkReply*,QList<QSslError>)), SLOT(onSslErrors(QNetworkReply*,QList<QSslError>)));
	_cookiePath = qApp->applicationDirPath();
	jar->loadCookies(_cookiePath);
}

NetworkingPrivate::~NetworkingPrivate()
{
	jar->saveCookies(_cookiePath);
	nam->deleteLater();
	loop->deleteLater();
}

void NetworkingPrivate::httpGetAsync(NetworkingPrivate::RequestProperties::Type type, const QUrl &src, QObject *receiver, const char *slot, const char *errorSlot)
{
	QNetworkRequest request;
	request.setUrl(src);
	RequestProperties props;
	props.requestType = RequestProperties::GetReq;
	props.type = type;
	props.url = src;
	props.receiver = receiver;
	props.slot = slot;
	props.errorSlot = errorSlot;
	QNetworkReply *reply = nam->get(request);
	connect(reply, SIGNAL(sslErrors(QList<QSslError>)), SLOT(onSslErrors(QList<QSslError>)));
	requests.insert(reply, props);
}

void NetworkingPrivate::httpPostAsync(const QUrl &src, const QByteArray &data, QObject *receiver, const char *slot, const char *errorSlot)
{
	QNetworkRequest request;
	request.setUrl(src);
	request.setHeader(QNetworkRequest::ContentTypeHeader, "application/octet-stream");
	RequestProperties props;
	props.requestType = RequestProperties::PostReq;
	props.postData = data;
	props.type = RequestProperties::String;
	props.url = src;
	props.receiver = receiver;
	props.slot = slot;
	props.errorSlot = errorSlot;
	QNetworkReply *reply = nam->post(request, data);
	//connect(reply, SIGNAL(sslErrors(QList<QSslError>)), SLOT(onSslErrors(QList<QSslError>)));
	connect(reply, SIGNAL(sslErrors(QList<QSslError>)), reply, SLOT(ignoreSslErrors()));
	requests.insert(reply, props);
}

QImage NetworkingPrivate::httpGetImage(const QUrl &src) const
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

void NetworkingPrivate::httpGetImageAsync(const QUrl &src, QObject *receiver, const char *slot, const char *errorSlot)
{
	httpGetAsync(RequestProperties::Image, src, receiver, slot, errorSlot);
}

QString NetworkingPrivate::httpGetString(const QUrl &src) const
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

void NetworkingPrivate::httpGetStringAsync(const QUrl &src, QObject *receiver, const char *slot, const char *errorSlot)
{
	httpGetAsync(RequestProperties::String, src, receiver, slot, errorSlot);
}

void NetworkingPrivate::setCookiePath(const QString &path)
{
	_cookiePath = path;
	jar->loadCookies(_cookiePath);
}

QString NetworkingPrivate::cookiePath() const
{
	return _cookiePath;
}

void NetworkingPrivate::onFinished(QNetworkReply *reply)
{
	if (requests.contains(reply))
	{
		RequestProperties props = requests.value(reply);
		if (reply->error() == QNetworkReply::NoError)
		{
			// request succeeded
			QVariant redirectedUrl = reply->attribute(QNetworkRequest::RedirectionTargetAttribute);
			QUrl redirectedTo = redirectedUrl.toUrl();
			if (redirectedTo.isValid())
			{
				// guard from infinite redirect loop
				if (redirectedTo != reply->request().url())
				{
					if (props.requestType == RequestProperties::PostReq)
					{
						httpPostAsync(redirectedTo, props.postData, props.receiver, props.slot, props.errorSlot);
					}
					else
					{
						switch (props.type)
						{
						case RequestProperties::Image:
							httpGetImageAsync(redirectedTo, props.receiver, props.slot, props.errorSlot);
							break;
						case RequestProperties::String:
						default:
							httpGetStringAsync(redirectedTo, props.receiver, props.slot, props.errorSlot);
							break;
						}
					}
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
						reader.read(&img);
#ifdef DEBUG_ENABLED
						qDebug() << "[NetworkingPrivate]: Image loaded!";
#endif
						if (props.receiver && props.slot)
							QMetaObject::invokeMethod(props.receiver, props.slot, Qt::AutoConnection, Q_ARG(QUrl, props.url), Q_ARG(QImage, img));
						break;
					}
				case RequestProperties::String:
					{
						QString result;
						result = QString::fromUtf8(reply->readAll().constData());
#ifdef DEBUG_ENABLED
						qDebug() << "[NetworkingPrivate]: String loaded!" << result;
#endif
						if (props.receiver && props.slot)
							QMetaObject::invokeMethod(props.receiver, props.slot, Qt::AutoConnection, Q_ARG(QUrl, props.url), Q_ARG(QString, result));
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
		else
		{
			// request failed
			// TODO: test if "finished()" signal really follows the reply's "error()" signal
			QString replyError = reply->errorString();
			LogError(QString("[NetworkingPrivate::onFinished]: Request for %1 finished with error \"%2\"").arg(props.url.toString(), replyError));
			if (props.receiver && props.errorSlot)
				QMetaObject::invokeMethod(props.receiver, props.errorSlot, Qt::AutoConnection, Q_ARG(QUrl, props.url), Q_ARG(QString, replyError));
		}
	}
}

void NetworkingPrivate::onSslErrors(QNetworkReply *reply, const QList<QSslError> &errors)
{
#ifdef DEBUG_ENABLED
	QStringList errorList;
	foreach (QSslError err, errors)
		errorList << err.errorString();
	qDebug() << QString("[NetworkingPrivate]: Got SSL errors: \n\t%1").arg(errorList.join("\n\t"));
#endif
	reply->ignoreSslErrors(errors);
}

void NetworkingPrivate::onSslErrors(const QList<QSslError> &errors)
{
	Q_UNUSED(errors)
#ifdef DEBUG_ENABLED
	QStringList errorList;
	foreach (QSslError err, errors)
		errorList << err.errorString();
	qDebug() << QString("[NetworkingPrivate]: Got SSL errors: \n\t%1").arg(errorList.join("\n\t"));
#endif
	if (QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender()))
	{
		reply->ignoreSslErrors(errors);
	}
}

// Networking class

NetworkingPrivate *Networking::networkingPrivate = 0;

QImage Networking::httpGetImage(const QUrl& src)
{
	return p()->httpGetImage(src);
}

void Networking::httpGetImageAsync(const QUrl& src, QObject * receiver, const char * slot, const char *errorSlot)
{
	p()->httpGetImageAsync(src, receiver, slot, errorSlot);
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

void Networking::httpGetStringAsync(const QUrl &src, QObject *receiver, const char *slot, const char *errorSlot)
{
	p()->httpGetStringAsync(src, receiver, slot, errorSlot);
}

void Networking::httpPostAsync(const QUrl &src, const QByteArray &data, QObject *receiver, const char *slot, const char *errorSlot)
{
	p()->httpPostAsync(src, data, receiver, slot, errorSlot);
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
