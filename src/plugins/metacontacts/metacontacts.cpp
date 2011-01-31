#include "metacontacts.h"

#include <QDir>
#include <QMimeData>
#include <QMessageBox>
#include <QInputDialog>
#include <QDragMoveEvent>
#include <QDragEnterEvent>
#include <QDragLeaveEvent>

#define ADR_STREAM_JID      Action::DR_StreamJid
#define ADR_META_ID         Action::DR_Parametr1
#define ADR_NAME            Action::DR_Parametr2
#define ADR_VIEW_JID        Action::DR_Parametr2
#define ADR_GROUP           Action::DR_Parametr3
#define ADR_TO_GROUP        Action::DR_Parametr3
#define ADR_RELEASE_ITEMS   Action::DR_Parametr3
#define ADR_CHILD_META_IDS  Action::DR_Parametr3
#define ADR_TAB_PAGE_ID     Action::DR_Parametr2

static const QList<int> DragGroups = QList<int>() << RIT_GROUP << RIT_GROUP_BLANK;

QDataStream &operator<<(QDataStream &AStream, const TabPageInfo &AInfo)
{
	AStream << AInfo.streamJid;
	AStream << AInfo.metaId;
	return AStream;
}

QDataStream &operator>>(QDataStream &AStream, TabPageInfo &AInfo)
{
	AStream >> AInfo.streamJid;
	AStream >> AInfo.metaId;
	AInfo.page = NULL;
	return AStream;
}

void GroupMenu::mouseReleaseEvent(QMouseEvent *AEvent)
{
	QAction *action = actionAt(AEvent->pos());
	if (action)
		action->trigger();
	else
		Menu::mouseReleaseEvent(AEvent);
}

MetaContacts::MetaContacts()
{
	FPluginManager = NULL;
	FRosterPlugin = NULL;
	FRostersViewPlugin = NULL;
	FMessageWidgets = NULL;
	FMessageProcessor = NULL;
	FStatusIcons = NULL;
}

MetaContacts::~MetaContacts()
{
	FCleanupHandler.clear();
}

void MetaContacts::pluginInfo(IPluginInfo *APluginInfo)
{
	APluginInfo->name = tr("Meta Contacts");
	APluginInfo->description = tr("Allows other modules to get information about meta contacts in roster");
	APluginInfo->version = "1.0";
	APluginInfo->author = "Potapov S.A. aka Lion";
	APluginInfo->homePage = "http://virtus.rambler.ru";
	APluginInfo->dependences.append(ROSTER_UUID);
	APluginInfo->dependences.append(STANZAPROCESSOR_UUID);
}

bool MetaContacts::initConnections(IPluginManager *APluginManager, int &AInitOrder)
{
	Q_UNUSED(AInitOrder);
	FPluginManager = APluginManager;

	IPlugin *plugin = APluginManager->pluginInterface("IRosterPlugin").value(0,NULL);
	if (plugin)
	{
		FRosterPlugin = qobject_cast<IRosterPlugin *>(plugin->instance());
		if (FRosterPlugin)
		{
			connect(FRosterPlugin->instance(),SIGNAL(rosterAdded(IRoster *)),SLOT(onRosterAdded(IRoster *)));
			connect(FRosterPlugin->instance(),SIGNAL(rosterRemoved(IRoster *)),SLOT(onRosterRemoved(IRoster *)));
		}
	}

	plugin = APluginManager->pluginInterface("IRostersViewPlugin").value(0,NULL);
	if (plugin)
	{
		FRostersViewPlugin = qobject_cast<IRostersViewPlugin *>(plugin->instance());
		if (FRostersViewPlugin)
		{
			connect(FRostersViewPlugin->rostersView()->instance(),SIGNAL(indexContextMenu(IRosterIndex *, Menu *)),SLOT(onRosterIndexContextMenu(IRosterIndex *, Menu *)));
		}
	}

	plugin = APluginManager->pluginInterface("IMessageWidgets").value(0,NULL);
	if (plugin)
	{
		FMessageWidgets = qobject_cast<IMessageWidgets *>(plugin->instance());
		if (FMessageWidgets)
		{
			connect(FMessageWidgets->instance(),SIGNAL(chatWindowCreated(IChatWindow *)),SLOT(onChatWindowCreated(IChatWindow *)));
		}
	}

	plugin = APluginManager->pluginInterface("IMessageProcessor").value(0,NULL);
	if (plugin)
		FMessageProcessor = qobject_cast<IMessageProcessor *>(plugin->instance());

	plugin = APluginManager->pluginInterface("IStatusIcons").value(0,NULL);
	if (plugin)
		FStatusIcons = qobject_cast<IStatusIcons *>(plugin->instance());

	connect(Options::instance(),SIGNAL(optionsOpened()),SLOT(onOptionsOpened()));
	connect(Options::instance(),SIGNAL(optionsClosed()),SLOT(onOptionsClosed()));

	return FRosterPlugin!=NULL;
}

bool MetaContacts::initObjects()
{
	initMetaItemDescriptors();
	if (FMessageWidgets)
	{
		FMessageWidgets->insertTabPageHandler(this);
		FMessageWidgets->insertViewDropHandler(this);
	}
	if (FRostersViewPlugin)
	{
		MetaProxyModel *proxyModel = new MetaProxyModel(this, FRostersViewPlugin->rostersView());
		FRostersViewPlugin->rostersView()->insertProxyModel(proxyModel, RPO_METACONTACTS_MODIFIER);
		FRostersViewPlugin->rostersView()->insertClickHooker(RCHO_DEFAULT,this);
		FRostersViewPlugin->rostersView()->insertDragDropHandler(this);
	}
	return true;
}

