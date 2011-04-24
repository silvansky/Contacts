#include "rostersview.h"

#include <QCursor>
#include <QToolTip>
#include <QPainter>
#include <QKeyEvent>
#include <QDropEvent>
#include <QHelpEvent>
#include <QClipboard>
#include <QScrollBar>
#include <QHeaderView>
#include <QResizeEvent>
#include <QApplication>
#include <QTextDocument>
#include <QDragMoveEvent>
#include <QDragEnterEvent>
#include <QDragLeaveEvent>
#include <QContextMenuEvent>
#include <utils/imagemanager.h>

#define BLINK_VISIBLE           750
#define BLINK_INVISIBLE         250

#define ADR_CLIPBOARD_DATA      Action::DR_Parametr1

static const QList<int> groupIndexes = QList<int>() << RIT_GROUP << RIT_GROUP_BLANK << RIT_GROUP_NOT_IN_ROSTER << RIT_GROUP_MY_RESOURCES << RIT_GROUP_AGENTS;

QDataStream &operator<<(QDataStream &AStream, const IRostersLabel &ALabel)
{
	AStream << ALabel.order << ALabel.flags << ALabel.label;
	return AStream;
}

QDataStream &operator>>(QDataStream &AStream, IRostersLabel &ALabel)
{
	AStream >> ALabel.order >> ALabel.flags >> ALabel.label;
	return AStream;
}

RostersView::RostersView(QWidget *AParent) : QTreeView(AParent)
{
	setAttribute(Qt::WA_MacShowFocusRect, false);
	StyleStorage::staticStorage(RSR_STORAGE_STYLESHEETS)->insertAutoStyle(this,STS_ROSTERVIEW_ROSTER);
	connect(verticalScrollBar(), SIGNAL(rangeChanged(int,int)), SLOT(onScrollBarRangeChanged(int,int)));

	FRostersModel = NULL;

	FPressedPos = QPoint();
	FPressedLabel = RLID_NULL;
	FPressedIndex = QModelIndex();

	FBlinkVisible = true;
	FBlinkTimer.setSingleShot(true);
	connect(&FBlinkTimer,SIGNAL(timeout()),SLOT(onBlinkTimerTimeout()));

	header()->hide();
	header()->setStretchLastSection(false);

	setAnimated(true);
	setIndentation(0);
	setAutoScroll(true);
	setDragEnabled(true);
	setAcceptDrops(true);
	setMouseTracking(true);
	setRootIsDecorated(false);
	setDropIndicatorShown(true);
	setAlternatingRowColors(true);
	setSelectionMode(ExtendedSelection);
	setContextMenuPolicy(Qt::DefaultContextMenu);
	setFrameShape(QFrame::NoFrame);
	setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);

	FRosterIndexDelegate = new RosterIndexDelegate(this);
	setItemDelegate(FRosterIndexDelegate);

	FDragExpandTimer.setSingleShot(true);
	FDragExpandTimer.setInterval(500);
	connect(&FDragExpandTimer,SIGNAL(timeout()),SLOT(onDragExpandTimer()));

	connect(this,SIGNAL(labelToolTips(IRosterIndex *, int, QMultiMap<int,QString> &, ToolBarChanger*)),
		SLOT(onRosterLabelToolTips(IRosterIndex *, int, QMultiMap<int,QString> &, ToolBarChanger*)));
	connect(this,SIGNAL(indexContextMenu(IRosterIndex *, QList<IRosterIndex *>, Menu *)),
		SLOT(onRosterIndexContextMenu(IRosterIndex *, QList<IRosterIndex *>, Menu *)));
	connect(this, SIGNAL(entered(const QModelIndex&)), SLOT(onIndexEntered(const QModelIndex&)));
	connect(this, SIGNAL(viewportEntered()), SLOT(onViewportEntered()));

	connect(verticalScrollBar(), SIGNAL(valueChanged(int)), SLOT(onRepaintNeeded()));
	connect(verticalScrollBar(), SIGNAL(rangeChanged(int,int)), SLOT(onRepaintNeeded()));
	connect(verticalScrollBar(), SIGNAL(sliderMoved(int)), SLOT(onRepaintNeeded()));

	qRegisterMetaTypeStreamOperators<IRostersLabel>("IRostersLabel");
	qRegisterMetaTypeStreamOperators<RostersLabelItems>("RostersLabelItems");
}

RostersView::~RostersView()
{
	removeLabels();
}


int RostersView::rosterDataOrder() const
{
	return RDHO_ROSTER_NOTIFY;
}

QList<int> RostersView::rosterDataRoles() const
{
	static QList<int> dataRoles = QList<int>()
		<< RDR_LABEL_ITEMS
		<< RDR_FOOTER_TEXT << RDR_ALLWAYS_VISIBLE << RDR_DECORATION_FLAGS << Qt::DecorationRole << Qt::BackgroundColorRole;
	return dataRoles;
}

QList<int> RostersView::rosterDataTypes() const
{
	static QList<int> dataTypes = QList<int>() << RIT_ANY_TYPE;
	return dataTypes;
}

QVariant RostersView::rosterData(const IRosterIndex *AIndex, int ARole) const
{
	QVariant data;
	if (ARole == RDR_LABEL_ITEMS)
	{
		RostersLabelItems labelItems;
		foreach(int labelId, FIndexLabels.values(const_cast<IRosterIndex *>(AIndex)))
			labelItems.insert(labelId,FLabelItems.value(labelId));
		data.setValue(labelItems);
	}
	else if (FActiveNotifies.contains(const_cast<IRosterIndex *>(AIndex)))
	{
		const IRostersNotify &notify = FNotifyItems.value(FActiveNotifies.value(const_cast<IRosterIndex *>(AIndex)));
		if (ARole == RDR_FOOTER_TEXT)
		{
			static bool block = false;
			if (!block && !notify.footer.isNull())
			{
				block = true;
				QVariantMap footer = AIndex->data(ARole).toMap();
				footer.insert(intId2StringId(FTO_ROSTERSVIEW_STATUS),notify.footer);
				data = footer;
				block = false;
			}
		}
		else if (ARole == RDR_ALLWAYS_VISIBLE)
		{
			static bool block = false;
			if (!block && (notify.flags & IRostersNotify::AllwaysVisible)>0)
			{
				block = true;
				data = AIndex->data(ARole).toInt() + 1;
				block = false;
			}
		}
		else if (ARole == RDR_DECORATION_FLAGS)
		{
			static bool block = false;
			if (!block && (notify.flags & IRostersNotify::Blink)>0)
			{
				block = true;
				data = AIndex->data(ARole).toInt() | IRostersLabel::Blink;
				block = false;
			}
		}
		else if (ARole == Qt::DecorationRole)
		{
			data = !notify.icon.isNull() ? notify.icon : data;
		}
		else if (ARole == Qt::BackgroundColorRole)
		{
			data = notify.background;
		}
	}
	return data;
}

