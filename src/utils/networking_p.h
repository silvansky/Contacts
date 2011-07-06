#ifndef NETWORKING_P_H
#define NETWORKING_P_H

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QEventLoop>
#include <QImage>
#include <QObject>

class NetworkingPrivate : public QObject
{
	Q_OBJECT
public:
	NetworkingPrivate();
	~NetworkingPrivate();
	QImage httpGetImage(const QUrl& src) const;
	void httpGetImageAsync(const QUrl& src, QObject * receiver, const char * slot);
	QString httpGetString(const QUrl& src) const;
public slots:
	void onFinished(QNetworkReply* reply);
private:
	QNetworkAccessManager * nam;
	QEventLoop * loop;
	QMap<QNetworkReply*, QPair<QObject*, QPair<QUrl, const char *> > > requests;
};

#endif // NETWORKING_P_H
