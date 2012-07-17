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
		enum ReqType
		{
			PostReq,
			GetReq
		};
		Type type;
		ReqType requestType;
		QByteArray postData;
		QUrl url;
		QObject *receiver;
		const char *slot;
		const char *errorSlot;
	};

public:
	NetworkingPrivate();
	virtual ~NetworkingPrivate();
	void httpGetAsync(RequestProperties::Type type, const QUrl &src, QObject *receiver, const char *slot, const char *errorSlot);
	void httpPostAsync(const QUrl &src, const QByteArray &data, QObject *receiver, const char *slot, const char *errorSlot);
	QImage httpGetImage(const QUrl &src) const;
	void httpGetImageAsync(const QUrl &src, QObject *receiver, const char *slot, const char *errorSlot);
	QString httpGetString(const QUrl &src) const;
	void httpGetStringAsync(const QUrl &src, QObject *receiver, const char *slot, const char *errorSlot);
	//void httpPostAsync(const QUrl &src, const QByteArray &data, QObject *receiver, const char *slot, const char *errorSlot);
	void setCookiePath(const QString &path);
	QString cookiePath() const;
public slots:
	void onFinished(QNetworkReply *reply);
	void onSslErrors(QNetworkReply *reply, const QList<QSslError> &errors);
	void onSslErrors(const QList<QSslError> &errors);
private:
	QNetworkAccessManager * nam;
	QEventLoop * loop;
	QMap<QNetworkReply*, RequestProperties> requests;
	QString _cookiePath;
	CookieJar * jar;
};

#endif // NETWORKING_P_H