bool RostersView::setRosterData(IRosterIndex *AIndex, int ARole, const QVariant &AValue)
{
	Q_UNUSED(AIndex);
	Q_UNUSED(ARole);
	Q_UNUSED(AValue);
	return false;
}

IRostersModel * RostersView::rostersModel() const
{
	return FRostersModel;
}

void RostersView::setRostersModel(IRostersModel *AModel)
{
	if (FRostersModel != AModel)
	{
		emit modelAboutToBeSet(AModel);

		if (selectionModel())
			selectionModel()->clear();

		if (FRostersModel)
		{
			disconnect(FRostersModel->instance(),SIGNAL(indexInserted(IRosterIndex *)),this,SLOT(onIndexInserted(IRosterIndex *)));
			disconnect(FRostersModel->instance(),SIGNAL(indexDestroyed(IRosterIndex *)),this,SLOT(onIndexDestroyed(IRosterIndex *)));
			FRostersModel->removeDefaultDataHolder(this);
			removeLabels();
		}

		FRostersModel = AModel;

		if (FRostersModel)
		{
			FRostersModel->insertDefaultDataHolder(this);
			connect(FRostersModel->instance(),SIGNAL(indexInserted(IRosterIndex *)),this,SLOT(onIndexInserted(IRosterIndex *)));
			connect(FRostersModel->instance(),SIGNAL(indexDestroyed(IRosterIndex *)), SLOT(onIndexDestroyed(IRosterIndex *)));
		}

		if (FProxyModels.isEmpty())
		{
			emit viewModelAboutToBeChanged(FRostersModel!=NULL ? FRostersModel->instance() : NULL);
			QTreeView::setModel(FRostersModel!=NULL ? FRostersModel->instance() : NULL);
			emit viewModelChanged(FRostersModel!=NULL ? FRostersModel->instance() : NULL);
		}
		else
			FProxyModels.values().first()->setSourceModel(FRostersModel!=NULL ? FRostersModel->instance() : NULL);

		if (selectionModel())
		{
			connect(selectionModel(),SIGNAL(selectionChanged(const QItemSelection &, const QItemSelection &)),
				SLOT(onSelectionChanged(const QItemSelection &, const QItemSelection &)));
		}

		emit modelSet(FRostersModel);
	}
}

QList<IRosterIndex *> RostersView::selectedRosterIndexes() const
{
	QList<IRosterIndex *> rosterIndexes;
	if (FRostersModel)
	{
		foreach(QModelIndex modelIndex, selectedIndexes())
		{
			IRosterIndex *index = FRostersModel->rosterIndexByModelIndex(mapToModel(modelIndex));
			if (index)
				rosterIndexes.append(index);
		}
	}
	return rosterIndexes;
}

bool RostersView::repaintRosterIndex(IRosterIndex *AIndex)
{
	if (FRostersModel)
	{
		QModelIndex modelIndex = mapFromModel(FRostersModel->modelIndexByRosterIndex(AIndex));
		if (modelIndex.isValid())
		{
			QRect rect = visualRect(modelIndex).adjusted(1,1,-1,-1);
			if (!rect.isEmpty())
			{
				viewport()->repaint(rect);
				return true;
			}
		}
	}
	return false;
}

void RostersView::expandIndexParents(IRosterIndex *AIndex)
{
	QModelIndex index = FRostersModel->modelIndexByRosterIndex(AIndex);
	index = mapFromModel(index);
	expandIndexParents(index);
}

void RostersView::expandIndexParents(const QModelIndex &AIndex)
{
	QModelIndex index = AIndex;
	while (index.parent().isValid())
	{
		expand(index.parent());
		index = index.parent();
	}
}

void RostersView::insertProxyModel(QAbstractProxyModel *AProxyModel, int AOrder)
{
	if (AProxyModel && !FProxyModels.values().contains(AProxyModel))
	{
		emit proxyModelAboutToBeInserted(AProxyModel,AOrder);

		bool changeViewModel = FProxyModels.upperBound(AOrder) == FProxyModels.end();

		if (changeViewModel)
			emit viewModelAboutToBeChanged(AProxyModel);

		IRosterIndex *selectedIndex = FRostersModel!=NULL ? FRostersModel->rosterIndexByModelIndex(selectionModel()!=NULL ? mapToModel(selectionModel()->currentIndex()) : QModelIndex()) : NULL;
		if (selectionModel())
			selectionModel()->clear();

		FProxyModels.insert(AOrder,AProxyModel);
		QList<QAbstractProxyModel *> proxies = FProxyModels.values();
		int index = proxies.indexOf(AProxyModel);

		QAbstractProxyModel *befour = proxies.value(index-1,NULL);
		QAbstractProxyModel *after = proxies.value(index+1,NULL);

		if (befour)
		{
			AProxyModel->setSourceModel(befour);
		}
		else
		{
			AProxyModel->setSourceModel(FRostersModel!=NULL ? FRostersModel->instance() : NULL);
		}
		if (after)
		{
			after->setSourceModel(NULL);  //костыли для QSortFilterProxyModel, аналогичные в removeProxyModel
			after->setSourceModel(AProxyModel);
		}
		else
		{
			QTreeView::setModel(AProxyModel);
		}

		if (selectedIndex)
			setCurrentIndex(mapFromModel(FRostersModel->modelIndexByRosterIndex(selectedIndex)));

		if (changeViewModel)
			emit viewModelChanged(model());

		emit proxyModelInserted(AProxyModel);
		if (model() && model()->columnCount()>0)
			header()->resizeSection(0, width());
	}
}

QList<QAbstractProxyModel *> RostersView::proxyModels() const
{
	return FProxyModels.values();
}

