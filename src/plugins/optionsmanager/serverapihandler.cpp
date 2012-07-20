#include "serverapihandler.h"

#include <utils/networking.h>

#include <QVariant>
#include <QStringList>
#include <QDomDocument>
#include <QTextDocument>

#ifdef DEBUG_ENABLED
# include <QDebug>
#endif

// server defines

#define REG_SERVER_PROTOCOL           "https"
#define REG_SERVER                    "reg.tx.xmpp.rambler.ru"

#define REG_SERVER_PARAMS             "f=xml"
#define REG_SERVER_COUNTER            "counter"

#define REG_SERVER_QUERY              REG_SERVER_PROTOCOL"://"REG_SERVER"/?"REG_SERVER_PARAMS
#define REG_SERVER_COUNTER_QUERY      REG_SERVER_PROTOCOL"://"REG_SERVER"/"REG_SERVER_COUNTER

#define AUTH_SERVER_PROTOCOL          "http"
#define AUTH_SERVER                   "auth.tx01.xmpp.rambler.ru"

#define AUTH_SERVER_PARAMS            "f=xml"

#define AUTH_SERVER_VERIFY_QUERY      AUTH_SERVER_PROTOCOL"://"AUTH_SERVER"/verify?"AUTH_SERVER_PARAMS
#define AUTH_SERVER_REGISTER_QUERY    AUTH_SERVER_PROTOCOL"://"AUTH_SERVER"/create-user?"AUTH_SERVER_PARAMS

// urlencode

QString urlencode(const QString &s)
{
	return QString::fromUtf8(QUrl::toPercentEncoding(s).data());
}

// impl

ServerApiHandler::ServerApiHandler() :
	QObject(NULL)
{
	setProperty(NW_CONTENT_TYPE_PROP, "text/xml");
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

	sendData(QUrl(REG_SERVER_QUERY), doc.toString().toUtf8());
}

void ServerApiHandler::sendCheckAuthRequest(const QString &service, const QString &user, const QString &password)
{
	QDomDocument doc;
	QDomElement root = doc.createElement("verify");
	doc.appendChild(root);

	QDomElement serviceEl = doc.createElement("service");
	serviceEl.appendChild(doc.createTextNode(Qt::escape(service)));
	root.appendChild(serviceEl);

	QDomElement userEl = doc.createElement("login");
	userEl.appendChild(doc.createTextNode(Qt::escape(user)));
	root.appendChild(userEl);

	QDomElement passEl = doc.createElement("password");
	passEl.appendChild(doc.createTextNode(Qt::escape(password)));
	root.appendChild(passEl);

#ifdef DEBUG_ENABLED
	qDebug() << QString("Sending check auth request with XML:\n%1").arg(doc.toString());
#endif

	sendData(QUrl(AUTH_SERVER_VERIFY_QUERY), doc.toString().toUtf8());
}

void ServerApiHandler::sendAuthRequest(const QList<ServiceAuthInfo> &services)
{
	if (services.isEmpty())
	{
		emit requestFailed(tr("No accounts provided"));
		return;
	}

	QDomDocument doc;
	QDomElement root = doc.createElement("create-user");
	doc.appendChild(root);

	foreach (ServiceAuthInfo info, services)
	{
		if (info.authorized)
		{
			QDomElement accountEl = doc.createElement("account");

			QDomElement serviceEl = doc.createElement("service");
			serviceEl.appendChild(doc.createTextNode(Qt::escape(info.service)));
			accountEl.appendChild(serviceEl);

			QDomElement userEl = doc.createElement("login");
			userEl.appendChild(doc.createTextNode(Qt::escape(info.user)));
			accountEl.appendChild(userEl);

			QDomElement authtokenEl = doc.createElement("verification-token");
			authtokenEl.appendChild(doc.createTextNode(Qt::escape(info.authToken)));
			accountEl.appendChild(authtokenEl);

			root.appendChild(accountEl);
		}
	}

	sendData(QUrl(AUTH_SERVER_REGISTER_QUERY), doc.toString().toUtf8());
}


void ServerApiHandler::sendData(const QUrl &url, const QByteArray &data)
{
	Networking::httpPostAsync(url, data, this, NW_SLOT(onNetworkRequestFinished), NW_SLOT(onNetworkRequestFailed));
}

