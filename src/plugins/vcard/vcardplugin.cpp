#include "vcardplugin.h"

#include <QDir>
#include <QFile>
#include <QDomDocument>
#include <definitions/resources.h>
#include <definitions/customborder.h>
#include <definitions/stylesheets.h>
#include <utils/customborderstorage.h>
#include <utils/stylestorage.h>

#define DIR_VCARDS		  "vcards"
#define VCARD_TIMEOUT     60000
#define AVATARS_TIMEOUT   120000
#define ADR_STREAM_JID    Action::DR_StreamJid
#define ADR_CONTACT_JID   Action::DR_Parametr1

VCardPlugin::VCardPlugin()
{
	FPluginManager = NULL;
	FXmppStreams = NULL;
	FRostersView = NULL;
	FRostersViewPlugin = NULL;
	FStanzaProcessor = NULL;
	FDiscovery = NULL;
	FXmppUriQueries = NULL;
	FBitsOfBinary = NULL;
	FStatusIcons = NULL;
	FAvatars = NULL;
	FRosterPlugin = NULL;
	FPresencePlugin = NULL;
	FRosterChanger = NULL;
}

VCardPlugin::~VCardPlugin()
{

}

void VCardPlugin::pluginInfo(IPluginInfo *APluginInfo)
{
	APluginInfo->name = tr("vCard Manager");
	APluginInfo->description = tr("Allows to obtain personal contact information");
	APluginInfo->author = "Potapov S.A. aka Lion";
	APluginInfo->version = "1.0";
	APluginInfo->homePage = "http://contacts.rambler.ru";
	APluginInfo->dependences.append(BITSOFBINARY_UUID);
}

bool VCardPlugin::initConnections(IPluginManager *APluginManager, int &/*AInitOrder*/)
{
	FPluginManager = APluginManager;

	IPlugin *plugin = APluginManager->pluginInterface("IStanzaProcessor").value(0,NULL);
	if (plugin)
		FStanzaProcessor = qobject_cast<IStanzaProcessor *>(plugin->instance());

	plugin = APluginManager->pluginInterface("IXmppStreams").value(0,NULL);
	if (plugin)
	{
		FXmppStreams = qobject_cast<IXmppStreams *>(plugin->instance());
		if (FXmppStreams)
		{
			connect(FXmppStreams->instance(),SIGNAL(closed(IXmppStream *)),SLOT(onXmppStreamClosed(IXmppStream *)));
		}
	}

	plugin = APluginManager->pluginInterface("IRostersViewPlugin").value(0,NULL);
	if (plugin)
		FRostersViewPlugin = qobject_cast<IRostersViewPlugin *>(plugin->instance());

	plugin = APluginManager->pluginInterface("IServiceDiscovery").value(0,NULL);
	if (plugin)
	{
		FDiscovery = qobject_cast<IServiceDiscovery *>(plugin->instance());
	}

	plugin = APluginManager->pluginInterface("IXmppUriQueries").value(0,NULL);
	if (plugin)
	{
		FXmppUriQueries = qobject_cast<IXmppUriQueries *>(plugin->instance());
	}

	plugin = APluginManager->pluginInterface("IBitsOfBinary").value(0,NULL);
	if (plugin)
	{
		FBitsOfBinary = qobject_cast<IBitsOfBinary*>(plugin->instance());
		if (FBitsOfBinary)
		{
			connect(FBitsOfBinary->instance(), SIGNAL(binaryCached(const QString &, const QString &, const QByteArray &, quint64)), SLOT(onBinaryCached(const QString &, const QString &, const QByteArray &, quint64)));
		}
	}

	plugin = APluginManager->pluginInterface("IStatusIcons").value(0,NULL);
	if (plugin)
	{
		FStatusIcons = qobject_cast<IStatusIcons*>(plugin->instance());
	}

	plugin = APluginManager->pluginInterface("IStatusChanger").value(0,NULL);
	if (plugin)
	{
		FStatusChanger = qobject_cast<IStatusChanger*>(plugin->instance());
	}

	plugin = APluginManager->pluginInterface("IAvatars").value(0,NULL);
	if (plugin)
	{
		FAvatars = qobject_cast<IAvatars*>(plugin->instance());
	}

	plugin = APluginManager->pluginInterface("IRosterPlugin").value(0,NULL);
	if (plugin)
	{
		FRosterPlugin = qobject_cast<IRosterPlugin*>(plugin->instance());
	}

	plugin = APluginManager->pluginInterface("IPresencePlugin").value(0,NULL);
	if (plugin)
	{
		FPresencePlugin = qobject_cast<IPresencePlugin*>(plugin->instance());
	}

	plugin = APluginManager->pluginInterface("IRosterChanger").value(0,NULL);
	if (plugin)
	{
		FRosterChanger = qobject_cast<IRosterChanger *>(plugin->instance());
	}

	return true;
}

