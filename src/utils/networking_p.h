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
public:
	NetworkingPrivate();
	~NetworkingPrivate();
	QImage httpGetImage(const QUrl& src) const;
	void httpGetImageAsync(const QUrl& src, QObject * receiver, const char * slot);
	QString httpGetString(const QUrl& src) const;
	void setCookiePath(const QString & path);
	QString cookiePath() const;
public slots:
	void onFinished(QNetworkReply* reply);
private:
	QNetworkAccessManager * nam;
	QEventLoop * loop;
	QMap<QNetworkReply*, QPair<QObject*, QPair<QUrl, const char *> > > requests;
	QString _cookiePath;
	CookieJar * jar;
};

#endif // NETWORKING_P_H
