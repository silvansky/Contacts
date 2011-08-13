#include "saslsession.h"
#include <utils/log.h>

SASLSession::SASLSession(IXmppStream *AXmppStream) : QObject(AXmppStream->instance())
{
	FXmppStream = AXmppStream;
}

SASLSession::~SASLSession()
{
	FXmppStream->removeXmppStanzaHandler(this, XSHO_XMPP_FEATURE);
	emit featureDestroyed();
}

bool SASLSession::xmppStanzaIn(IXmppStream *AXmppStream, Stanza &AStanza, int AOrder)
{
	if (AXmppStream==FXmppStream && AOrder==XSHO_XMPP_FEATURE && AStanza.id()=="session")
	{
		if (AStanza.type() == "result")
		{
			LogDetaile(QString("[SASLSession][%1] XMPP session established successfully").arg(FXmppStream->streamJid().bare()));
			deleteLater();
			emit finished(false);
		}
		else
		{
			ErrorHandler err(AStanza.element());
			LogError(QString("[SASLSession][%1] Failed to establish XMPP session: %1").arg(FXmppStream->streamJid().bare()).arg(err.message()));
			emit error(err.message());
		}
		return true;
	}
	return false;
}

bool SASLSession::xmppStanzaOut(IXmppStream *AXmppStream, Stanza &AStanza, int AOrder)
{
	Q_UNUSED(AXmppStream);
	Q_UNUSED(AStanza);
	Q_UNUSED(AOrder);
	return false;
}

bool SASLSession::start(const QDomElement &AElem)
{
	if (AElem.tagName() == "session")
	{
		LogDetaile(QString("[SASLSession][%1] Establishing XMPP session").arg(FXmppStream->streamJid().bare()));
		Stanza session("iq");
		session.setType("set").setId("session");
		session.addElement("session",NS_FEATURE_SESSION);
		FXmppStream->insertXmppStanzaHandler(this, XSHO_XMPP_FEATURE);
		FXmppStream->sendStanza(session);
		return true;
	}
	deleteLater();
	return false;
}