bool VCardPlugin::initObjects()
{
	if (FRostersViewPlugin)
	{
		FRostersView = FRostersViewPlugin->rostersView();
		//connect(FRostersView->instance(),SIGNAL(indexContextMenu(IRosterIndex *, QList<IRosterIndex *>, Menu *)),SLOT(onRosterIndexContextMenu(IRosterIndex *, QList<IRosterIndex *>, Menu *)));
	}
	if (FDiscovery)
	{
		registerDiscoFeatures();
	}
	if (FXmppUriQueries)
	{
		FXmppUriQueries->insertUriHandler(this, XUHO_DEFAULT);
	}
	return true;
}

void VCardPlugin::stanzaRequestResult(const Jid &AStreamJid, const Stanza &AStanza)
{
	Q_UNUSED(AStreamJid);
	if (FVCardRequestId.contains(AStanza.id()))
	{
		Jid fromJid = FVCardRequestId.take(AStanza.id());
		QDomElement elem = AStanza.firstElement(VCARD_TAGNAME,NS_VCARD_TEMP);
		if (AStanza.type()=="result")
		{
			LogDetail(QString("[VCardPlugin] Received vCard of '%1', id='%2'").arg(fromJid.full(),AStanza.id()));
			saveVCardFile(elem,fromJid);
			emit vcardReceived(fromJid);
		}
		else if (AStanza.type()=="error")
		{
			ErrorHandler err(AStanza.element());
			LogError(QString("[VCardPlugin] Failed to request vCard of '%1', id='%2': %3").arg(fromJid.full(),AStanza.id(),err.message()));
			emit vcardError(fromJid,err.message());
		}
	}
	else if (FVCardPublishId.contains(AStanza.id()))
	{
		Jid fromJid = FVCardPublishId.take(AStanza.id());
		Stanza stanza = FVCardPublishStanza.take(AStanza.id());
		if (AStanza.type() == "result")
		{
			LogDetail(QString("[VCardPlugin] Published vCard of '%1', id='%2'").arg(fromJid.full(),AStanza.id()));
			saveVCardFile(stanza.element().firstChildElement(VCARD_TAGNAME),fromJid);
			emit vcardPublished(fromJid);
		}
		else if (AStanza.type() == "error")
		{
			ErrorHandler err(AStanza.element());
			LogError(QString("[VCardPlugin] Failed to publish vCard of '%1', id='%2': %3").arg(fromJid.full(),AStanza.id(),err.message()));
			emit vcardError(fromJid, err.message());
		}
	}
	else if (FAvatarsRequestId.contains(AStanza.id()))
	{
		Jid fromJid = FAvatarsRequestId.take(AStanza.id());
		if (AStanza.type() == "result")
		{
			QDomElement elem = AStanza.firstElement("query", NS_RAMBLER_AVATAR);
			for (QDomElement child = elem.firstChildElement("avatar"); !child.isNull(); child = child.nextSiblingElement("avatar"))
			{
				QDomAttr src = child.attributeNode("src");
				if (!src.isNull())
				{
					QString cidString = src.nodeValue();
					int left = cidString.indexOf(':') + 1;
					int right = cidString.indexOf('@') - 1;
					cidString = cidString.mid(left, right - left);
					if (FBitsOfBinary->hasBinary(cidString))
					{
						FAvatarsBinaryCids.insert(cidString, AStreamJid);
						FBitsOfBinary->loadBinary(cidString, AStreamJid, "avatar.rambler.ru");
					}
				}
			}
			saveVCardFile(elem,fromJid);
			LogDetail(QString("[VCardPlugin] Received avatrs of '%1', id='%2'").arg(fromJid.full(),AStanza.id()));
			emit avatarsRecieved(fromJid);
		}
		else if (AStanza.type()=="error")
		{
			ErrorHandler err(AStanza.element());
			LogError(QString("[VCardPlugin] Failed to request avatars of '%1', id='%2': %3").arg(fromJid.full(),AStanza.id(),err.message()));
			emit avatarsError(fromJid, err.message());
		}
	}
}