void RostersView::removeProxyModel(QAbstractProxyModel *AProxyModel)
{
	if (FProxyModels.values().contains(AProxyModel))
	{
		emit proxyModelAboutToBeRemoved(AProxyModel);

		QList<QAbstractProxyModel *> proxies = FProxyModels.values();
		int index = proxies.indexOf(AProxyModel);

		QAbstractProxyModel *before = proxies.value(index-1,NULL);
		QAbstractProxyModel *after = proxies.value(index+1,NULL);

		bool changeViewModel = after==NULL;
		if (changeViewModel)
		{
			if (before!=NULL)
				emit viewModelAboutToBeChanged(before);
			else
				emit viewModelAboutToBeChanged(FRostersModel!=NULL ? FRostersModel->instance() : NULL);
		}

		IRosterIndex *selectedIndex = FRostersModel!=NULL ? FRostersModel->rosterIndexByModelIndex(selectionModel()!=NULL ? mapToModel(selectionModel()->currentIndex()) : QModelIndex()) : NULL;
		if (selectionModel())
			selectionModel()->clear();

		FProxyModels.remove(FProxyModels.key(AProxyModel),AProxyModel);

		if (after == NULL && before == NULL)
		{
			QTreeView::setModel(FRostersModel!=NULL ? FRostersModel->instance() : NULL);
		}
		else if (after == NULL)
		{
			QTreeView::setModel(before);
		}
		else if (before == NULL)
		{
			after->setSourceModel(NULL);
			after->setSourceModel(FRostersModel!=NULL ? FRostersModel->instance() : NULL);
		}
		else
		{
			after->setSourceModel(NULL);
			after->setSourceModel(before);
		}

		AProxyModel->setSourceModel(NULL);

		if (selectedIndex)
			setCurrentIndex(mapFromModel(FRostersModel->modelIndexByRosterIndex(selectedIndex)));

		if (changeViewModel)
			emit viewModelChanged(model());

		emit proxyModelRemoved(AProxyModel);
		if (model() && model()->columnCount()>0)
			header()->resizeSection(0, width());
	}
}

QModelIndex RostersView::mapToModel(const QModelIndex &AProxyIndex) const
{
	QModelIndex index = AProxyIndex;
	if (!FProxyModels.isEmpty())
	{
		QMap<int, QAbstractProxyModel *>::const_iterator it = FProxyModels.constEnd();
		do
		{
			it--;
			index = it.value()->mapToSource(index);
		} while (it != FProxyModels.constBegin());
	}
	return index;
}

QModelIndex RostersView::mapFromModel(const QModelIndex &AModelIndex) const
{
	QModelIndex index = AModelIndex;
	if (!FProxyModels.isEmpty())
	{
		QMap<int, QAbstractProxyModel *>::const_iterator it = FProxyModels.constBegin();
		while (it != FProxyModels.constEnd())
		{
			index = it.value()->mapFromSource(index);
			it++;
		}
	}
	return index;
}

QModelIndex RostersView::mapToProxy(QAbstractProxyModel *AProxyModel, const QModelIndex &AModelIndex) const
{
	QModelIndex index = AModelIndex;
	if (!FProxyModels.isEmpty())
	{
		QMap<int,QAbstractProxyModel *>::const_iterator it = FProxyModels.constBegin();
		while (it!=FProxyModels.constEnd())
		{
			index = it.value()->mapFromSource(index);
			if (it.value() == AProxyModel)
				return index;
			it++;
		}
	}
	return index;
}

QModelIndex RostersView::mapFromProxy(QAbstractProxyModel *AProxyModel, const QModelIndex &AProxyIndex) const
{
	QModelIndex index = AProxyIndex;
	if (!FProxyModels.isEmpty())
	{
		bool doMap = false;
		QMap<int, QAbstractProxyModel *>::const_iterator it = FProxyModels.constEnd();
		do
		{
			it--;
			if (it.value() == AProxyModel)
				doMap = true;
			if (doMap)
				index = it.value()->mapToSource(index);
		} while (it != FProxyModels.constBegin());
	}
	return index;
}

int RostersView::registerLabel(const IRostersLabel &ALabel)
{
	int labelId = -1;
	while (labelId<=0 || FLabelItems.contains(labelId))
		labelId = qrand();

	if (ALabel.flags & IRostersLabel::Blink)
		appendBlinkItem(labelId,-1);
	FLabelItems.insert(labelId,ALabel);
	return labelId;
}

void RostersView::updateLabel(int ALabelId, const IRostersLabel &ALabel)
{
	if (FLabelItems.contains(ALabelId))
	{
		if (ALabel.flags & IRostersLabel::Blink)
			appendBlinkItem(ALabelId,-1);
		else
			removeBlinkItem(ALabelId,-1);
		FLabelItems[ALabelId] = ALabel;

		foreach(IRosterIndex *index, FIndexLabels.keys(ALabelId))
			emit rosterDataChanged(index, RDR_LABEL_ITEMS);
	}
}

void RostersView::insertLabel(int ALabelId, IRosterIndex *AIndex)
{
	if (FLabelItems.contains(ALabelId) && !FIndexLabels.contains(AIndex,ALabelId))
	{
		const IRostersLabel &label = FLabelItems.value(ALabelId);
		if (label.flags & IRostersLabel::ExpandParents)
			expandIndexParents(AIndex);
		if (label.flags & IRostersLabel::AllwaysVisible)
			AIndex->setData(RDR_ALLWAYS_VISIBLE, AIndex->data(RDR_ALLWAYS_VISIBLE).toInt()+1);
		FIndexLabels.insertMulti(AIndex, ALabelId);
		emit rosterDataChanged(AIndex,RDR_LABEL_ITEMS);
	}
}

void RostersView::removeLabel(int ALabelId, IRosterIndex *AIndex)
{
	if (FIndexLabels.contains(AIndex,ALabelId))
	{
		FIndexLabels.remove(AIndex,ALabelId);
		const IRostersLabel &label = FLabelItems.value(ALabelId);
		if (label.flags & IRostersLabel::AllwaysVisible)
			AIndex->setData(RDR_ALLWAYS_VISIBLE, AIndex->data(RDR_ALLWAYS_VISIBLE).toInt()-1);
		emit rosterDataChanged(AIndex,RDR_LABEL_ITEMS);
	}
}

void RostersView::destroyLabel(int ALabelId)
{
	if (FLabelItems.contains(ALabelId))
	{
		FLabelItems.remove(ALabelId);
		foreach (IRosterIndex *index, FIndexLabels.keys(ALabelId))
			removeLabel(ALabelId, index);
		removeBlinkItem(ALabelId,-1);
	}
}

int RostersView::labelAt(const QPoint &APoint, const QModelIndex &AIndex) const
{
	return itemDelegate(AIndex)==FRosterIndexDelegate ? FRosterIndexDelegate->labelAt(APoint,indexOption(AIndex),AIndex) : RLID_DISPLAY;
}

QRect RostersView::labelRect(int ALabeld, const QModelIndex &AIndex) const
{
	return itemDelegate(AIndex)==FRosterIndexDelegate ? FRosterIndexDelegate->labelRect(ALabeld,indexOption(AIndex),AIndex) : QRect();
}

