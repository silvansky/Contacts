#ifndef NETWORKING_P_H
#define NETWORKING_P_H

#include <QNetworkAccessManager>
#include <QNetworkCookieJar>
#include <QNetworkReply>
#include <QEventLoop>
#include <QImage>
#include <QObject>

class CookieJar : public QNetworkCookieJar
{
	Q_OBJECT
public:
	CookieJar();
	~CookieJar();
	void loadCookies(const QString & cookiePath);
	void saveCookies(const QString & cookiePath);
};

class NetworkingPrivate : public QObject
{
	Q_OBJECT
	struct RequestProperties
	{
		enum Type
		{
			String,
			Image
		};
		Type type;
		QUrl url;
		QObject *receiver;
		const char *slot;
	};

public:
	NetworkingPrivate();
	virtual ~NetworkingPrivate();
	void httpGetAsync(RequestProperties::Type type, const QUrl& src, QObject * receiver, const char * slot);
	QImage httpGetImage(const QUrl& src) const;
	void httpGetImageAsync(const QUrl& src, QObject * receiver, const char * slot);
	QString httpGetString(const QUrl& src) const;
	void httpGetStringAsync(const QUrl& src, QObject * receiver, const char * slot);
	void setCookiePath(const QString & path);
	QString cookiePath() const;
public slots:
	void onFinished(QNetworkReply* reply);
private:
	QNetworkAccessManager * nam;
	QEventLoop * loop;
	QMap<QNetworkReply*, RequestProperties> requests;
	QString _cookiePath;
	CookieJar * jar;
};

#endif // NETWORKING_P_H