void VCardPlugin::stanzaRequestTimeout(const Jid &AStreamJid, const QString &AStanzaId)
{
	Q_UNUSED(AStreamJid);
	if (FVCardRequestId.contains(AStanzaId))
	{
		Jid fromJid = FVCardRequestId.take(AStanzaId);
		ErrorHandler err(ErrorHandler::REQUEST_TIMEOUT);
		LogError(QString("[VCardPlugin] Failed to request vCard of '%1', id='%2': %3").arg(fromJid.full(),AStanzaId,err.message()));
		emit vcardError(fromJid,err.message());
	}
	else if (FVCardPublishId.contains(AStanzaId))
	{
		Jid fromJid = FVCardPublishId.take(AStanzaId);
		ErrorHandler err(ErrorHandler::REQUEST_TIMEOUT);
		LogError(QString("[VCardPlugin] Failed to publish vCard of '%1', id='%2': %3").arg(fromJid.full(),AStanzaId,err.message()));
		emit vcardError(fromJid,err.message());
	}
	else if (FAvatarsRequestId.contains(AStanzaId))
	{
		Jid fromJid = FAvatarsRequestId.take(AStanzaId);
		ErrorHandler err(ErrorHandler::REQUEST_TIMEOUT);
		LogError(QString("[VCardPlugin] Failed to request avatars of '%1', id='%2': %3").arg(fromJid.full(),AStanzaId,err.message()));
		emit avatarsError(fromJid,err.message());
	}
}

bool VCardPlugin::xmppUriOpen(const Jid &AStreamJid, const Jid &AContactJid, const QString &AAction, const QMultiMap<QString, QString> &AParams)
{
	Q_UNUSED(AParams);
	if (AAction == "vcard")
	{
		showSimpleVCardDialog(AStreamJid, AContactJid);
		return true;
	}
	return false;
}

QString VCardPlugin::vcardFileName(const Jid &AContactJid) const
{
	QDir dir(FPluginManager->homePath());
	if (!dir.exists(DIR_VCARDS))
		dir.mkdir(DIR_VCARDS);
	dir.cd(DIR_VCARDS);
	return dir.absoluteFilePath(Jid::encode(AContactJid.pFull())+".xml");
}

bool VCardPlugin::hasVCard(const Jid &AContactJid) const
{
	QString fileName = vcardFileName(AContactJid);
	return QFile::exists(fileName);
}

IVCard *VCardPlugin::vcard(const Jid &AContactJid)
{
	VCardItem &vcardItem = FVCards[AContactJid];
	if (vcardItem.vcard == NULL)
		vcardItem.vcard = new VCard(AContactJid,this);
	vcardItem.locks++;
	return vcardItem.vcard;
}

bool VCardPlugin::requestVCard(const Jid &AStreamJid, const Jid &AContactJid)
{
	if (FStanzaProcessor)
	{
		if (FVCardRequestId.key(AContactJid).isEmpty())
		{
			Stanza request("iq");
			request.setTo(AContactJid.eFull()).setType("get").setId(FStanzaProcessor->newId());
			request.addElement(VCARD_TAGNAME,NS_VCARD_TEMP);
			if (FStanzaProcessor->sendStanzaRequest(this,AStreamJid,request,VCARD_TIMEOUT))
			{
				LogDetail(QString("[VCardPlugin] Load vCard of '%1' request sent, id='%2'").arg(AContactJid.full(),request.id()));
				FVCardRequestId.insert(request.id(),AContactJid);
				return true;
			}
			else
			{
				LogError(QString("[VCardPlugin] Failed to send load vCard of '%1' request").arg(AContactJid.full()));
			}
		}
		else
		{
			return true;
		}
	}
	return false;
}

