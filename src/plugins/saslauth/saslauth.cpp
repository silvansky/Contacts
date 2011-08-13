#include "saslauth.h"

#include <stdlib.h>

#include <QMap>
#include <QStringList>
#include <QCryptographicHash>

#define AUTH_PLAIN        "PLAIN"
#define AUTH_ANONYMOUS    "ANONYMOUS"
#define AUTH_DIGEST_MD5   "DIGEST-MD5"

static QMap<QByteArray, QByteArray> parseChallenge(const QByteArray &AChallenge)
{
	QMap<QByteArray, QByteArray> map;
	QList<QByteArray> paramsList = AChallenge.split(',');
	for (int i = 0; i<paramsList.count(); i++)
	{
		QByteArray param = paramsList.at(i).trimmed();
		int delimIndex = param.indexOf('=');
		QByteArray key = param.left(delimIndex);
		QByteArray value = param.right(param.length()-delimIndex-1);
		if (value.startsWith('"'))
		{
			value.remove(0,1).chop(1);
			value.replace("\\\"", "\"");
			value.replace("\\\\", "\\"); 
		}
		map.insert(key,value);
	}
	return map;
}

static QByteArray serializeResponse(const QMap<QByteArray, QByteArray> &AResponse)
{
	QByteArray response;
	foreach (const QByteArray &key, AResponse.keys())
	{
		QByteArray value = AResponse[key];
		value.replace("\\", "\\\\");
		value.replace("\"", "\\\"");
		response.append(key + "=\"" + value + "\",");
	}
	response.chop(1);
	return response;
}

static QByteArray getResponseValue(const QMap<QByteArray, QByteArray> &AResponse, const QString &APassword)
{
	QByteArray secret = QCryptographicHash::hash(AResponse.value("username")+':'+AResponse.value("realm")+':'+APassword.toUtf8(),QCryptographicHash::Md5);
	QByteArray a1Hex = QCryptographicHash::hash(secret+':'+AResponse.value("nonce")+':'+AResponse.value("cnonce"),QCryptographicHash::Md5).toHex();
	QByteArray a2Hex = QCryptographicHash::hash("AUTHENTICATE:"+AResponse.value("digest-uri"),QCryptographicHash::Md5).toHex();
	QByteArray value = QCryptographicHash::hash(a1Hex+':'+AResponse.value("nonce")+':'+AResponse.value("nc")+':'+AResponse.value("cnonce")+':'+AResponse.value("qop")+':'+a2Hex,QCryptographicHash::Md5);
	return value.toHex();
}

SASLAuth::SASLAuth(IXmppStream *AXmppStream) : QObject(AXmppStream->instance())
{
	FChallengeStep = 0;
	FXmppStream = AXmppStream;
}

SASLAuth::~SASLAuth()
{
	FXmppStream->removeXmppStanzaHandler(this, XSHO_XMPP_FEATURE);
	emit featureDestroyed();
}