int RostersView::activeNotify(IRosterIndex *AIndex) const
{
	return FActiveNotifies.value(AIndex,-1);
}

QList<int> RostersView::notifyQueue(IRosterIndex *AIndex) const
{
	QMultiMap<int, int> queue;
	foreach(int notifyId, FIndexNotifies.values(AIndex))
		queue.insertMulti(FNotifyItems.value(notifyId).order, notifyId);
	return queue.values();
}

IRostersNotify RostersView::notifyById(int ANotifyId) const
{
	return FNotifyItems.value(ANotifyId);
}

QList<IRosterIndex *> RostersView::notifyIndexes(int ANotifyId) const
{
	return FIndexNotifies.keys(ANotifyId);
}

int RostersView::insertNotify(const IRostersNotify &ANotify, const QList<IRosterIndex *> &AIndexes)
{
	int notifyId = -1;
	while(notifyId<=0 || FNotifyItems.contains(notifyId))
		notifyId = qrand();

	foreach(IRosterIndex *index, AIndexes)
	{
		FNotifyUpdates += index;
		FIndexNotifies.insertMulti(index, notifyId);
	}

	if (ANotify.flags & IRostersNotify::Blink)
		appendBlinkItem(-1,notifyId);

	if (ANotify.timeout > 0)
	{
		QTimer *timer = new QTimer(this);
		timer->start(ANotify.timeout);
		FNotifyTimer.insert(timer,notifyId);
		connect(timer,SIGNAL(timeout()),SLOT(onRemoveIndexNotifyTimeout()));
	}

	FNotifyItems.insert(notifyId, ANotify);
	QTimer::singleShot(0,this,SLOT(onUpdateIndexNotifyTimeout()));
	emit notifyInserted(notifyId);

	return notifyId;
}

void RostersView::removeNotify(int ANotifyId)
{
	if (FNotifyItems.contains(ANotifyId))
	{
		foreach(IRosterIndex *index, FIndexNotifies.keys(ANotifyId))
		{
			FNotifyUpdates += index;
			FIndexNotifies.remove(index,ANotifyId);
		}
		removeBlinkItem(-1,ANotifyId);

		QTimer *timer = FNotifyTimer.key(ANotifyId,NULL);
		bool timeout = timer!=NULL ? !timer->isActive() : false;
		if (timer)
		{
			timer->deleteLater();
			FNotifyTimer.remove(timer);
		}

		FNotifyItems.remove(ANotifyId);
		QTimer::singleShot(0,this,SLOT(onUpdateIndexNotifyTimeout()));

		if (timeout)
			emit notifyTimeout(ANotifyId);
		else
			emit notifyRemoved(ANotifyId);
	}
}

void RostersView::insertClickHooker(int AOrder, IRostersClickHooker *AHooker)
{
	FClickHookers.insertMulti(AOrder,AHooker);
}

void RostersView::removeClickHooker(int AOrder, IRostersClickHooker *AHooker)
{
	FClickHookers.remove(AOrder,AHooker);
}

void RostersView::insertDragDropHandler(IRostersDragDropHandler *AHandler)
{
	if (!FDragDropHandlers.contains(AHandler))
	{
		FDragDropHandlers.append(AHandler);
		emit dragDropHandlerInserted(AHandler);
	}
}

void RostersView::removeDragDropHandler(IRostersDragDropHandler *AHandler)
{
	if (FDragDropHandlers.contains(AHandler))
	{
		FDragDropHandlers.removeAt(FDragDropHandlers.indexOf(AHandler));
		emit dragDropHandlerRemoved(AHandler);
	}
}

void RostersView::insertFooterText(int AOrderAndId, const QVariant &AValue, IRosterIndex *AIndex)
{
	if (!AValue.isNull())
	{
		QString footerId = intId2StringId(AOrderAndId);
		QMap<QString,QVariant> footerMap = AIndex->data(RDR_FOOTER_TEXT).toMap();
		footerMap.insert(footerId, AValue);
		AIndex->setData(RDR_FOOTER_TEXT,footerMap);
	}
	else
		removeFooterText(AOrderAndId,AIndex);
}

void RostersView::removeFooterText(int AOrderAndId, IRosterIndex *AIndex)
{
	QString footerId = intId2StringId(AOrderAndId);
	QMap<QString,QVariant> footerMap = AIndex->data(RDR_FOOTER_TEXT).toMap();
	if (footerMap.contains(footerId))
	{
		footerMap.remove(footerId);
		if (!footerMap.isEmpty())
			AIndex->setData(RDR_FOOTER_TEXT,footerMap);
		else
			AIndex->setData(RDR_FOOTER_TEXT,QVariant());
	}
}

void RostersView::contextMenuForIndex(IRosterIndex *AIndex, QList<IRosterIndex *> ASelected, int ALabelId, Menu *AMenu)
{
	if (AIndex && AMenu)
	{
		if (ALabelId != RLID_DISPLAY)
			emit labelContextMenu(AIndex,ALabelId,AMenu);
		else
			emit indexContextMenu(AIndex,ASelected,AMenu);
	}
}

void RostersView::clipboardMenuForIndex(IRosterIndex *AIndex, Menu *AMenu)
{
	if (AIndex && AMenu)
	{
		if (!AIndex->data(RDR_JID).toString().isEmpty())
		{
			Action *action = new Action(AMenu);
			action->setText(tr("Jabber ID"));
			action->setData(ADR_CLIPBOARD_DATA, AIndex->data(RDR_JID));
			connect(action,SIGNAL(triggered(bool)),SLOT(onCopyToClipboardActionTriggered(bool)));
			AMenu->addAction(action, AG_DEFAULT, true);
		}
		if (!AIndex->data(RDR_STATUS).toString().isEmpty())
		{
			Action *action = new Action(AMenu);
			action->setText(tr("Status"));
			action->setData(ADR_CLIPBOARD_DATA, AIndex->data(RDR_STATUS));
			connect(action,SIGNAL(triggered(bool)),SLOT(onCopyToClipboardActionTriggered(bool)));
			AMenu->addAction(action, AG_DEFAULT, true);
		}
		if (!AIndex->data(RDR_NAME).toString().isEmpty())
		{
			Action *action = new Action(AMenu);
			action->setText(tr("Name"));
			action->setData(ADR_CLIPBOARD_DATA, AIndex->data(RDR_NAME));
			connect(action,SIGNAL(triggered(bool)),SLOT(onCopyToClipboardActionTriggered(bool)));
			AMenu->addAction(action, AG_DEFAULT, true);
		}
		emit indexClipboardMenu(AIndex, AMenu);
	}
}

QBrush RostersView::groupBrush() const
{
	return groupBackground;
}