bool VCardPlugin::publishVCard(IVCard *AVCard, const Jid &AStreamJid)
{
	if (FStanzaProcessor && AVCard->isValid())
	{
		if (FVCardPublishId.key(AStreamJid.pBare()).isEmpty())
		{
			Stanza publish("iq");
			publish.setTo(AStreamJid.eBare()).setType("set").setId(FStanzaProcessor->newId());
			QDomElement elem = publish.element().appendChild(AVCard->vcardElem().cloneNode(true)).toElement();
			removeEmptyChildElements(elem);
			if (FStanzaProcessor->sendStanzaRequest(this,AStreamJid,publish,VCARD_TIMEOUT))
			{
				LogDetail(QString("[VCardPlugin] Publish vCard of '%1' request sent, id='%2'").arg(AStreamJid.bare(),publish.id()));
				FVCardPublishId.insert(publish.id(),AStreamJid.pBare());
				FVCardPublishStanza.insert(publish.id(),publish);
				return true;
			}
			else
			{
				LogError(QString("[VCardPlugin] Failed to send publish vCard of '%1' request").arg(AStreamJid.bare()));
			}
		}
		else
		{
			return true;
		}
	}
	return false;
}

bool VCardPlugin::requestAvatars(const Jid &AStreamJid, const Jid &AContactJid)
{
	if (FStanzaProcessor)
	{
		if (FAvatarsRequestId.key(AContactJid).isEmpty())
		{
			// if my vCard is requested
			if (AStreamJid && AContactJid)
			{
				// requesting default avatars from the rambler server
				Stanza request("iq");
				request.setTo(AContactJid.eFull()).setType("get").setId(FStanzaProcessor->newId());
				request.addElement("query", NS_RAMBLER_AVATAR);
				if (FStanzaProcessor->sendStanzaRequest(this, AStreamJid, request, AVATARS_TIMEOUT))
				{
					LogDetail(QString("[VCardPlugin] Load avatars of '%1' request sent, id='%2'").arg(AStreamJid.bare(),request.id()));
					FAvatarsRequestId.insert(request.id(),AContactJid);
					return true;
				}
				else
				{
					LogError(QString("[VCardPlugin] Failed to send load avatars of '%1' request").arg(AStreamJid.bare()));
				}
			}
		}
		else
		{
			return true;
		}
	}
	return false;
}

void VCardPlugin::showSimpleVCardDialog(const Jid &AStreamJid, const Jid &AContactJid)
{
	IRoster *roster = FRosterPlugin!=NULL ? FRosterPlugin->findRoster(AStreamJid) : NULL;
	if (roster && roster->isOpen())
	{
		if (FSimpleVCardDialogs.contains(AContactJid))
		{
			SimpleVCardDialog *dialog = FSimpleVCardDialogs.value(AContactJid);
			WidgetManager::showActivateRaiseWindow(dialog->window());
		}
		else if (AStreamJid.isValid() && AContactJid.isValid())
		{
			SimpleVCardDialog *dialog = new SimpleVCardDialog(this,FAvatars, FStatusIcons, FStatusChanger, FRosterPlugin, FPresencePlugin, FRosterChanger, AStreamJid, AContactJid);
			connect(dialog,SIGNAL(destroyed(QObject *)),SLOT(onSimpleVCardDialogDestroyed(QObject *)));
			FSimpleVCardDialogs.insert(AContactJid, dialog);
			WidgetManager::showActivateRaiseWindow(dialog->window());
		}
	}
}

void VCardPlugin::unlockVCard(const Jid &AContactJid)
{
	VCardItem &vcardItem = FVCards[AContactJid];
	vcardItem.locks--;
	if (vcardItem.locks == 0)
	{
		VCard *vcardCopy = vcardItem.vcard;   //После remove vcardItem будет недействителен
		FVCards.remove(AContactJid);
		delete vcardCopy;
	}
}