void ServerApiHandler::onNetworkRequestFinished(const QUrl &url, const QString &result)
{
	// TODO: parse resulting XML...
#ifdef DEBUG_ENABLED
	qDebug() << QString("Got answer from %1:\n%2").arg(url.toString(), result);
#endif
	QDomDocument doc;
	if (doc.setContent(result))
	{
		// parsing verification
		QDomElement verifyEl = doc.firstChildElement("verify");
		if (!verifyEl.isNull())
		{
			bool ok = false;
			QString errorText;
			QString user;
			QString token;
			QDomElement successEl = verifyEl.firstChildElement("success");
			if (!successEl.isNull())
			{
				ok = (successEl.text().toLower() == "true");
			}
			QDomElement errorEl = verifyEl.firstChildElement("error");
			if (!errorEl.isNull())
			{
				errorText = errorEl.text();
			}
			QDomElement loginEl = verifyEl.firstChildElement("login");
			if (!loginEl.isNull())
			{
				user = loginEl.text();
			}
			QDomElement tokenEl = verifyEl.firstChildElement("verification-token");
			if (!tokenEl.isNull())
			{
				token = tokenEl.text();
			}
			if (ok && !user.isEmpty() && !token.isEmpty())
			{
				emit checkAuthRequestSucceeded(user, token);
			}
			else
			{
				if (errorText.isEmpty())
					errorText = tr("Invalid server answer");
				emit checkAuthRequestFailed(user, errorText);
			}
		}

		// parsing fake user registration
		QDomElement createUserEl = doc.firstChildElement("create-user");
		if (!createUserEl.isNull())
		{
			bool ok = false;
			QString errorText;
			Jid jid;
			QString name;
			QString password;
			QDomElement successEl = createUserEl.firstChildElement("success");
			if (!successEl.isNull())
			{
				ok = (successEl.text().toLower() == "true");
			}
			QDomElement errorEl = createUserEl.firstChildElement("error");
			if (!errorEl.isNull())
			{
				errorText = errorEl.text();
			}
			QDomElement jidEl = createUserEl.firstChildElement("jid");
			if (!jidEl.isNull())
			{
				jid = jidEl.text();
			}
			QDomElement nameEl = createUserEl.firstChildElement("name");
			if (!nameEl.isNull())
			{
				name = nameEl.text();
			}
			QDomElement passwordEl = createUserEl.firstChildElement("password");
			if (!passwordEl.isNull())
			{
				password = passwordEl.text();
			}
			if (ok && !jid.node().isEmpty() && !jid.domain().isEmpty() && !password.isEmpty() && !name.isEmpty())
			{
				emit authRequestSucceeded(jid, password);
			}
			else
			{
				if (errorText.isEmpty())
					errorText = tr("Invalid server answer");
				emit authRequestFailed(errorText);
			}

			// easy registration
			QDomElement resultEl = doc.firstChildElement("result");
			if (!resultEl.isNull())
			{
				QStringList suggests;
				bool userRegistered = false;
				bool gotErrors = false;
				QString errorSummary, loginError, passwordError;
				QDomElement successEl = resultEl.firstChildElement("success");
				if (!successEl.isNull())
				{
					userRegistered = (successEl.text().toLower() == "true");
				}
				QDomElement errorsEl = resultEl.firstChildElement("errors");
				if (!errorsEl.isNull())
				{
					gotErrors = true;
					QDomElement summaryEl = errorsEl.firstChildElement("summary");
					if (!summaryEl.isNull())
					{
						errorSummary = summaryEl.text();
					}
					QDomElement loginEl = errorsEl.firstChildElement("login");
					if (!loginEl.isNull())
					{
						loginError = loginEl.text();
					}
					QDomElement passwordEl = errorsEl.firstChildElement("password");
					if (!loginEl.isNull())
					{
						passwordError = passwordEl.text();
					}
				}
				QDomElement suggestsEl = resultEl.firstChildElement("suggests");
				if (!suggestsEl.isNull())
				{
					QDomElement suggestEl = suggestsEl.firstChildElement("suggest");
					while (!suggestEl.isNull())
					{
						Jid suggestedUser;
						QDomElement loginEl = suggestEl.firstChildElement("login");
						if (!loginEl.isNull())
						{
							suggestedUser.setNode(loginEl.text());
						}
						QDomElement domainEl = suggestEl.firstChildElement("domain");
						if (!loginEl.isNull())
						{
							suggestedUser.setDomain(domainEl.text());
						}
						if (!(suggestedUser.node().isEmpty() || suggestedUser.domain().isEmpty()))
						{
							suggests.append(suggestedUser.full());
						}
						suggestEl = suggestEl.nextSiblingElement("suggest");
					}
				}

				if (userRegistered)
				{
					emit registrationSucceeded("");
				}
				else
				{
					emit registrationFailed(errorSummary, loginError, passwordError, suggests);
				}
			}
		}
	}
	else
	{
		// invalid answer
		emit requestFailed(tr("Server returned invalid XML"));
	}
}

void ServerApiHandler::onNetworkRequestFailed(const QUrl &url, const QString &errorString)
{
#ifdef DEBUG_ENABLED
	qDebug() << QString("Request to %1 failed:\n%2").arg(url.toString(), errorString);
#endif
	emit requestFailed(errorString);
}