bool MetaContacts::tabPageAvail(const QString &ATabPageId) const
{
	if (FTabPages.contains(ATabPageId))
	{
		const TabPageInfo &pageInfo = FTabPages.value(ATabPageId);
		IMetaRoster *mroster = findBareMetaRoster(pageInfo.streamJid);
		return pageInfo.page!=NULL || (mroster!=NULL && mroster->isEnabled() && mroster->metaContact(pageInfo.metaId).id.isValid());
	}
	return false;
}

ITabPage *MetaContacts::tabPageFind(const QString &ATabPageId) const
{
	return FTabPages.contains(ATabPageId) ? FTabPages.value(ATabPageId).page : NULL;
}

ITabPage *MetaContacts::tabPageCreate(const QString &ATabPageId)
{
	ITabPage *page = tabPageFind(ATabPageId);
	if (page==NULL && tabPageAvail(ATabPageId))
	{
		TabPageInfo &pageInfo = FTabPages[ATabPageId];
		IMetaRoster *mroster = findBareMetaRoster(pageInfo.streamJid);
		if (mroster)
		{
			pageInfo.page = newMetaTabWindow(mroster->roster()->streamJid(),pageInfo.metaId);
			page = pageInfo.page;
		}
	}
	return page;
}

Action *MetaContacts::tabPageAction(const QString &ATabPageId, QObject *AParent)
{
	if (tabPageAvail(ATabPageId))
	{
		const TabPageInfo &pageInfo = FTabPages.value(ATabPageId);
		IMetaRoster *mroster = findBareMetaRoster(pageInfo.streamJid);
		if (mroster && mroster->isOpen())
		{
			IMetaContact contact = mroster->metaContact(pageInfo.metaId);

			Action *action = new Action(AParent);
			action->setData(ADR_TAB_PAGE_ID, ATabPageId);
			action->setText(!contact.name.isEmpty() ? contact.name : contact.id.node());
			connect(action,SIGNAL(triggered(bool)),SLOT(onOpenTabPageAction(bool)));

			ITabPage *page = tabPageFind(ATabPageId);
			if (page)
			{
				if (page->tabPageNotifier() && page->tabPageNotifier()->activeNotify()>0)
				{
					ITabPageNotify notify = page->tabPageNotifier()->notifyById(page->tabPageNotifier()->activeNotify());
					if (!notify.iconKey.isEmpty() && !notify.iconStorage.isEmpty())
						action->setIcon(notify.iconStorage, notify.iconKey);
					else
						action->setIcon(notify.icon);
				}
				else
				{
					action->setIcon(page->tabPageIcon());
				}
			}
			else
			{
				IPresenceItem pitem = mroster->metaPresence(pageInfo.metaId);
				action->setIcon(FStatusIcons!=NULL ? FStatusIcons->iconByStatus(pitem.show,SUBSCRIPTION_BOTH,false) : QIcon());
			}
			return action;
		}
	}
	return NULL;
}

bool MetaContacts::rosterIndexClicked(IRosterIndex *AIndex, int AOrder)
{
	Q_UNUSED(AOrder);
	if (AIndex->type() == RIT_METACONTACT)
	{
		IMetaRoster *mroster = findMetaRoster(AIndex->data(RDR_STREAM_JID).toString());
		if (FMessageWidgets && mroster && mroster->isEnabled())
		{
			Jid metaId = AIndex->data(RDR_INDEX_ID).toString();
			IMetaTabWindow *window = newMetaTabWindow(mroster->streamJid(), metaId);
			window->showTabPage();
		}
	}
	return false;
}

Qt::DropActions MetaContacts::rosterDragStart(const QMouseEvent *AEvent, const QModelIndex &AIndex, QDrag *ADrag)
{
	Q_UNUSED(AEvent);
	Q_UNUSED(ADrag);
	if (AIndex.data(RDR_TYPE).toInt() == RIT_METACONTACT)
	{
		IMetaRoster *mroster = findMetaRoster(AIndex.data(RDR_STREAM_JID).toString());
		if (mroster && mroster->isOpen())
			return Qt::MoveAction;
	}
	return Qt::IgnoreAction;
}

bool MetaContacts::rosterDragEnter(const QDragEnterEvent *AEvent)
{
	if (AEvent->mimeData()->hasFormat(DDT_ROSTERSVIEW_INDEX_DATA))
	{
		QMap<int, QVariant> indexData;
		QDataStream stream(AEvent->mimeData()->data(DDT_ROSTERSVIEW_INDEX_DATA));
		stream >> indexData;

		if (indexData.value(RDR_TYPE).toInt() == RIT_METACONTACT)
			return true;
	}
	return false;
}

bool MetaContacts::rosterDragMove(const QDragMoveEvent *AEvent, const QModelIndex &AHover)
{
	Q_UNUSED(AEvent);
	if (AHover.data(RDR_TYPE).toInt() == RIT_METACONTACT)
	{
		IMetaRoster *mroster = findMetaRoster(AHover.data(RDR_STREAM_JID).toString());
		if (mroster && mroster->isOpen())
			return true;
	}
	return false;
}

void MetaContacts::rosterDragLeave(const QDragLeaveEvent *AEvent)
{
	Q_UNUSED(AEvent);
}