void VCardPlugin::saveVCardFile(const QDomElement &AElem, const Jid &AContactJid) const
{
	if (!AElem.isNull() && AContactJid.isValid())
	{
		QDomDocument doc;
		QDomElement elem = doc.appendChild(doc.createElement(VCARD_FILE_ROOT_TAGNAME)).toElement();
		elem.setAttribute("jid",AContactJid.full());
		elem.setAttribute("dateTime",QDateTime::currentDateTime().toString(Qt::ISODate));
		elem.appendChild(AElem.cloneNode(true));
		QFile vcardFile(vcardFileName(AContactJid));
		if (vcardFile.open(QIODevice::WriteOnly | QIODevice::Truncate))
		{
			vcardFile.write(doc.toByteArray());
			vcardFile.close();
		}
	}
}

void VCardPlugin::removeEmptyChildElements(QDomElement &AElem) const
{
	static const QStringList tagList = QStringList() << "HOME" << "WORK" << "INTERNET" << "X400" << "CELL" << "MODEM";

	QDomElement curChild = AElem.firstChildElement();
	while (!curChild.isNull())
	{
		removeEmptyChildElements(curChild);
		QDomElement nextChild = curChild.nextSiblingElement();
		if (curChild.text().isEmpty() && !tagList.contains(curChild.tagName()))
			curChild.parentNode().removeChild(curChild);
		curChild = nextChild;
	}
}

void VCardPlugin::registerDiscoFeatures()
{
	IDiscoFeature dfeature;

	dfeature.active = false;
	dfeature.var = NS_VCARD_TEMP;
	dfeature.name = tr("vCard");
	dfeature.description = tr("Supports the requesting of the personal contact information");
	FDiscovery->insertDiscoFeature(dfeature);
}

void VCardPlugin::onRosterIndexContextMenu(IRosterIndex *AIndex, QList<IRosterIndex *> ASelected, Menu *AMenu)
{
	if (AIndex->type()==RIT_STREAM_ROOT || AIndex->type()==RIT_CONTACT || AIndex->type()==RIT_AGENT)
	{
		IRoster *roster = FRosterPlugin!=NULL ? FRosterPlugin->findRoster(AIndex->data(RDR_STREAM_JID).toString()) : NULL;
		if (roster && roster->isOpen() && ASelected.count()<2)
		{
			Action *action = new Action(AMenu);
			action->setText(tr("Contact info"));
			action->setData(ADR_STREAM_JID,AIndex->data(RDR_STREAM_JID));
			action->setData(ADR_CONTACT_JID,Jid(AIndex->data(RDR_FULL_JID).toString()).bare());
			AMenu->addAction(action,AG_RVCM_VCARD_SHOWINFO,true);
			connect(action,SIGNAL(triggered(bool)),SLOT(onShowVCardDialogByAction(bool)));
		}
	}
}

void VCardPlugin::onShowVCardDialogByAction(bool)
{
	Action *action = qobject_cast<Action *>(sender());
	if (action)
	{
		Jid streamJid = action->data(ADR_STREAM_JID).toString();
		Jid contactJid = action->data(ADR_CONTACT_JID).toString();
		showSimpleVCardDialog(streamJid,contactJid);
	}
}

void VCardPlugin::onSimpleVCardDialogDestroyed(QObject *ADialog)
{
	SimpleVCardDialog *dialog = static_cast<SimpleVCardDialog *>(ADialog);
	FSimpleVCardDialogs.remove(FSimpleVCardDialogs.key(dialog));
}

void VCardPlugin::onXmppStreamClosed(IXmppStream *AXmppStream)
{
	foreach(SimpleVCardDialog *dialog, FSimpleVCardDialogs.values())
		if (dialog->streamJid() == AXmppStream->streamJid())
			delete dialog;
}

void VCardPlugin::onBinaryCached(const QString &AContentId, const QString &AType, const QByteArray &AData, quint64 AMaxAge)
{
	Q_UNUSED(AContentId);
	Q_UNUSED(AType);
	Q_UNUSED(AData);
	Q_UNUSED(AMaxAge);
	//if (FAvatarsBinaryCids.contains(AContentId))
	//{
	//	Jid streamJid = FAvatarsBinaryCids.take(AContentId);
	//	QImage img = QImage::fromData(AData, AType.toLatin1().data());
	//}
}

Q_EXPORT_PLUGIN2(plg_vcard, VCardPlugin)