void RostersView::setGroupBrush(const QBrush & newBrush)
{
	groupBackground = newBrush;
}

QColor RostersView::groupColor() const
{
	return groupForeground;
}

void RostersView::setGroupColor(const QColor& newColor)
{
	groupForeground = newColor;
}

int RostersView::groupFontSize() const
{
	return groupFont;
}

void RostersView::setGroupFontSize(int size)
{
	groupFont = size;
}

QColor RostersView::footerColor() const
{
	return footerTextColor;
}

void RostersView::setFooterColor(const QColor& newColor)
{
	footerTextColor = newColor;
}

void RostersView::updateStatusText(IRosterIndex *AIndex)
{
	const static QList<int> statusTypes = QList<int>() << RIT_STREAM_ROOT << RIT_CONTACT << RIT_AGENT << RIT_METACONTACT;

	QList<IRosterIndex *> indexes;
	if (AIndex == NULL)
	{
		QMultiMap<int,QVariant> findData;
		foreach(int type, statusTypes)
			findData.insert(RDR_TYPE,type);
		indexes = FRostersModel!=NULL ? FRostersModel->rootIndex()->findChild(findData,true) : QList<IRosterIndex *>();
	}
	else if (statusTypes.contains(AIndex->type()))
	{
		indexes.append(AIndex);
	}

	bool show = Options::node(OPV_ROSTER_SHOWSTATUSTEXT).value().toBool();
	foreach(IRosterIndex *index, indexes)
	{
		if (show)
			insertFooterText(FTO_ROSTERSVIEW_STATUS,RDR_STATUS,index);
		else
			removeFooterText(FTO_ROSTERSVIEW_STATUS,index);
	}
}

QStyleOptionViewItemV4 RostersView::indexOption(const QModelIndex &AIndex) const
{
	QStyleOptionViewItemV4 option = viewOptions();
	option.initFrom(this);
	option.rect = visualRect(AIndex);
	option.widget = this;
	option.locale = locale();
	option.locale.setNumberOptions(QLocale::OmitGroupSeparator);
	option.showDecorationSelected |= selectionBehavior() & SelectRows;

	if (wordWrap())
		option.features = QStyleOptionViewItemV2::WrapText;

	option.state |= isExpanded(AIndex) ? QStyle::State_Open : QStyle::State_None;
	if (hasFocus() && currentIndex() == AIndex)
		option.state |= QStyle::State_HasFocus;
	if (selectedIndexes().contains(AIndex))
		option.state |= QStyle::State_Selected;
	if ((AIndex.flags() & Qt::ItemIsEnabled) == 0)
		option.state &= ~QStyle::State_Enabled;
	if (indexAt(viewport()->mapFromGlobal(QCursor::pos())) == AIndex)
		option.state |= QStyle::State_MouseOver;
	option.state |= (QStyle::State)AIndex.data(RDR_STATES_FORCE_ON).toInt();
	option.state &= ~(QStyle::State)AIndex.data(RDR_STATES_FORCE_OFF).toInt();

	return option;
}

void RostersView::appendBlinkItem(int ALabelId, int ANotifyId)
{
	if (ALabelId > 0)
		FBlinkLabels += ALabelId;
	if (ANotifyId > 0)
		FBlinkNotifies += ANotifyId;
	if (!FBlinkTimer.isActive())
		FBlinkTimer.start(BLINK_VISIBLE);
}

void RostersView::removeBlinkItem(int ALabelId, int ANotifyId)
{
	FBlinkLabels -= ALabelId;
	FBlinkNotifies -= ANotifyId;
	if (FBlinkLabels.isEmpty() && FBlinkNotifies.isEmpty())
		FBlinkTimer.stop();
}

QString RostersView::intId2StringId(int AIntId) const
{
	return QString("%1").arg(AIntId,10,10,QLatin1Char('0'));
}

void RostersView::removeLabels()
{
	foreach(int labelId, FLabelItems.keys())
	{
		foreach(IRosterIndex *index, FIndexLabels.keys(labelId))
			removeLabel(labelId,index);
	}
}

void RostersView::setDropIndicatorRect(const QRect &ARect)
{
	if (FDropIndicatorRect != ARect)
	{
		FDropIndicatorRect = ARect;
		viewport()->update();
	}
}

void RostersView::setInsertIndicatorRect(const QRect &ARect)
{
	if (FInsertIndicatorRect != ARect)
	{
		FInsertIndicatorRect = ARect;
		viewport()->update();
	}
}

void RostersView::drawBranches(QPainter *APainter, const QRect &ARect, const QModelIndex &AIndex) const
{
	Q_UNUSED(APainter);
	Q_UNUSED(ARect);
	Q_UNUSED(AIndex);
}

bool RostersView::viewportEvent(QEvent *AEvent)
{
	if (AEvent->type() == QEvent::ToolTip)
	{
		return true;
		QHelpEvent *helpEvent = static_cast<QHelpEvent *>(AEvent);
		QModelIndex viewIndex = indexAt(helpEvent->pos());
		if (FRostersModel && viewIndex.isValid())
		{
			IRosterIndex *index = FRostersModel->rosterIndexByModelIndex(mapToModel(viewIndex));
			if (index)
			{
				RosterToolTip::createInstance(helpEvent->globalPos(),this);
				RosterToolTip::toolBarChanger()->clear();

				QMultiMap<int,QString> toolTipsMap;
				emit labelToolTips(index, RLID_DISPLAY, toolTipsMap, RosterToolTip::toolBarChanger());

				if (!toolTipsMap.isEmpty())
					RosterToolTip::showTip(helpEvent->globalPos(),"<span>"+QStringList(toolTipsMap.values()).join("<br>")+"</span>",this);
				else
					RosterToolTip::showTip(helpEvent->globalPos(),QString::null,this);

				return true;
			}
		}
	}
	return QTreeView::viewportEvent(AEvent);
}

void RostersView::resizeEvent(QResizeEvent *AEvent)
{
	if (model() && model()->columnCount()>0)
		header()->resizeSection(0,AEvent->size().width());
	QTreeView::resizeEvent(AEvent);
}

void RostersView::paintEvent(QPaintEvent *AEvent)
{
	QTreeView::paintEvent(AEvent);
	if (!FDropIndicatorRect.isNull())
	{
		QPainter painter(viewport());
		QImage highlight = IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getImage(MNI_ROSTERVIEW_HIGHLIGHTED_ITEM, 1);
		qreal border = 10.0; // yao border - magic! =)
		painter.translate(FDropIndicatorRect.topLeft());
		ImageManager::drawNinePartImage(highlight, FDropIndicatorRect, border, &painter);
	}
	if (!FInsertIndicatorRect.isNull())
	{
		QStyleOption option;
		option.init(this);
		option.rect = FInsertIndicatorRect.adjusted(0,0,-1,-1);
		QPainter painter(viewport());
		style()->drawPrimitive(QStyle::PE_IndicatorItemViewItemDrop, &option, &painter, this);
	}
}