bool MetaContacts::rosterDropAction(const QDropEvent *AEvent, const QModelIndex &AIndex, Menu *AMenu)
{
	if (AEvent->dropAction() == Qt::MoveAction)
	{
		IMetaRoster *mroster = findMetaRoster(AIndex.data(RDR_STREAM_JID).toString());
		if (mroster && mroster->isOpen())
		{
			QMap<int, QVariant> indexData;
			QDataStream stream(AEvent->mimeData()->data(DDT_ROSTERSVIEW_INDEX_DATA));
			stream >> indexData;
			Jid indexMetaId = indexData.value(RDR_INDEX_ID).toString();

			if (AIndex.data(RDR_TYPE).toInt() == RIT_METACONTACT)
			{
				Jid hoverMetaId = AIndex.data(RDR_INDEX_ID).toString();
				if (hoverMetaId != indexMetaId)
				{
					Action *mergeAction = new Action(AMenu);
					mergeAction->setText(tr("Merge contacts"));
					mergeAction->setData(ADR_STREAM_JID,mroster->streamJid().full());
					mergeAction->setData(ADR_META_ID,hoverMetaId.pBare());
					mergeAction->setData(ADR_CHILD_META_IDS,QList<QVariant>() << indexMetaId.pBare());
					connect(mergeAction,SIGNAL(triggered(bool)),SLOT(onMergeContacts(bool)));
					AMenu->addAction(mergeAction,AG_DEFAULT);
					AMenu->setDefaultAction(mergeAction);
					return true;
				}
				else
				{

				}
			}
		}
	}
	return false;
}

bool MetaContacts::viewDragEnter(IViewWidget *AWidget, const QDragEnterEvent *AEvent)
{
	if (AEvent->mimeData()->hasFormat(DDT_ROSTERSVIEW_INDEX_DATA))
	{
		QMap<int, QVariant> indexData;
		QDataStream stream(AEvent->mimeData()->data(DDT_ROSTERSVIEW_INDEX_DATA));
		stream >> indexData;

		if (AWidget->streamJid()==indexData.value(RDR_STREAM_JID).toString() && indexData.value(RDR_TYPE).toInt()==RIT_METACONTACT)
		{
			IMetaRoster *mroster = findMetaRoster(AWidget->streamJid());
			if (mroster && mroster->isOpen())
			{
				Jid metaId = mroster->itemMetaContact(AWidget->contactJid());
				return metaId.isValid() && metaId!=indexData.value(RDR_INDEX_ID).toString();
			}
		}
	}
}

bool MetaContacts::viewDragMove(IViewWidget *AWidget, const QDragMoveEvent *AEvent)
{
	Q_UNUSED(AWidget);
	Q_UNUSED(AEvent);
	return true;
}

void MetaContacts::viewDragLeave(IViewWidget *AWidget, const QDragLeaveEvent *AEvent)
{
	Q_UNUSED(AWidget);
	Q_UNUSED(AEvent);
}

bool MetaContacts::viewDropAction(IViewWidget *AWidget, const QDropEvent *AEvent, Menu *AMenu)
{
	if (AEvent->dropAction() == Qt::MoveAction)
	{
		IMetaRoster *mroster = findMetaRoster(AWidget->streamJid());
		if (mroster && mroster->isOpen())
		{
			QMap<int, QVariant> indexData;
			QDataStream stream(AEvent->mimeData()->data(DDT_ROSTERSVIEW_INDEX_DATA));
			stream >> indexData;

			Jid indexMetaId = indexData.value(RDR_INDEX_ID).toString();
			Jid viewMetaId = mroster->itemMetaContact(AWidget->contactJid());
			IMetaContact indexContact = mroster->metaContact(indexMetaId);

			Action *nameAction = new Action(AMenu);
			nameAction->setText(!indexContact.name.isEmpty() ? indexContact.name : indexContact.id.node());
			nameAction->setEnabled(false);
			AMenu->addAction(nameAction,AG_DEFAULT-100);

			Action *infoAction = new Action(AMenu);
			infoAction->setData(ADR_STREAM_JID,mroster->streamJid().full());
			infoAction->setData(ADR_META_ID,indexMetaId.pBare());
			infoAction->setData(ADR_VIEW_JID,AWidget->contactJid().full());
			infoAction->setText(tr("Send contact data"));
			AMenu->addAction(infoAction,AG_DEFAULT);

			if (indexMetaId != viewMetaId)
			{
				Action *mergeAction = new Action(AMenu);
				mergeAction->setText(tr("Merge contacts"));
				mergeAction->setData(ADR_STREAM_JID,mroster->streamJid().full());
				mergeAction->setData(ADR_META_ID,viewMetaId.pBare());
				mergeAction->setData(ADR_CHILD_META_IDS,QList<QVariant>() << indexMetaId.pBare());
				connect(mergeAction,SIGNAL(triggered(bool)),SLOT(onMergeContacts(bool)));
				AMenu->addAction(mergeAction,AG_DEFAULT);
			}

			return true;
		}
	}
	return false;
}

QString MetaContacts::itemHint(const Jid &AItemJid) const
{
	QString hint = AItemJid.node();
	if (!hint.isEmpty())
	{
		int dog = hint.lastIndexOf('%');
		if (dog>=0)
			hint[dog] = '@';
	}
	else
	{
		hint = AItemJid.domain();
	}
	return hint;
}

IMetaItemDescriptor MetaContacts::itemDescriptor(const Jid &AItemJid) const
{
	for (QList<IMetaItemDescriptor>::const_iterator it=FMetaItemDescriptors.constBegin(); it!=FMetaItemDescriptors.constEnd(); it++)
	{
		QRegExp regexp(it->pattern);
		if (regexp.indexIn(AItemJid.pBare())>=0)
			return *it;
	}
	return FDefaultItemDescriptor;
}

IMetaRoster *MetaContacts::newMetaRoster(IRoster *ARoster)
{
	IMetaRoster *mroster = findMetaRoster(ARoster->streamJid());
	if (mroster == NULL)
	{
		mroster = new MetaRoster(FPluginManager,this,ARoster);
		connect(mroster->instance(),SIGNAL(destroyed(QObject *)),SLOT(onMetaRosterDestroyed(QObject *)));
		FCleanupHandler.add(mroster->instance());
		FMetaRosters.append(mroster);
	}
	return mroster;
}

