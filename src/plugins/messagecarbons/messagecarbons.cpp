#include "messagecarbons.h"

#include <definitions/namespaces.h>
#include <definitions/stanzahandlerorders.h>
#include <utils/log.h>
#include <utils/stanza.h>
#include <utils/errorhandler.h>

#define CARBONS_TIMEOUT    30000

#define SHC_FORWARDED_MESSAGE         "/message/forwarded[@xmlns='" NS_MESSAGE_FORWARD "']"

MessageCarbons::MessageCarbons()
{
	FXmppStreams = NULL;
	FDiscovery = NULL;
	FStanzaProcessor = NULL;
	FMessageProcessor = NULL;
}

MessageCarbons::~MessageCarbons()
{

}

void MessageCarbons::pluginInfo(IPluginInfo *APluginInfo)
{
	APluginInfo->name = tr("Message Carbons");
	APluginInfo->description = tr("Allows to keep all IM clients for a user engaged in a conversation");
	APluginInfo->version = "1.0";
	APluginInfo->author = "Potapov S.A. aka Lion";
	APluginInfo->homePage = "http://contacts.rambler.ru";
	APluginInfo->dependences.append(XMPPSTREAMS_UUID);
	APluginInfo->dependences.append(STANZAPROCESSOR_UUID);
	APluginInfo->dependences.append(SERVICEDISCOVERY_UUID);
}

bool MessageCarbons::initConnections(IPluginManager *APluginManager, int &AInitOrder)
{
	Q_UNUSED(AInitOrder);
	IPlugin *plugin = APluginManager->pluginInterface("IXmppStreams").value(0,NULL);
	if (plugin)
	{
		FXmppStreams = qobject_cast<IXmppStreams *>(plugin->instance());
		if (FXmppStreams)
		{
			connect(FXmppStreams->instance(),SIGNAL(opened(IXmppStream *)),SLOT(onXmppStreamOpened(IXmppStream *)));
			connect(FXmppStreams->instance(),SIGNAL(closed(IXmppStream *)),SLOT(onXmppStreamClosed(IXmppStream *)));
		}
	}

	plugin = APluginManager->pluginInterface("IStanzaProcessor").value(0,NULL);
	if (plugin)
	{
		FStanzaProcessor = qobject_cast<IStanzaProcessor *>(plugin->instance());
	}

	plugin = APluginManager->pluginInterface("IServiceDiscovery").value(0,NULL);
	if (plugin)
	{
		FDiscovery = qobject_cast<IServiceDiscovery *>(plugin->instance());
		if (FDiscovery)
		{
			connect(FDiscovery->instance(),SIGNAL(discoInfoReceived(const IDiscoInfo &)),SLOT(onDiscoInfoReceived(const IDiscoInfo &)));
		}
	}

	plugin = APluginManager->pluginInterface("IMessageProcessor").value(0,NULL);
	if (plugin)
	{
		FMessageProcessor = qobject_cast<IMessageProcessor *>(plugin->instance());
	}

	return FXmppStreams!=NULL && FStanzaProcessor!=NULL && FDiscovery!=NULL;
}

bool MessageCarbons::stanzaReadWrite(int AHandleId, const Jid &AStreamJid, Stanza &AStanza, bool &AAccept)
{
	if (isEnabled(AStreamJid) && FSHIForwards.value(AStreamJid)==AHandleId)
	{
		QDomElement fwdElem = AStanza.firstElement("forwarded",NS_MESSAGE_FORWARD);
		bool isSent = !Stanza::findElement(fwdElem,"sent",NS_MESSAGE_CARBONS).isNull();
		bool isReceived = !Stanza::findElement(fwdElem,"received",NS_MESSAGE_CARBONS).isNull();
		QDomElement msgElem = Stanza::findElement(fwdElem,"message");
		if (!msgElem.isNull() && (isSent || isReceived))
		{
			AAccept = true;
			Stanza stanza(msgElem);
			Message message(stanza);
			if (isSent)
			{
				message.stanza().addElement("sent",NS_MESSAGE_CARBONS);
				if (FMessageProcessor)
				{
					if (FMessageProcessor->processMessage(AStreamJid,message,IMessageProcessor::MessageOut))
						FMessageProcessor->displayMessage(AStreamJid,message,IMessageProcessor::MessageOut);
				}
				emit messageSent(AStreamJid,message);
			}
			else
			{
				message.stanza().addElement("received",NS_MESSAGE_CARBONS);
				if (FMessageProcessor)
				{
					if (FMessageProcessor->processMessage(AStreamJid,message,IMessageProcessor::MessageIn))
						FMessageProcessor->displayMessage(AStreamJid,message,IMessageProcessor::MessageIn);
				}
				emit messageReceived(AStreamJid,message);
			}
		}
	}
	return false;
}