void RostersView::contextMenuEvent(QContextMenuEvent *AEvent)
{
	QModelIndex modelIndex = indexAt(AEvent->pos());
	if (FRostersModel && modelIndex.isValid())
	{
		const int labelId = labelAt(AEvent->pos(),modelIndex);
		IRosterIndex *index = FRostersModel->rosterIndexByModelIndex(mapToModel(modelIndex));

		Menu *contextMenu = new Menu(this);
		contextMenu->setAttribute(Qt::WA_DeleteOnClose, true);

		QList<IRosterIndex *> selIndexes = selectedRosterIndexes();
		if (selIndexes.count() < 2)
		{
			contextMenuForIndex(index,selIndexes,labelId,contextMenu);
			if (labelId!=RLID_DISPLAY && contextMenu->isEmpty())
				contextMenuForIndex(index,selIndexes,RLID_DISPLAY,contextMenu);
		}
		else
		{
			contextMenuForIndex(index,selIndexes,RLID_DISPLAY,contextMenu);
		}

		if (!contextMenu->isEmpty())
			contextMenu->popup(AEvent->globalPos());
		else
			delete contextMenu;
	}
}

void RostersView::mouseDoubleClickEvent(QMouseEvent *AEvent)
{
	bool accepted = false;
	if (viewport()->rect().contains(AEvent->pos()))
	{
		QModelIndex viewIndex = indexAt(AEvent->pos());
		if (FRostersModel && viewIndex.isValid())
		{
			IRosterIndex *index = FRostersModel->rosterIndexByModelIndex(mapToModel(viewIndex));
			if (index != NULL)
			{
				int notifyId = FActiveNotifies.value(index,-1);
				if (notifyId>0 && FNotifyItems.value(notifyId).hookClick)
				{
					emit notifyActivated(notifyId);
				}
				else
				{
					const int labelId = labelAt(AEvent->pos(),viewIndex);
					QMultiMap<int,IRostersClickHooker *>::const_iterator it = FClickHookers.constBegin();
					while (!accepted && it!=FClickHookers.constEnd())
					{
						accepted = it.value()->rosterIndexClicked(index,it.key());
						it++;
					}
					emit labelDoubleClicked(index,labelId,accepted);
				}
			}
		}
	}
	if (!accepted)
	{
		QTreeView::mouseDoubleClickEvent(AEvent);
	}
}

void RostersView::mousePressEvent(QMouseEvent *AEvent)
{
	FStartDragFailed = false;
	FPressedPos = AEvent->pos();
	if (viewport()->rect().contains(FPressedPos))
	{
		FPressedIndex = indexAt(FPressedPos);
		if (FPressedIndex.isValid())
		{
			FPressedLabel = labelAt(AEvent->pos(),FPressedIndex);
			if (itemsExpandable() && AEvent->button()==Qt::LeftButton && FPressedLabel==RLID_INDICATORBRANCH)
				setExpanded(FPressedIndex,!isExpanded(FPressedIndex));
		}
	}
	QTreeView::mousePressEvent(AEvent);
}

void RostersView::mouseMoveEvent(QMouseEvent *AEvent)
{
	if (!FStartDragFailed && AEvent->buttons()!=Qt::NoButton && FPressedIndex.isValid() && selectedRosterIndexes().count()<2 &&
	    (AEvent->pos()-FPressedPos).manhattanLength() > QApplication::startDragDistance())
	{
		QDrag *drag = new QDrag(this);
		drag->setMimeData(new QMimeData);

		Qt::DropActions actions = Qt::IgnoreAction;
		foreach(IRostersDragDropHandler *handler, FDragDropHandlers)
			actions |= handler->rosterDragStart(AEvent,FPressedIndex,drag);

		if (actions != Qt::IgnoreAction)
		{
			QAbstractItemDelegate *itemDeletage = itemDelegate(FPressedIndex);
			if (itemDeletage)
			{
				QStyleOptionViewItemV4 option = indexOption(FPressedIndex);
				QPoint indexPos = option.rect.topLeft();
				option.state &= ~QStyle::State_Selected;
				option.state &= ~QStyle::State_MouseOver;
				option.rect = QRect(QPoint(0,0),option.rect.size());
				QPixmap pixmap(option.rect.size());
				QPainter painter(&pixmap);
				painter.fillRect(option.rect,style()->standardPalette().color(QPalette::Normal,QPalette::Base));
				itemDeletage->paint(&painter,option,FPressedIndex);
				//painter.drawRect(option.rect.adjusted(0,0,-1,-1));
				drag->setPixmap(pixmap);
				drag->setHotSpot(FPressedPos - indexPos);
			}

			QByteArray data;
			QDataStream stream(&data,QIODevice::WriteOnly);
			stream << model()->itemData(FPressedIndex);
			drag->mimeData()->setData(DDT_ROSTERSVIEW_INDEX_DATA,data);

			IRosterIndex *index = FRostersModel->rosterIndexByModelIndex(mapToModel(FPressedIndex));
			setState(DraggingState);
			if (index)
				index->setData(RDR_IS_DRAGGED, true);

			drag->exec(actions);

			if (index)
				index->setData(RDR_IS_DRAGGED, false);
			setState(NoState);
		}
		else
		{
			FStartDragFailed = true;
		}
	}
	QTreeView::mouseMoveEvent(AEvent);
}

void RostersView::mouseReleaseEvent(QMouseEvent *AEvent)
{
	bool isClick = (FPressedPos-AEvent->pos()).manhattanLength() < QApplication::startDragDistance();
	if (isClick && AEvent->button()==Qt::LeftButton && viewport()->rect().contains(AEvent->pos()))
	{
		QModelIndex viewIndex = indexAt(AEvent->pos());
		const int labelId = viewIndex.isValid() ? labelAt(AEvent->pos(),viewIndex) : RLID_NULL;
		if (FRostersModel && FPressedIndex.isValid() && FPressedIndex==viewIndex && FPressedLabel==labelId)
		{
			IRosterIndex *index = FRostersModel->rosterIndexByModelIndex(mapToModel(viewIndex));
			if (index)
			{
				emit labelClicked(index,labelId!=RLID_NULL ? labelId : RLID_DISPLAY);
			}
		}
	}

	FPressedPos = QPoint();
	FPressedLabel = RLID_NULL;
	FPressedIndex = QModelIndex();

	QTreeView::mouseReleaseEvent(AEvent);
}