IMetaRoster *MetaContacts::findMetaRoster(const Jid &AStreamJid) const
{
	foreach(IMetaRoster *mroster, FMetaRosters)
		if (mroster->streamJid() == AStreamJid)
			return mroster;
	return NULL;
}

void MetaContacts::removeMetaRoster(IRoster *ARoster)
{
	IMetaRoster *mroster = findMetaRoster(ARoster->streamJid());
	if (mroster)
	{
		disconnect(mroster->instance(),SIGNAL(destroyed(QObject *)),this,SLOT(onMetaRosterDestroyed(QObject *)));
		FMetaRosters.removeAll(mroster);
		delete mroster->instance();
	}
}

QString MetaContacts::metaRosterFileName(const Jid &AStreamJid) const
{
	QDir dir(FPluginManager->homePath());
	if (!dir.exists("metarosters"))
		dir.mkdir("metarosters");
	dir.cd("metarosters");
	return dir.absoluteFilePath(Jid::encode(AStreamJid.pBare())+".xml");
}

QList<IMetaTabWindow *> MetaContacts::metaTabWindows() const
{
	return FMetaTabWindows;
}

IMetaTabWindow *MetaContacts::newMetaTabWindow(const Jid &AStreamJid, const Jid &AMetaId)
{
	IMetaTabWindow *window = findMetaTabWindow(AStreamJid,AMetaId);
	if (!window && FMessageWidgets)
	{
		IMetaRoster *mroster = findMetaRoster(AStreamJid);
		if (mroster && mroster->isEnabled() && mroster->metaContact(AMetaId).id.isValid())
		{
			window = new MetaTabWindow(FPluginManager,this,mroster,AMetaId);
			connect(window->instance(),SIGNAL(tabPageActivated()),SLOT(onMetaTabWindowActivated()));
			connect(window->instance(),SIGNAL(itemPageRequested(const Jid &)),SLOT(onMetaTabWindowItemPageRequested(const Jid &)));
			connect(window->instance(),SIGNAL(tabPageDestroyed()),SLOT(onMetaTabWindowDestroyed()));
			FCleanupHandler.add(window->instance());

			window->setTabPageNotifier(FMessageWidgets->newTabPageNotifier(window));

			if (FRostersViewPlugin && FRostersViewPlugin->rostersView()->rostersModel())
			{
				MetaContextMenu *menu = new MetaContextMenu(FRostersViewPlugin->rostersView()->rostersModel(),FRostersViewPlugin->rostersView(),window);
				QToolButton *button = window->toolBarChanger()->insertAction(menu->menuAction(),TBG_MCMTW_USER_TOOLS);
				button->setPopupMode(QToolButton::InstantPopup);
			}

			FMetaTabWindows.append(window);
			emit metaTabWindowCreated(window);

			TabPageInfo &pageInfo = FTabPages[window->tabPageId()];
			pageInfo.page = window;
			emit tabPageCreated(window);
		}
	}
	return window;
}

IMetaTabWindow *MetaContacts::findMetaTabWindow(const Jid &AStreamJid, const Jid &AMetaId) const
{
	foreach(IMetaTabWindow *window, FMetaTabWindows)
	{
		if (window->metaId()==AMetaId && window->metaRoster()->streamJid()==AStreamJid)
			return window;
	}
	return NULL;
}

