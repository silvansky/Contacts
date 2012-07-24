#ifndef SERVERAPIHANDLER_H
#define SERVERAPIHANDLER_H

#include <QObject>
#include <QUrl>

#include <utils/jid.h>

struct ServiceAuthInfo
{
	bool authorized;
	QString service;
	QString user;
	QString displayName;
	QString password;
	QString authToken;
};

class ServerApiHandler : public QObject
{
	Q_OBJECT
public:
	ServerApiHandler();
	virtual ~ServerApiHandler();
public:
	// registration request
	void sendRegistrationRequest(const Jid &user, const QString &password, const QString &userName);

	// auth through third-party services request
	void sendCheckAuthRequest(const QString &service, const QString &user, const QString &password);
	void sendAuthRequest(const QList<ServiceAuthInfo> &services);
signals:

	// common error

	void requestFailed(const QString &error);

	// registration response

	void registrationSucceeded(const Jid &user);
	void registrationFailed(const QString &reason, const QString &loginError, const QString &passwordError, const QStringList &suggests);

	// auth through third-party services response

	void checkAuthRequestSucceeded(const QString &user, const QString &displayName, const QString &authToken);
	void checkAuthRequestFailed(const QString &user, const QString &reason);

	void authRequestSucceeded(const Jid &user, const QString &password);
	void authRequestFailed(const QString &reason);

protected slots:
	void sendData(const QUrl &url, const QByteArray &data);
	// for Networking callbacks
	void onNetworkRequestFinished(const QUrl &url, const QString &result);
	void onNetworkRequestFailed(const QUrl &url, const QString &errorString);
private:
};

#endif // SERVERAPIHANDLER_H