void RostersView::dropEvent(QDropEvent *AEvent)
{
	Menu *dropMenu = new Menu(this);

	bool accepted = false;
	QModelIndex index = indexAt(AEvent->pos());
	foreach(IRostersDragDropHandler *handler, FActiveDragHandlers)
		if (handler->rosterDropAction(AEvent,index,dropMenu))
			accepted = true;

	QList<Action *> actionList = dropMenu->groupActions();
	if (accepted && !actionList.isEmpty())
	{
		QAction *action = !(AEvent->mouseButtons() & Qt::RightButton) && actionList.count()==1 ? actionList.value(0) : NULL;
		if (action)
			action->trigger();
		else
			action = dropMenu->exec(mapToGlobal(AEvent->pos()));

		if (action)
			AEvent->acceptProposedAction();
		else
			AEvent->ignore();
	}
	else
		AEvent->ignore();

	delete dropMenu;
	stopAutoScroll();
	setDropIndicatorRect(QRect());
	setInsertIndicatorRect(QRect());
}

void RostersView::dragEnterEvent(QDragEnterEvent *AEvent)
{
	FActiveDragHandlers.clear();
	foreach (IRostersDragDropHandler *handler, FDragDropHandlers)
		if (handler->rosterDragEnter(AEvent))
			FActiveDragHandlers.append(handler);

	if (!FActiveDragHandlers.isEmpty())
	{
		if (hasAutoScroll())
			startAutoScroll();
		AEvent->acceptProposedAction();
	}
	else
	{
		AEvent->ignore();
	}
}

void RostersView::dragMoveEvent(QDragMoveEvent *AEvent)
{
	QModelIndex index = indexAt(AEvent->pos());

	bool accepted = false;
	foreach(IRostersDragDropHandler *handler, FActiveDragHandlers)
		if (handler->rosterDragMove(AEvent,index))
			accepted = true;

	if (accepted)
		AEvent->acceptProposedAction();
	else
		AEvent->ignore();

	if (!isExpanded(index))
		FDragExpandTimer.start();
	else
		FDragExpandTimer.stop();

	if (index != FPressedIndex)
	{
		FDragRect = visualRect(index);

		QRect insertRect = FDragRect;
		if (Options::node(OPV_ROSTER_SORTBYHAND).value().toBool())
		{
			if (AEvent->pos().y() < insertRect.top()+(insertRect.height()/8))
				insertRect.setBottom(insertRect.top()-1);
			else if (AEvent->pos().y() > insertRect.bottom()-(insertRect.height()/8))
				insertRect.setTop(insertRect.bottom()+1);
			else
				insertRect = QRect();
		}
		else
		{
			insertRect = QRect();
		}
		setInsertIndicatorRect(insertRect);

		QRect dropRect = FDragRect;
		int indexType = index.data(RDR_TYPE).toInt();
		if (indexType == RIT_CONTACT || indexType == RIT_METACONTACT)
		{
			// cutting 30% from top and bottom
			int r = - int(FDragRect.height() * 0.3); // 0.3 is a magic numer!
			if (!FDragRect.adjusted(0, r, 0, r).contains(AEvent->pos())) // putting contact into parent group
			{
				if (index.parent().isValid() && index.parent() != FPressedIndex.parent())
				{
					index = index.parent();
					indexType = index.data(RDR_TYPE).toInt();
				}
			}
		}
		if (indexType == RIT_CONTACT || indexType == RIT_GROUP || indexType == RIT_GROUP_BLANK)
		{
			QModelIndex group = indexType==RIT_CONTACT ? index.parent() : index;
			dropRect = visualRect(group);

			int irow = 0;
			QModelIndex child;
			while ((child = group.child(irow, 0)).isValid())
			{
				int childType=child.data(RDR_TYPE).toInt();
				if (childType==RIT_CONTACT || childType==RIT_METACONTACT)
					dropRect = dropRect.united(visualRect(child));
				irow++;
			}
		}
		if (FDragRect!=dropRect || insertRect.isNull())
			setDropIndicatorRect(dropRect);
		else
			setDropIndicatorRect(QRect());
	}
	else
	{
		setDropIndicatorRect(QRect());
		setInsertIndicatorRect(QRect());
	}
}

void RostersView::dragLeaveEvent(QDragLeaveEvent *AEvent)
{
	foreach(IRostersDragDropHandler *handler, FActiveDragHandlers)
		handler->rosterDragLeave(AEvent);
	stopAutoScroll();
	setDropIndicatorRect(QRect());
	setInsertIndicatorRect(QRect());
}

void RostersView::onRosterIndexContextMenu(IRosterIndex *AIndex, QList<IRosterIndex *> ASelected, Menu *AMenu)
{
	if (groupIndexes.contains(AIndex->type()) && ASelected.count()<2)
	{
		QModelIndex index = mapFromModel(FRostersModel->modelIndexByRosterIndex(AIndex));
		if (index.isValid())
		{
			FGroupIndex = index;

			Action *changeAction = new Action(AMenu);
			changeAction->setText(isExpanded(index) ? tr("Collapse group") : tr("Expand group"));
			connect(changeAction,SIGNAL(triggered()),SLOT(onChangeGroupState()));
			AMenu->addAction(changeAction,AG_RVCM_ROSTERSVIEW_GROUP_STATE);
			AMenu->setDefaultAction(changeAction);

			Action *expandAction = new Action(AMenu);
			expandAction->setText(tr("Expand all groups"));
			connect(expandAction,SIGNAL(triggered()),SLOT(onExpandAllGroups()));
			AMenu->addAction(expandAction,AG_RVCM_ROSTERSVIEW_GROUPS_STATE);

			Action *collapseAction = new Action(AMenu);
			collapseAction->setText(tr("Collapse all groups"));
			connect(collapseAction,SIGNAL(triggered()),SLOT(onCollapseAllGroups()));
			AMenu->addAction(collapseAction,AG_RVCM_ROSTERSVIEW_GROUPS_STATE);
		}
	}
}