void MetaContacts::initMetaItemDescriptors()
{
	FDefaultItemDescriptor.name = tr("Jabber");
	FDefaultItemDescriptor.icon = MNI_METACONTACTS_ITEM_JABBER;
	FDefaultItemDescriptor.combine = false;
	FDefaultItemDescriptor.detach = true;
	FDefaultItemDescriptor.service = false;
	FDefaultItemDescriptor.pageOrder = MIPO_JABBER;
	FDefaultItemDescriptor.pattern = QString::null;

	IMetaItemDescriptor sms;
	sms.name = tr("SMS");
	sms.icon = MNI_METACONTACTS_ITEM_SMS;
	sms.combine = true;
	sms.detach = true;
	sms.service = true;
	sms.pageOrder = MIPO_SMS;
	sms.pattern = "*.@sms\\.";
	FMetaItemDescriptors.append(sms);

	IMetaItemDescriptor mail;
	mail.name = tr("Mail");
	mail.icon = MNI_METACONTACTS_ITEM_MAIL;
	mail.combine = false;
	mail.detach = true;
	mail.service = true;
	mail.pageOrder = MIPO_MAIL;
	mail.pattern = "*.@mail\\.";
	FMetaItemDescriptors.append(mail);

	IMetaItemDescriptor icq;
	icq.name = tr("ICQ");
	icq.icon = MNI_METACONTACTS_ITEM_ICQ;
	icq.combine = false;
	icq.detach = true;
	icq.service = false;
	icq.pageOrder = MIPO_ICQ;
	icq.pattern = "*.@icq\\.";
	FMetaItemDescriptors.append(icq);

	IMetaItemDescriptor magent;
	magent.name = tr("Agent@Mail");
	magent.icon = MNI_METACONTACTS_ITEM_MAGENT;
	magent.combine = false;
	magent.detach = true;
	magent.service = false;
	magent.pageOrder = MIPO_MAGENT;
	magent.pattern = "*.@mrim\\.";
	FMetaItemDescriptors.append(magent);

	IMetaItemDescriptor twitter;
	twitter.name = tr("Twitter");
	twitter.icon = MNI_METACONTACTS_ITEM_TWITTER;
	twitter.combine = false;
	twitter.detach = true;
	twitter.service = false;
	twitter.pageOrder = MIPO_TWITTER;
	twitter.pattern = "*.@twitter\\.";
	FMetaItemDescriptors.append(twitter);

	IMetaItemDescriptor fring;
	fring.name = tr("Twitter");
	fring.icon = MNI_METACONTACTS_ITEM_FRING;
	fring.combine = false;
	fring.detach = true;
	fring.service = false;
	fring.pageOrder = MIPO_FRING;
	fring.pattern = "*.@fring\\.";
	FMetaItemDescriptors.append(fring);

	IMetaItemDescriptor gtalk;
	gtalk.name = tr("GTalk");
	gtalk.icon = MNI_METACONTACTS_ITEM_GTALK;
	gtalk.combine = false;
	gtalk.detach = true;
	gtalk.service = false;
	gtalk.pageOrder = MIPO_GTALK;
	gtalk.pattern = "*.@(gtalk\\.|gmail\\.com|googlemail\\.com$)";
	FMetaItemDescriptors.append(gtalk);

	IMetaItemDescriptor yonline;
	yonline.name = tr("Y.Online");
	yonline.icon = MNI_METACONTACTS_ITEM_YONLINE;
	yonline.combine = false;
	yonline.detach = true;
	yonline.service = false;
	yonline.pageOrder = MIPO_YONLINE;
	yonline.pattern = "*.@(yonline\\.|ya\\.ru$)";
	FMetaItemDescriptors.append(yonline);

	IMetaItemDescriptor qip;
	qip.name = tr("QIP");
	qip.icon = MNI_METACONTACTS_ITEM_QIP;
	qip.combine = false;
	qip.detach = true;
	qip.service = false;
	qip.pageOrder = MIPO_QIP;
	qip.pattern = "*.@(qip\\.|qip\\.ru$)";
	FMetaItemDescriptors.append(qip);

	IMetaItemDescriptor vkontakte;
	vkontakte.name = tr("VKontakte");
	vkontakte.icon = MNI_METACONTACTS_ITEM_VKONTAKTE;
	vkontakte.combine = false;
	vkontakte.detach = true;
	vkontakte.service = false;
	vkontakte.pageOrder = MIPO_VKONTAKTE;
	vkontakte.pattern = "*.@(vk\\.|vk\\.com$)";
	FMetaItemDescriptors.append(vkontakte);

	IMetaItemDescriptor odnoklasniki;
	odnoklasniki.name = tr("Odnoklasniki");
	odnoklasniki.icon = MNI_METACONTACTS_ITEM_ODNOKLASNIKI;
	odnoklasniki.combine = false;
	odnoklasniki.detach = true;
	odnoklasniki.service = false;
	odnoklasniki.pageOrder = MIPO_ODNOKLASNIKI;
	odnoklasniki.pattern = "*.@odnkl\\.";
	FMetaItemDescriptors.append(odnoklasniki);

	IMetaItemDescriptor facebook;
	facebook.name = tr("Facebook");
	facebook.icon = MNI_METACONTACTS_ITEM_FACEBOOK;
	facebook.combine = false;
	facebook.detach = true;
	facebook.service = false;
	facebook.pageOrder = MIPO_FACEBOOK;
	facebook.pattern = "*.@(facebook\\.|chat\\.facebook\\.com$)";
	FMetaItemDescriptors.append(facebook);

	IMetaItemDescriptor livejournal;
	livejournal.name = tr("LiveJournal");
	livejournal.icon = MNI_METACONTACTS_ITEM_LIVEJOURNAL;
	livejournal.combine = false;
	livejournal.detach = true;
	livejournal.service = false;
	livejournal.pageOrder = MIPO_LIVEJOURNAL;
	livejournal.pattern = "*.@(livejournal\\.|livejournal\\.com$)";
	FMetaItemDescriptors.append(livejournal);

	IMetaItemDescriptor rambler;
	rambler.name = tr("Rambler");
	rambler.icon = MNI_METACONTACTS_ITEM_RAMBLER;
	rambler.combine = false;
	rambler.detach = true;
	rambler.service = false;
	rambler.pageOrder = MIPO_RAMBLER;
	rambler.pattern = "*.@(rambler\\.ru|lenta\\.ru|myrambler\\.ru|autorambler\\.ru|ro\\.ru|r0\\.ru)$";
	FMetaItemDescriptors.append(rambler);
}

void MetaContacts::deleteMetaRosterWindows(IMetaRoster *AMetaRoster)
{
	QList<IMetaTabWindow *> windows = FMetaTabWindows;
	foreach(IMetaTabWindow *window, windows)
		if (window->metaRoster() == AMetaRoster)
			delete window->instance();
}

IMetaRoster * MetaContacts::findBareMetaRoster(const Jid &AStreamJid) const
{
	IMetaRoster *mroster = findMetaRoster(AStreamJid);
	for (int i=0; mroster==NULL && i<FMetaRosters.count(); i++)
		if (FMetaRosters.at(i)->roster()->streamJid() && AStreamJid)
			mroster = FMetaRosters.at(i);
	return mroster;
}

void MetaContacts::onMetaRosterOpened()
{
	IMetaRoster *mroster = qobject_cast<IMetaRoster *>(sender());
	if (mroster)
		emit metaRosterOpened(mroster);
}

void MetaContacts::onMetaAvatarChanged( const Jid &AMetaId )
{
	IMetaRoster *mroster = qobject_cast<IMetaRoster *>(sender());
	if (mroster)
		emit metaAvatarChanged(mroster,AMetaId);
}

void MetaContacts::onMetaPresenceChanged(const Jid &AMetaId)
{
	IMetaRoster *mroster = qobject_cast<IMetaRoster *>(sender());
	if (mroster)
		emit metaPresenceChanged(mroster,AMetaId);
}

