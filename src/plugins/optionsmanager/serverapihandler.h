#ifndef SERVERAPIHANDLER_H
#define SERVERAPIHANDLER_H

#include <QObject>
#include <QUrl>

#include <utils/jid.h>

struct ServiceAuthInfo
{
	QString service;
	QString user;
	QString password;
	QString authToken;
};

class ServerApiHandler : public QObject
{
	Q_OBJECT
public:
	ServerApiHandler();
public:
	// registration request
	void sendRegistrationRequest(const Jid &user, const QString &password, const QString &userName);
	// auth through third-party services request
	void sendCheckAuthRequest(const QString &service, const QString &login, const QString &password);
	void sendAuthRequest(const QList<ServiceAuthInfo> &services);
	//
signals:
	// registration response
	void registrationSucceeded(const Jid &user);
	void registrationFailed(const QString &reason, const QString &loginError, const QString &passwordError, const QStringList &suggests);
	void registrationRequestFailed(const QString &error);
	// auth through third-party services response
	void checkAuthRequestSucceeded(const Jid &user, const QString &authToken);
	void checkAuthRequestFailed(const QString &reason);
	void checkAuthRequestRequestFailed(const QString &error);
	void authRequestSucceeded(const Jid &user, const QString &password);
	void authRequestFailed(const QString &reason);
	void authRequestRequestFailed(const QString &error);
protected slots:
	void onNetworkRequestFinished(const QUrl &url, const QString &result);
	void onNetworkRequestFailed(const QUrl &url, const QString &errorString);
private:
};

#endif // SERVERAPIHANDLER_H