void RostersView::onRosterLabelToolTips(IRosterIndex *AIndex, int ALabelId, QMultiMap<int,QString> &AToolTips, ToolBarChanger *AToolBarChanger)
{
	Q_UNUSED(AToolBarChanger);

	if (ALabelId==RLID_DISPLAY && (AIndex->data(RDR_TYPE).toInt()==RIT_CONTACT || AIndex->data(RDR_TYPE).toInt()==RIT_METACONTACT))
	{
		QString name = AIndex->data(RDR_NAME).toString();
		if (!name.isEmpty())
			AToolTips.insert(RTTO_CONTACT_NAME, "<b>" + Qt::escape(name) + "</b>");

		QString jid = AIndex->data(RDR_JID).toString();
		if (!jid.isEmpty())
			AToolTips.insert(RTTO_CONTACT_JID, "<font color=grey>" + Qt::escape(jid) + "</font>");

		/*
		QString priority = AIndex->data(RDR_PRIORITY).toString();
		if (!priority.isEmpty())
			AToolTips.insert(RTTO_CONTACT_PRIORITY, tr("Priority: %1").arg(priority.toInt()));

		QString ask = AIndex->data(RDR_ASK).toString();
		QString subscription = AIndex->data(RDR_SUBSCRIBTION).toString();
		if (!subscription.isEmpty())
			AToolTips.insert(RTTO_CONTACT_SUBSCRIPTION, tr("Subscription: %1 %2").arg(Qt::escape(subscription)).arg(Qt::escape(ask)));
		*/

		QString status = AIndex->data(RDR_STATUS).toString();
		if (!status.isEmpty())
			AToolTips.insert(RTTO_CONTACT_STATUS, QString("%1 <div style='margin-left:10px;'>%2</div>").arg(tr("Status:")).arg(Qt::escape(status).replace("\n","<br>")));
	}
}

void RostersView::onSelectionChanged(const QItemSelection &ASelected, const QItemSelection &ADeselected)
{
	QList<IRosterIndex *> newSelection = selectedRosterIndexes();
	if (newSelection.count() > 1)
	{
		bool accepted = false;
		emit acceptMultiSelection(newSelection,accepted);
		if (!accepted)
		{
			selectionModel()->blockSignals(true);
			selectionModel()->select(ASelected,QItemSelectionModel::Deselect);
			selectionModel()->select(ADeselected,QItemSelectionModel::Select);
			selectionModel()->blockSignals(false);
		}
	}
}

void RostersView::onCopyToClipboardActionTriggered(bool)
{
	Action *action = qobject_cast<Action *>(sender());
	if (action)
	{
		QApplication::clipboard()->setText(action->data(ADR_CLIPBOARD_DATA).toString());
	}
}

void RostersView::onIndexInserted(IRosterIndex *AIndex)
{
	updateStatusText(AIndex);
}

void RostersView::onIndexEntered(const QModelIndex &AIndex)
{
	setCursor(QCursor(Qt::CursorShape(AIndex.data(RDR_MOUSE_CURSOR).toInt())));
}

void RostersView::onIndexDestroyed(IRosterIndex *AIndex)
{
	FIndexLabels.remove(AIndex);
	FIndexNotifies.remove(AIndex);
	FActiveNotifies.remove(AIndex);
	FNotifyUpdates -= AIndex;
}

void RostersView::onRemoveIndexNotifyTimeout()
{
	QTimer *timer = qobject_cast<QTimer *>(sender());
	timer->stop();
	timer->deleteLater();
	removeNotify(FNotifyTimer.value(timer));
}

void RostersView::onUpdateIndexNotifyTimeout()
{
	foreach(IRosterIndex *index, FNotifyUpdates)
	{
		int curNotify = activeNotify(index);
		QList<int> queque = notifyQueue(index);
		int newNotify = !queque.isEmpty() ? queque.last() : -1;
		if (curNotify != newNotify)
		{
			if (newNotify > 0)
				FActiveNotifies.insert(index,newNotify);
			else
				FActiveNotifies.remove(index);

			const IRostersNotify &notify = FNotifyItems.value(newNotify);
			if(notify.flags & IRostersNotify::ExpandParents)
				expandIndexParents(index);

			emit rosterDataChanged(index,RDR_FOOTER_TEXT);
			emit rosterDataChanged(index,RDR_ALLWAYS_VISIBLE);
			emit rosterDataChanged(index,RDR_DECORATION_FLAGS);
			emit rosterDataChanged(index,Qt::DecorationRole);
			emit rosterDataChanged(index,Qt::BackgroundRole);
		}
	}
	FNotifyUpdates.clear();
}

void RostersView::onBlinkTimerTimeout()
{
	FBlinkVisible = !FBlinkVisible;
	FRosterIndexDelegate->setShowBlinkLabels(FBlinkVisible);
	foreach(int labelId, FBlinkLabels)
	{
		foreach(IRosterIndex *index, FIndexLabels.keys(labelId))
			repaintRosterIndex(index);
	}
	foreach(int notifyId, FBlinkNotifies)
	{
		foreach(IRosterIndex *index, FActiveNotifies.keys(notifyId))
			repaintRosterIndex(index);
	}
	FBlinkTimer.start(FBlinkVisible ? BLINK_VISIBLE : BLINK_INVISIBLE);
}

void RostersView::onDragExpandTimer()
{
	QModelIndex index = indexAt(FDragRect.center());
	setExpanded(index,true);
}

void RostersView::onViewportEntered()
{
	setCursor(QCursor(Qt::ArrowCursor));
}

void RostersView::onChangeGroupState()
{
	if (isExpanded(FGroupIndex))
		collapse(FGroupIndex);
	else
		expand(FGroupIndex);
}

void RostersView::onExpandAllGroups()
{
	if (FRostersModel)
	{
		QMultiMap<int, QVariant> findData;
		foreach(int type, groupIndexes)
			findData.insert(RDR_TYPE,type);
		foreach(IRosterIndex *index, FRostersModel->rootIndex()->findChild(findData,true)) {
			expand(mapFromModel(FRostersModel->modelIndexByRosterIndex(index))); }
	}
}

void RostersView::onCollapseAllGroups()
{
	if (FRostersModel)
	{
		QMultiMap<int, QVariant> findData;
		foreach(int type, groupIndexes)
			findData.insert(RDR_TYPE,type);
		foreach(IRosterIndex *index, FRostersModel->rootIndex()->findChild(findData,true)) {
			collapse(mapFromModel(FRostersModel->modelIndexByRosterIndex(index))); }
	}
}

void RostersView::onScrollBarRangeChanged(int min, int max)
{
	Q_UNUSED(min)
	Q_UNUSED(max)
	verticalScrollBar()->setStyleSheet(verticalScrollBar()->styleSheet());
}

void RostersView::onRepaintNeeded()
{
	// TODO: some optimization (repaint only dirty regions, not the whole view)
	QWidget * p = parentWidget();
	while (p && !p->isWindow())
		p = p->parentWidget();
	if (p)
		p->repaint();
	viewport()->repaint();
}