void MetaContacts::onMetaContactReceived(const IMetaContact &AContact, const IMetaContact &ABefore)
{
	IMetaRoster *mroster = qobject_cast<IMetaRoster *>(sender());
	if (mroster)
		emit metaContactReceived(mroster,AContact,ABefore);
}

void MetaContacts::onMetaActionResult(const QString &AActionId, const QString &AErrCond, const QString &AErrMessage)
{
	IMetaRoster *mroster = qobject_cast<IMetaRoster *>(sender());
	if (mroster)
		emit metaActionResult(mroster,AActionId,AErrCond,AErrMessage);
}

void MetaContacts::onMetaRosterClosed()
{
	IMetaRoster *mroster = qobject_cast<IMetaRoster *>(sender());
	if (mroster)
		emit metaRosterClosed(mroster);
}

void MetaContacts::onMetaRosterEnabled(bool AEnabled)
{
	IMetaRoster *mroster = qobject_cast<IMetaRoster *>(sender());
	if (mroster)
		emit metaRosterEnabled(mroster,AEnabled);
}

void MetaContacts::onMetaRosterStreamJidAboutToBeChanged(const Jid &AAfter)
{
	IMetaRoster *mroster = qobject_cast<IMetaRoster *>(sender());
	if (mroster)
	{
		if (!(mroster->streamJid() && AAfter))
			mroster->saveMetaContacts(metaRosterFileName(mroster->streamJid()));
		emit metaRosterStreamJidAboutToBeChanged(mroster, AAfter);
	}
}

void MetaContacts::onMetaRosterStreamJidChanged(const Jid &ABefore)
{
	IMetaRoster *mroster = qobject_cast<IMetaRoster *>(sender());
	if (mroster)
	{
		emit metaRosterStreamJidChanged(mroster, ABefore);
		if (!(mroster->streamJid() && ABefore))
			mroster->loadMetaContacts(metaRosterFileName(mroster->streamJid()));
	}
}

void MetaContacts::onMetaRosterDestroyed(QObject *AObject)
{
	MetaRoster *mroster = static_cast<MetaRoster *>(AObject);
	FMetaRosters.removeAll(mroster);
}

void MetaContacts::onRosterAdded(IRoster *ARoster)
{
	IMetaRoster *mroster = newMetaRoster(ARoster);
	connect(mroster->instance(),SIGNAL(metaRosterOpened()),SLOT(onMetaRosterOpened()));
	connect(mroster->instance(),SIGNAL(metaAvatarChanged(const Jid &)),SLOT(onMetaAvatarChanged(const Jid &)));
	connect(mroster->instance(),SIGNAL(metaPresenceChanged(const Jid &)),SLOT(onMetaPresenceChanged(const Jid &)));
	connect(mroster->instance(),SIGNAL(metaContactReceived(const IMetaContact &, const IMetaContact &)),
		SLOT(onMetaContactReceived(const IMetaContact &, const IMetaContact &)));
	connect(mroster->instance(),SIGNAL(metaActionResult(const QString &, const QString &, const QString &)),
		SLOT(onMetaActionResult(const QString &, const QString &, const QString &)));
	connect(mroster->instance(),SIGNAL(metaRosterClosed()),SLOT(onMetaRosterClosed()));
	connect(mroster->instance(),SIGNAL(metaRosterEnabled(bool)),SLOT(onMetaRosterEnabled(bool)));
	connect(mroster->instance(),SIGNAL(metaRosterStreamJidAboutToBeChanged(const Jid &)),SLOT(onMetaRosterStreamJidAboutToBeChanged(const Jid &)));
	connect(mroster->instance(),SIGNAL(metaRosterStreamJidChanged(const Jid &)),SLOT(onMetaRosterStreamJidChanged(const Jid &)));
	emit metaRosterAdded(mroster);

	FLoadQueue.append(mroster);
	QTimer::singleShot(0,this,SLOT(onLoadMetaRosters()));
}

void MetaContacts::onRosterRemoved(IRoster *ARoster)
{
	IMetaRoster *mroster = findMetaRoster(ARoster->streamJid());
	if (mroster)
	{
		deleteMetaRosterWindows(mroster);
		mroster->saveMetaContacts(metaRosterFileName(mroster->streamJid()));
		emit metaRosterRemoved(mroster);
		removeMetaRoster(ARoster);
	}
}

void MetaContacts::onMetaTabWindowActivated()
{
	IMetaTabWindow *window = qobject_cast<IMetaTabWindow *>(sender());
	if (window)
	{
		TabPageInfo &pageInfo = FTabPages[window->tabPageId()];
		pageInfo.streamJid = window->metaRoster()->streamJid();
		pageInfo.metaId = window->metaId();
		pageInfo.page = window;
	}
}

void MetaContacts::onMetaTabWindowItemPageRequested(const Jid &AItemJid)
{
	IMetaTabWindow *window = qobject_cast<IMetaTabWindow *>(sender());
	if (window)
	{
		FMessageProcessor->createWindow(window->metaRoster()->streamJid(),AItemJid,Message::Chat,IMessageHandler::SM_ADD_TAB);
	}
}

void MetaContacts::onMetaTabWindowDestroyed()
{
	IMetaTabWindow *window = qobject_cast<IMetaTabWindow *>(sender());
	if (window)
	{
		if (FTabPages.contains(window->tabPageId()))
			FTabPages[window->tabPageId()].page = NULL;
		FMetaTabWindows.removeAll(window);
		emit metaTabWindowDestroyed(window);
		emit tabPageDestroyed(window);
	}
}

