#include "serverapihandler.h"

#include <utils/networking.h>

// server defines

#define REG_SERVER_PROTOCOL   "https"
#define REG_SERVER            "reg.tx.xmpp.rambler.ru"

#define REG_SERVER_PARAMS     "f=xml"
#define REG_SERVER_COUNTER    "counter"

#define REG_SERVER_QUERY              REG_SERVER_PROTOCOL"://"REG_SERVER"/?"REG_SERVER_PARAMS
#define REG_SERVER_COUNTER_QUERY      REG_SERVER_PROTOCOL"://"REG_SERVER"/"REG_SERVER_COUNTER

// urlencode

QString urlencode(const QString &s)
{
	return QString::fromUtf8(QUrl::toPercentEncoding(s).data());
}

// impl

ServerApiHandler::ServerApiHandler() :
	QObject(NULL)
{
}

void ServerApiHandler::sendRegistrationRequest(const Jid &user, const QString &password, const QString &userName)
{
	QUrl request(REG_SERVER_QUERY);
	QString postData = QString("login=%1&domain=%2&password=%3&name=%4").arg(urlencode(user.node()), urlencode(user.domain()), urlencode(password), urlencode(userName));
	Networking::httpPostAsync(request, postData.toUtf8(), this, NW_SLOT(onNetworkRequestFinished), NW_SLOT(onNetworkRequestFailed));
}

void ServerApiHandler::sendCheckAuthRequest(const QString &service, const QString &login, const QString &password)
{
	Q_UNUSED(service)
	Q_UNUSED(login)
	Q_UNUSED(password)
}

void ServerApiHandler::sendAuthRequest(const QList<ServiceAuthInfo> &services)
{
	Q_UNUSED(services)
}


void ServerApiHandler::onNetworkRequestFinished(const QUrl &url, const QString &result)
{
	Q_UNUSED(url)
	Q_UNUSED(result)
}

void ServerApiHandler::onNetworkRequestFailed(const QUrl &url, const QString &errorString)
{
	Q_UNUSED(url)
	Q_UNUSED(errorString)
}
