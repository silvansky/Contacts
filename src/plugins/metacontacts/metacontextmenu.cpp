#include "metacontextmenu.h"

MetaContextMenu::MetaContextMenu(IRostersModel *AModel, IRostersView *AView, IMetaTabWindow *AWindow) : Menu(AWindow->instance())
{
	FRosterIndex = NULL;
	FRostersModel = AModel;
	FRostersView = AView;
	FMetaTabWindow = AWindow;

	connect(this,SIGNAL(aboutToShow()),SLOT(onAboutToShow()));
	connect(this,SIGNAL(aboutToHide()),SLOT(onAboutToHide()));
	connect(FRostersModel->instance(),SIGNAL(indexInserted(IRosterIndex *)),SLOT(onRosterIndexInserted(IRosterIndex *)));
	connect(FRostersModel->instance(),SIGNAL(indexDataChanged(IRosterIndex *,int)),SLOT(onRosterIndexDataChanged(IRosterIndex *,int)));
	connect(FRostersModel->instance(),SIGNAL(indexRemoved(IRosterIndex *)),SLOT(onRosterIndexRemoved(IRosterIndex *)));

	onRosterIndexRemoved(FRosterIndex);
}

MetaContextMenu::~MetaContextMenu()
{

}

bool MetaContextMenu::isAcceptedIndex(IRosterIndex *AIndex)
{
	if (AIndex && FMetaTabWindow->metaRoster()->roster()->streamJid()==AIndex->data(RDR_STREAM_JID).toString())
	{
		QString metaId = AIndex->data(RDR_META_ID).toString();
		if (FMetaTabWindow->metaId() == metaId)
			return true;
	}
	return false;
}

void MetaContextMenu::updateMenu()
{
	if (FRosterIndex)
	{
		QString name = FRosterIndex->data(Qt::DisplayRole).toString();
		QImage avatar = FRosterIndex->data(RDR_AVATAR_IMAGE_LARGE).value<QImage>();
		setIcon(QIcon(QPixmap::fromImage(avatar)));
		setTitle(name);
		menuAction()->setVisible(true);
	}
	else
	{
		menuAction()->setVisible(false);
	}
}

void MetaContextMenu::onAboutToShow()
{
	if (FRosterIndex)
	{
		FRostersView->contextMenuForIndex(FRosterIndex,QList<IRosterIndex *>()<<FRosterIndex,RLID_DISPLAY,this);
	}
}

void MetaContextMenu::onAboutToHide()
{
	clear();
}

void MetaContextMenu::onRosterIndexInserted(IRosterIndex *AIndex)
{
	if (!FRosterIndex && isAcceptedIndex(AIndex))
	{
		FRosterIndex = AIndex;
		updateMenu();
	}
}

void MetaContextMenu::onRosterIndexDataChanged(IRosterIndex *AIndex, int ARole)
{
	if (AIndex == FRosterIndex)
	{
		if (ARole == RDR_META_ID)
		{
			if (isAcceptedIndex(AIndex))
				updateMenu();
			else
				onRosterIndexRemoved(FRosterIndex);
		}
		else if (ARole == RDR_NAME)
		{
			updateMenu();
		}
		else if ((ARole == RDR_AVATAR_IMAGE) || (ARole == RDR_AVATAR_IMAGE_LARGE))
		{
			updateMenu();
		}
	}
	else if (!FRosterIndex && ARole==RDR_META_ID && isAcceptedIndex(AIndex))
	{
		FRosterIndex = AIndex;
		updateMenu();
	}
}

void MetaContextMenu::onRosterIndexRemoved(IRosterIndex *AIndex)
{
	if (FRosterIndex == AIndex)
	{
		QMultiMap<int, QVariant> findData;
		findData.insert(RDR_TYPE,RIT_METACONTACT);
		findData.insert(RDR_META_ID,FMetaTabWindow->metaId());

		IRosterIndex *searchRoot = FRostersModel->streamRoot(FMetaTabWindow->metaRoster()->roster()->streamJid());
		FRosterIndex = searchRoot!=NULL ? searchRoot->findChilds(findData,true).value(0) : NULL;
		updateMenu();
	}
}