void MetaContacts::onRenameContact(bool)
{
	Action *action = qobject_cast<Action *>(sender());
	if (action)
	{
		QString streamJid = action->data(ADR_STREAM_JID).toString();
		IMetaRoster *mroster = findMetaRoster(streamJid);
		if (mroster && mroster->isOpen())
		{
			Jid metaId = action->data(ADR_META_ID).toString();
			QString oldName = action->data(ADR_NAME).toString();
			bool ok = false;
			QString newName = QInputDialog::getText(NULL,tr("Contact name"),tr("Enter name for contact"), QLineEdit::Normal, oldName, &ok);
			if (ok && !newName.isEmpty() && newName != oldName)
				mroster->renameContact(metaId, newName);
		}
	}
}

void MetaContacts::onDeleteContact(bool)
{
	Action *action = qobject_cast<Action *>(sender());
	if (action)
	{
		QString streamJid = action->data(ADR_STREAM_JID).toString();
		IMetaRoster *mroster = findMetaRoster(streamJid);
		if (mroster && mroster->isOpen())
		{
			Jid metaId = action->data(ADR_META_ID).toString();
			IMetaContact contact = mroster->metaContact(metaId);
			if (QMessageBox::question(NULL,tr("Remove contact"),
				tr("You are assured that wish to remove a contact <b>%1</b> from roster?").arg(!contact.name.isEmpty() ? contact.name : contact.id.node()),
				QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes)
			{
				mroster->deleteContact(metaId);
			}
		}
	}
}

void MetaContacts::onMergeContacts(bool)
{
	Action *action = qobject_cast<Action *>(sender());
	if (action)
	{
		QString streamJid = action->data(ADR_STREAM_JID).toString();
		IMetaRoster *mroster = findMetaRoster(streamJid);
		if (mroster && mroster->isOpen())
		{
			QList<Jid> metaIds;
			metaIds.append(action->data(ADR_META_ID).toString());
			foreach(QVariant metaId, action->data(ADR_CHILD_META_IDS).toList())
				metaIds.append(metaId.toString());

			if (metaIds.count() > 1)
			{
				MergeContactsDialog *dialog = new MergeContactsDialog(mroster,metaIds);
				connect(mroster->instance(),SIGNAL(metaRosterClosed()),dialog,SLOT(reject()));
				WidgetManager::showActivateRaiseWindow(dialog);
			}
		}
	}
}

void MetaContacts::onReleaseContactItems(bool)
{
	Action *action = qobject_cast<Action *>(sender());
	if (action)
	{
		QString streamJid = action->data(ADR_STREAM_JID).toString();
		IMetaRoster *mroster = findMetaRoster(streamJid);
		if (mroster && mroster->isOpen())
		{
			Jid metaId = action->data(ADR_META_ID).toString();
			foreach(QVariant itemJid, action->data(ADR_RELEASE_ITEMS).toList())
				mroster->releaseContactItem(metaId,itemJid.toString());
		}
	}
}

void MetaContacts::onChangeContactGroups(bool AChecked)
{
	Action *action = qobject_cast<Action *>(sender());
	if (action)
	{
		QString streamJid = action->data(ADR_STREAM_JID).toString();
		IMetaRoster *mroster = findMetaRoster(streamJid);
		if (mroster && mroster->isOpen())
		{
			IMetaContact contact = mroster->metaContact(action->data(ADR_META_ID).toString());
			if (contact.id.isValid())
			{
				bool checkBlank = false;
				bool uncheckBlank = false;
				bool uncheckGroups = false;
				QString group = action->data(ADR_TO_GROUP).toString();
				if (group == mroster->roster()->groupDelimiter())
				{
					group = QInputDialog::getText(NULL,tr("Create new group"),tr("Enter group name:"));
					if (!group.isEmpty())
					{
						uncheckBlank = contact.groups.isEmpty();
						mroster->setContactGroups(contact.id,contact.groups += group);
					}
				}
				else if (group.isEmpty())
				{
					if (!contact.groups.isEmpty())
					{
						uncheckGroups = true;
						mroster->setContactGroups(contact.id,QSet<QString>());
					}
					action->setChecked(true);
				}
				else if (AChecked)
				{
					uncheckBlank = contact.groups.isEmpty();
					mroster->setContactGroups(contact.id,contact.groups += group);
				}
				else
				{
					checkBlank = (contact.groups-=group).isEmpty();
					mroster->setContactGroups(contact.id,contact.groups -= group);
				}

				Menu *menu = qobject_cast<Menu *>(action->parent());
				if (menu && (checkBlank || uncheckBlank || uncheckGroups))
				{
					Action *blankAction = menu->groupActions(AG_DEFAULT-1).value(0);
					if (blankAction && checkBlank)
						blankAction->setChecked(true);
					else if (blankAction && uncheckBlank)
						blankAction->setChecked(false);

					foreach(Action *groupAction, menu->groupActions(AG_DEFAULT))
					{
						if (uncheckGroups)
							groupAction->setChecked(false);
					}
				}
			}
		}
	}
}

void MetaContacts::onLoadMetaRosters()
{
	foreach(IMetaRoster *mroster, FLoadQueue)
		mroster->loadMetaContacts(metaRosterFileName(mroster->streamJid()));
	FLoadQueue.clear();
}

void MetaContacts::onOpenTabPageAction(bool)
{
	Action *action = qobject_cast<Action *>(sender());
	if (action)
	{
		ITabPage *page = tabPageCreate(action->data(ADR_TAB_PAGE_ID).toString());
		if (page)
			page->showTabPage();
	}
}

void MetaContacts::onChatWindowCreated(IChatWindow *AWindow)
{
	IMetaRoster *mroster = findMetaRoster(AWindow->streamJid());
	if (mroster && mroster->isEnabled())
	{
		Jid metaId = mroster->itemMetaContact(AWindow->contactJid());
		if (metaId.isValid())
		{
			IMetaTabWindow *window = newMetaTabWindow(mroster->streamJid(), metaId);
			window->setItemPage(AWindow->contactJid().pBare(),AWindow);
		}
	}
}

void MetaContacts::onRosterIndexContextMenu(IRosterIndex *AIndex, Menu *AMenu)
{
	Jid streamJid = AIndex->data(RDR_STREAM_JID).toString();
	IMetaRoster *mroster = findMetaRoster(streamJid);
	if (mroster && mroster->isOpen())
	{
		int itemType = AIndex->data(RDR_TYPE).toInt();
		if (itemType == RIT_METACONTACT)
		{
			Jid metaId = AIndex->data(RDR_INDEX_ID).toString();
			const IMetaContact &contact = mroster->metaContact(metaId);

			QHash<int,QVariant> data;
			data.insert(ADR_STREAM_JID,streamJid.full());
			data.insert(ADR_META_ID,metaId.pBare());
			data.insert(ADR_NAME,contact.name);

			// Change group menu
			GroupMenu *groupMenu = new GroupMenu(AMenu);
			groupMenu->setTitle(tr("Groups"));

			Action *blankGroupAction = new Action(groupMenu);
			blankGroupAction->setText(FRostersViewPlugin->rostersView()->rostersModel()->blankGroupName());
			blankGroupAction->setData(data);
			blankGroupAction->setCheckable(true);
			blankGroupAction->setChecked(contact.groups.isEmpty());
			connect(blankGroupAction,SIGNAL(triggered(bool)),SLOT(onChangeContactGroups(bool)));
			groupMenu->addAction(blankGroupAction,AG_DEFAULT-1,true);

			foreach (QString group, mroster->groups())
			{
				Action *action = new Action(groupMenu);
				action->setText(group);
				action->setData(data);
				action->setData(ADR_TO_GROUP, group);
				action->setCheckable(true);
				action->setChecked(contact.groups.contains(group));
				connect(action,SIGNAL(triggered(bool)),SLOT(onChangeContactGroups(bool)));
				groupMenu->addAction(action,AG_DEFAULT,true);
			}

			Action *action = new Action(groupMenu);
			action->setText(tr("New group..."));
			action->setData(data);
			action->setData(ADR_TO_GROUP, mroster->roster()->groupDelimiter());
			connect(action,SIGNAL(triggered(bool)),SLOT(onChangeContactGroups(bool)));
			groupMenu->addAction(action,AG_DEFAULT+1,true);

			AMenu->addAction(groupMenu->menuAction(),AG_RVCM_ROSTERCHANGER_GROUP);

			// Release items menu
			QList<Jid> detachItems;
			foreach(Jid itemJid, contact.items)
			{
				if (itemDescriptor(itemJid).detach)
					detachItems.append(itemJid);
			}
			if (detachItems.count() > 1)
			{
				Menu *releaseMenu = new Menu(AMenu);
				releaseMenu->setTitle(tr("Separate contact"));
				AMenu->addAction(releaseMenu->menuAction(),AG_RVCM_METACONTACTS_RELEASE);

				QList<QVariant> allItems;
				foreach(Jid itemJid, detachItems)
				{
					IMetaItemDescriptor descriptor = itemDescriptor(itemJid);
					Action *action = new Action(releaseMenu);
					action->setText(QString("%1 (%2)").arg(descriptor.name).arg(itemHint(itemJid)));
					action->setIcon(RSR_STORAGE_MENUICONS,descriptor.icon);
					action->setData(data);
					action->setData(ADR_RELEASE_ITEMS,QList<QVariant>() << itemJid.pBare());
					connect(action,SIGNAL(triggered(bool)),SLOT(onReleaseContactItems(bool)));
					releaseMenu->addAction(action,AG_DEFAULT,true);
					allItems.append(itemJid.pBare());
				}

				if (allItems.count() > 2)
				{
					Action *action = new Action(releaseMenu);
					action->setText(tr("Separate all contacts"));
					action->setData(data);
					action->setData(ADR_RELEASE_ITEMS,allItems);
					connect(action,SIGNAL(triggered(bool)),SLOT(onReleaseContactItems(bool)));
					releaseMenu->addAction(action,AG_DEFAULT+1);
				}
			}

			action = new Action(AMenu);
			action->setText(tr("Rename..."));
			action->setIcon(RSR_STORAGE_MENUICONS,MNI_RCHANGER_RENAME);
			action->setData(data);
			connect(action,SIGNAL(triggered(bool)),SLOT(onRenameContact(bool)));
			AMenu->addAction(action,AG_RVCM_ROSTERCHANGER_RENAME);

			action = new Action(AMenu);
			action->setText(tr("Delete"));
			action->setIcon(RSR_STORAGE_MENUICONS,MNI_RCHANGER_REMOVE_CONTACT);
			action->setData(data);
			connect(action,SIGNAL(triggered(bool)),SLOT(onDeleteContact(bool)));
			AMenu->addAction(action,AG_RVCM_ROSTERCHANGER_REMOVE_CONTACT);
		}
	}
}

void MetaContacts::onOptionsOpened()
{
	QByteArray data = Options::fileValue("messages.last-meta-tab-pages").toByteArray();
	QDataStream stream(data);
	stream >> FTabPages;
}

void MetaContacts::onOptionsClosed()
{
	QByteArray data;
	QDataStream stream(&data, QIODevice::WriteOnly);
	stream << FTabPages;
	Options::setFileValue(data,"messages.last-meta-tab-pages");
}

Q_EXPORT_PLUGIN2(plg_metacontacts, MetaContacts)