void MessageCarbons::stanzaRequestResult(const Jid &AStreamJid, const Stanza &AStanza)
{
	if (AStanza.type() == "result")
	{
		if (FEnableRequests.contains(AStanza.id()))
		{
			LogDetail(QString("[MessageCarbons] Message Carbons enabled for '%1'").arg(AStreamJid.full()));
			FEnabled[AStreamJid] = true;
			emit enableChanged(AStreamJid,true);
		}
		else if(FDisableRequests.contains(AStanza.id()))
		{
			LogDetail(QString("[MessageCarbons] Message Carbons disabled for '%1'").arg(AStreamJid.full()));
			FEnabled[AStreamJid] = false;
			emit enableChanged(AStreamJid,false);
		}
	}
	else
	{
		ErrorHandler err(AStanza.element());
		LogError(QString("[MessageCarbons] Failed to change Message Carbons state for '%1': %2").arg(AStreamJid.full(),err.message()));
		emit errorReceived(AStreamJid,err.condition(),err.message());
	}
	FEnableRequests.removeAll(AStanza.id());
	FDisableRequests.removeAll(AStanza.id());
}

bool MessageCarbons::isSupported(const Jid &AStreamJid) const
{
	return FDiscovery!=NULL && FDiscovery->discoInfo(AStreamJid,AStreamJid.domain()).features.contains(NS_MESSAGE_CARBONS);
}

bool MessageCarbons::isEnabled(const Jid &AStreamJid) const
{
	return FEnabled.value(AStreamJid);
}

bool MessageCarbons::setEnabled(const Jid &AStreamJid, bool AEnable)
{
	if (FStanzaProcessor && isSupported(AStreamJid))
	{
		if (AEnable != isEnabled(AStreamJid))
		{
			Stanza request("iq");
			request.setType("set").setId(FStanzaProcessor->newId());
			request.addElement(AEnable ? "enable" : "disable",NS_MESSAGE_CARBONS);
			if (FStanzaProcessor->sendStanzaRequest(this,AStreamJid,request,CARBONS_TIMEOUT))
			{
				if (AEnable)
				{
					FEnableRequests.append(request.id());
					LogDetail(QString("[MessageCarbons] Changing Message Carbons state for '%1' to enabled").arg(AStreamJid.full()));
				}
				else
				{
					FDisableRequests.append(request.id());
					LogDetail(QString("[MessageCarbons] Changing Message Carbons state for '%1' to disabled").arg(AStreamJid.full()));
				}
				return true;
			}
			else
			{
				LogError(QString("[MessageCarbons] Failed to send request to change Message Carbons state for '%1'").arg(AStreamJid.full()));
				return false;
			}
		}
		return true;
	}
	return false;
}

void MessageCarbons::onXmppStreamOpened(IXmppStream *AXmppStream)
{
	if (FStanzaProcessor)
	{
		IStanzaHandle shandle;
		shandle.handler = this;
		shandle.order = SHO_DEFAULT;
		shandle.direction = IStanzaHandle::DirectionIn;
		shandle.streamJid = AXmppStream->streamJid();
		shandle.conditions.append(SHC_FORWARDED_MESSAGE);
		FSHIForwards.insert(shandle.streamJid,FStanzaProcessor->insertStanzaHandle(shandle));
	}
}

void MessageCarbons::onXmppStreamClosed(IXmppStream *AXmppStream)
{
	if (FStanzaProcessor)
	{
		FStanzaProcessor->removeStanzaHandle(FSHIForwards.take(AXmppStream->streamJid()));
	}
	FEnabled.remove(AXmppStream->streamJid());
}

void MessageCarbons::onDiscoInfoReceived(const IDiscoInfo &AInfo)
{
	if (AInfo.node.isEmpty() && AInfo.contactJid==AInfo.streamJid.domain() && !FEnabled.contains(AInfo.streamJid))
	{
		FEnabled.insert(AInfo.streamJid,false);
		if (AInfo.features.contains(NS_MESSAGE_CARBONS))
			setEnabled(AInfo.streamJid,true);
		else
			LogWarning(QString("[MessageCarbons] Message Carbons does not supported by '%1'").arg(AInfo.streamJid.full()));
	}
}

Q_EXPORT_PLUGIN2(plg_messagecarbons, MessageCarbons)