bool SASLAuth::xmppStanzaIn(IXmppStream *AXmppStream, Stanza &AStanza, int AOrder)
{
	if (AXmppStream==FXmppStream && AOrder==XSHO_XMPP_FEATURE)
	{
		if (AStanza.tagName() == "challenge")
		{
			LogDetaile(QString("[SASLAuth][%1] DIGEST-MD5 authorization challenge received").arg(FXmppStream->streamJid().bare()));
			if (FChallengeStep == 0)
			{
				FChallengeStep++;
				QMap<QByteArray, QByteArray> challengeMap = parseChallenge(QByteArray::fromBase64(AStanza.element().text().toAscii()));

				QMap<QByteArray, QByteArray> responseMap;
				QByteArray randBytes(32,' ');
				for (int i=0; i<31; i++)
					randBytes[i] = (char) (256.0 * qrand() / (RAND_MAX + 1.0));
				responseMap["cnonce"] = randBytes.toBase64();
				if (challengeMap.contains("realm"))
					responseMap["realm"] = challengeMap.value("realm");
				else
					responseMap["realm"] = FXmppStream->streamJid().pDomain().toUtf8();
				responseMap["username"] = FXmppStream->streamJid().prepared().eNode().toUtf8();
				responseMap["nonce"] = challengeMap.value("nonce");
				responseMap["nc"] = "00000001";
				responseMap["qop"] = challengeMap.value("qop");
				responseMap["digest-uri"] = QString("xmpp/%1").arg(FXmppStream->streamJid().pDomain()).toUtf8();
				responseMap["charset"] = "utf-8";
				responseMap["response"] = getResponseValue(responseMap,FXmppStream->password());

				Stanza response("response");
				response.setAttribute("xmlns",NS_FEATURE_SASL);
				response.element().appendChild(response.createTextNode(serializeResponse(responseMap).toBase64()));
				FXmppStream->sendStanza(response);
			}
			else if (FChallengeStep == 1)
			{
				FChallengeStep--;
				Stanza response("response");
				response.setAttribute("xmlns",NS_FEATURE_SASL);
				FXmppStream->sendStanza(response);
			}
		}
		else
		{
			FXmppStream->removeXmppStanzaHandler(this, XSHO_XMPP_FEATURE);
			if (AStanza.tagName() == "success")
			{
				LogDetaile(QString("[SASLAuth][%1] Authorization finished successfully").arg(FXmppStream->streamJid().bare()));
				deleteLater();
				emit finished(true);
			}
			else if (AStanza.tagName() == "failure")
			{
				ErrorHandler err(AStanza.element(),NS_FEATURE_SASL);
				LogError(QString("[SASLAuth][%1] Authorization failure: %2").arg(FXmppStream->streamJid().bare()).arg(err.message()));
				emit error(err.message());
			}
			else if (AStanza.tagName() == "abort")
			{
				ErrorHandler err("aborted",NS_FEATURE_SASL);
				LogError(QString("[SASLAuth][%1] Authorization aborted: %2").arg(FXmppStream->streamJid().bare()).arg(err.message()));
				emit error(err.message());
			}
			else
			{
				LogError(QString("[SASLAuth][%1] Wrong SASL authentication response").arg(FXmppStream->streamJid().bare()));
				emit error(tr("Wrong SASL authentication response"));
			}
		}
		return true;
	}
	return false;
}

bool SASLAuth::xmppStanzaOut(IXmppStream *AXmppStream, Stanza &AStanza, int AOrder)
{
	Q_UNUSED(AXmppStream);
	Q_UNUSED(AStanza);
	Q_UNUSED(AOrder);
	return false;
}

bool SASLAuth::start(const QDomElement &AElem)
{
	if (AElem.tagName() == "mechanisms")
	{
		FChallengeStep = 0;

		QList<QString> mechList;
		QDomElement mechElem = AElem.firstChildElement("mechanism");
		while (!mechElem.isNull())
		{
			mechList.append(mechElem.text());
			mechElem = mechElem.nextSiblingElement("mechanism");
		}

		if (mechList.contains(AUTH_DIGEST_MD5))
		{
			LogDetaile(QString("[SASLAuth][%1] Authorization with %2 mechanism").arg(FXmppStream->streamJid().bare()).arg(AUTH_DIGEST_MD5));

			Stanza auth("auth");
			auth.setAttribute("xmlns",NS_FEATURE_SASL).setAttribute("mechanism",AUTH_DIGEST_MD5);
			FXmppStream->insertXmppStanzaHandler(this, XSHO_XMPP_FEATURE);
			FXmppStream->sendStanza(auth);
			return true;
		}
		else if (mechList.contains(AUTH_PLAIN))
		{
			LogDetaile(QString("[SASLAuth][%1] Authorization with %2 mechanism").arg(FXmppStream->streamJid().bare()).arg(AUTH_DIGEST_MD5));

			QByteArray resp;
			resp.append('\0').append(FXmppStream->streamJid().prepared().eNode().toUtf8()).append('\0').append(FXmppStream->password().toUtf8());

			Stanza auth("auth");
			auth.setAttribute("xmlns",NS_FEATURE_SASL).setAttribute("mechanism",AUTH_PLAIN);
			auth.element().appendChild(auth.createTextNode(resp.toBase64()));
			FXmppStream->insertXmppStanzaHandler(this, XSHO_XMPP_FEATURE);
			FXmppStream->sendStanza(auth);
			return true;
		}
		else if (mechList.contains(AUTH_ANONYMOUS))
		{
			LogDetaile(QString("[SASLAuth][%1] Authorization with %2 mechanism").arg(FXmppStream->streamJid().bare()).arg(AUTH_DIGEST_MD5));

			Stanza auth("auth");
			auth.setAttribute("xmlns",NS_FEATURE_SASL).setAttribute("mechanism",AUTH_ANONYMOUS);
			FXmppStream->insertXmppStanzaHandler(this, XSHO_XMPP_FEATURE);
			FXmppStream->sendStanza(auth);
			return true;
		}
		else 
		{
			LogError(QString("[SASLAuth][%1] Unsupported authorization mechanisms").arg(FXmppStream->streamJid().bare()));
		}
	}
	deleteLater();
	return false;
}
