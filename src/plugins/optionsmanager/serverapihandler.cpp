#include "serverapihandler.h"

#include <utils/networking.h>

#include <QDomDocument>
#include <QTextDocument>

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

ServerApiHandler::~ServerApiHandler()
{
}

void ServerApiHandler::sendRegistrationRequest(const Jid &user, const QString &password, const QString &userName)
{
	QDomDocument doc;
	QDomElement root = doc.createElement("register");
	doc.appendChild(root);

	QDomElement loginEl = doc.createElement("login");
	loginEl.appendChild(doc.createTextNode(Qt::escape(user.node())));
	root.appendChild(loginEl);

	QDomElement domainEl = doc.createElement("domain");
	domainEl.appendChild(doc.createTextNode(Qt::escape(user.domain())));
	root.appendChild(domainEl);

	QDomElement nameEl = doc.createElement("name");
	nameEl.appendChild(doc.createTextNode(Qt::escape(userName)));
	root.appendChild(nameEl);

	QDomElement passEl = doc.createElement("password");
	passEl.appendChild(doc.createTextNode(Qt::escape(password)));
	root.appendChild(passEl);

	sendData(doc.toString().toUtf8());
}

void ServerApiHandler::sendCheckAuthRequest(const QString &service, const QString &user, const QString &password)
{
	QDomDocument doc;
	QDomElement root = doc.createElement("authcheck");
	doc.appendChild(root);

	QDomElement serviceEl = doc.createElement("service");
	serviceEl.appendChild(doc.createTextNode(Qt::escape(service)));
	root.appendChild(serviceEl);

	QDomElement userEl = doc.createElement("user");
	userEl.appendChild(doc.createTextNode(Qt::escape(user)));
	root.appendChild(userEl);

	QDomElement passEl = doc.createElement("password");
	passEl.appendChild(doc.createTextNode(Qt::escape(password)));
	root.appendChild(passEl);

	sendData(doc.toString().toUtf8());
}

void ServerApiHandler::sendAuthRequest(const QList<ServiceAuthInfo> &services)
{
	if (services.isEmpty())
	{
		emit requestFailed(tr("No accounts provided"));
		return;
	}

	QDomDocument doc;
	QDomElement root = doc.createElement("fastregister");
	doc.appendChild(root);

	foreach (ServiceAuthInfo info, services)
	{
		if (info.authorized)
		{
			QDomElement accountEl = doc.createElement("account");

			QDomElement serviceEl = doc.createElement("service");
			serviceEl.appendChild(doc.createTextNode(Qt::escape(info.service)));
			accountEl.appendChild(serviceEl);

			QDomElement userEl = doc.createElement("user");
			userEl.appendChild(doc.createTextNode(Qt::escape(info.user)));
			accountEl.appendChild(userEl);

			QDomElement authtokenEl = doc.createElement("authtoken");
			authtokenEl.appendChild(doc.createTextNode(Qt::escape(info.authToken)));
			accountEl.appendChild(authtokenEl);

			root.appendChild(accountEl);
		}
	}

	sendData(doc.toString().toUtf8());
}


void ServerApiHandler::sendData(const QByteArray &data)
{
	Networking::httpPostAsync(QUrl(REG_SERVER_QUERY), data, this, NW_SLOT(onNetworkRequestFinished), NW_SLOT(onNetworkRequestFailed));
}

void ServerApiHandler::onNetworkRequestFinished(const QUrl &url, const QString &result)
{
	Q_UNUSED(url)
	Q_UNUSED(result)
	// TODO: parse resulting XML...
}

void ServerApiHandler::onNetworkRequestFailed(const QUrl &url, const QString &errorString)
{
	Q_UNUSED(url)
	emit requestFailed(errorString);
}
