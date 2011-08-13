#include "starttls.h"

StartTLS::StartTLS(IXmppStream *AXmppStream) : QObject(AXmppStream->instance())
{
	FConnection = NULL;
	FXmppStream = AXmppStream;
}

StartTLS::~StartTLS()
{
	FXmppStream->removeXmppStanzaHandler(this,XSHO_XMPP_FEATURE);
	emit featureDestroyed();
}

bool StartTLS::xmppStanzaIn(IXmppStream *AXmppStream, Stanza &AStanza, int AOrder)
{
	if (AXmppStream==FXmppStream && AOrder==XSHO_XMPP_FEATURE)
	{
		FXmppStream->removeXmppStanzaHandler(this,XSHO_XMPP_FEATURE);
		if (AStanza.tagName() == "proceed")
		{
			LogDetaile(QString("[StartTLS][%1] Starting connection encryption").arg(FXmppStream->streamJid().bare()));
			connect(FConnection->instance(),SIGNAL(encrypted()),SLOT(onConnectionEncrypted()));
			FConnection->startClientEncryption();
		}
		else if (AStanza.tagName() == "failure")
		{
			LogError(QString("[StartTLS][%1] StartTLS negotiation failed").arg(FXmppStream->streamJid().bare()));
			emit error(tr("StartTLS negotiation failed"));
		}
		else
		{
			LogError(QString("[StartTLS][%1] Wrong StartTLS negotiation response").arg(FXmppStream->streamJid().bare()));
			emit error(tr("Wrong StartTLS negotiation response"));
		}
		return true;
	}
	return false;
}

bool StartTLS::xmppStanzaOut(IXmppStream *AXmppStream, Stanza &AStanza, int AOrder)
{
	Q_UNUSED(AXmppStream);
	Q_UNUSED(AStanza);
	Q_UNUSED(AOrder);
	return false;
}

bool StartTLS::start(const QDomElement &AElem)
{
	FConnection = qobject_cast<IDefaultConnection *>(FXmppStream->connection()->instance());
	if (FConnection && AElem.tagName()=="starttls")
	{
		LogDetaile(QString("[StartTLS][%1] Negotiating StartTLS encryption").arg(FXmppStream->streamJid().bare()));
		Stanza request("starttls");
		request.setAttribute("xmlns",NS_FEATURE_STARTTLS);
		FXmppStream->insertXmppStanzaHandler(this,XSHO_XMPP_FEATURE);
		FXmppStream->sendStanza(request);
		return true;
	}
	deleteLater();
	return false;
}

void StartTLS::onConnectionEncrypted()
{
	deleteLater();
	emit finished(true);
}
