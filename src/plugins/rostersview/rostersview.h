#ifndef ROSTERSVIEW_H
#define ROSTERSVIEW_H

#include <QTimer>
#include <definitions/resources.h>
#include <definitions/menuicons.h>
#include <definitions/stylesheets.h>
#include <definitions/actiongroups.h>
#include <definitions/optionvalues.h>
#include <definitions/rosterdataholderorders.h>
#include <definitions/rostertooltiporders.h>
#include <definitions/rosterlabelorders.h>
#include <definitions/rosterindextyperole.h>
#include <definitions/rosterfootertextorders.h>
#include <definitions/rosterdragdropmimetypes.h>
#include <interfaces/irostersview.h>
#include <interfaces/irostersmodel.h>
#include <utils/options.h>
#include <utils/stylestorage.h>
#include "rosterindexdelegate.h"
#include "rostertooltip.h"

class RostersView :
			public QTreeView,
			public IRostersView,
			public IRosterDataHolder
{
	Q_OBJECT
	Q_INTERFACES(IRostersView IRosterDataHolder)
	Q_PROPERTY(QBrush groupBrush READ groupBrush WRITE setGroupBrush)
	Q_PROPERTY(QColor groupColor READ groupColor WRITE setGroupColor)
	Q_PROPERTY(int groupFontSize READ groupFontSize WRITE setGroupFontSize)
	Q_PROPERTY(QColor footerColor READ footerColor WRITE setFooterColor)
public:
	RostersView(QWidget *AParent = NULL);
	~RostersView();
	virtual QTreeView *instance() { return this; }
	//IRosterDataHolder
	virtual int rosterDataOrder() const;
	virtual QList<int> rosterDataRoles() const;
	virtual QList<int> rosterDataTypes() const;
	virtual QVariant rosterData(const IRosterIndex *AIndex, int ARole) const;
	virtual bool setRosterData(IRosterIndex *AIndex, int ARole, const QVariant &AValue);
	//IRostersView
	virtual IRostersModel *rostersModel() const;
	virtual void setRostersModel(IRostersModel *AModel);
	virtual bool repaintRosterIndex(IRosterIndex *AIndex);
	virtual void expandIndexParents(IRosterIndex *AIndex);
	virtual void expandIndexParents(const QModelIndex &AIndex);
	//--ProxyModels
	virtual void insertProxyModel(QAbstractProxyModel *AProxyModel, int AOrder);
	virtual QList<QAbstractProxyModel *> proxyModels() const;
	virtual void removeProxyModel(QAbstractProxyModel *AProxyModel);
	virtual QModelIndex mapToModel(const QModelIndex &AProxyIndex) const;
	virtual QModelIndex mapFromModel(const QModelIndex &AModelIndex) const;
	virtual QModelIndex mapToProxy(QAbstractProxyModel *AProxyModel, const QModelIndex &AModelIndex) const;
	virtual QModelIndex mapFromProxy(QAbstractProxyModel *AProxyModel, const QModelIndex &AProxyIndex) const;
	//--IndexLabel
	virtual int registerLabel(const IRostersLabel &ALabel);
	virtual void updateLabel(int ALabelId, const IRostersLabel &ALabel);
	virtual void insertLabel(int ALabelId, IRosterIndex *AIndex);
	virtual void removeLabel(int ALabelId, IRosterIndex *AIndex);
	virtual void destroyLabel(int ALabelId);
	virtual int labelAt(const QPoint &APoint, const QModelIndex &AIndex) const;
	virtual QRect labelRect(int ALabeld, const QModelIndex &AIndex) const;
	//--IndexNotify
	virtual int activeNotify(IRosterIndex *AIndex) const;
	virtual QList<int> notifyQueue(IRosterIndex *AIndex) const;
	virtual IRostersNotify notifyById(int ANotifyId) const;
	virtual int insertNotify(const IRostersNotify &ANotify, const QList<IRosterIndex *> &AIndexes);
	virtual void removeNotify(int ANotifyId);
	//--ClickHookers
	virtual void insertClickHooker(int AOrder, IRostersClickHooker *AHooker);
	virtual void removeClickHooker(int AOrder, IRostersClickHooker *AHooker);
	//--DragDrop
	virtual void insertDragDropHandler(IRostersDragDropHandler *AHandler);
	virtual void removeDragDropHandler(IRostersDragDropHandler *AHandler);
	//--FooterText
	virtual void insertFooterText(int AOrderAndId, const QVariant &AValue, IRosterIndex *AIndex);
	virtual void removeFooterText(int AOrderAndId, IRosterIndex *AIndex);
	//--ContextMenu
	virtual void contextMenuForIndex(IRosterIndex *AIndex, int ALabelId, Menu *AMenu);
	//--ClipboardMenu
	virtual void clipboardMenuForIndex(IRosterIndex *AIndex, Menu *AMenu);
	// props
	QBrush groupBrush() const;
	void setGroupBrush(const QBrush & newBrush);
	QColor groupColor() const;
	void setGroupColor(const QColor& newColor);
	int groupFontSize() const;
	void setGroupFontSize(int size);
	QColor footerColor() const;
	void setFooterColor(const QColor& newColor);
signals:
	void modelAboutToBeSet(IRostersModel *AModel);
	void modelSet(IRostersModel *AModel);
	void proxyModelAboutToBeInserted(QAbstractProxyModel *AProxyModel, int AOrder);
	void proxyModelInserted(QAbstractProxyModel *AProxyModel);
	void proxyModelAboutToBeRemoved(QAbstractProxyModel *AProxyModel);
	void proxyModelRemoved(QAbstractProxyModel *AProxyModel);
	void viewModelAboutToBeChanged(QAbstractItemModel *AModel);
	void viewModelChanged(QAbstractItemModel *AModel);
	void indexContextMenu(IRosterIndex *AIndex, Menu *AMenu);
	void indexClipboardMenu(IRosterIndex *AIndex, Menu *AMenu);
	void labelContextMenu(IRosterIndex *AIndex, int ALabelId, Menu *AMenu);
	void labelToolTips(IRosterIndex *AIndex, int ALabelId, QMultiMap<int,QString> &AToolTips, ToolBarChanger *AToolBarChanger);
	void labelClicked(IRosterIndex *AIndex, int ALabelId);
	void labelDoubleClicked(IRosterIndex *AIndex, int ALabelId, bool &AAccepted);
	void notifyInserted(int ANotifyId);
	void notifyActivated(int ANotifyId);
	void notifyTimeout(int ANotifyId);
	void notifyRemoved(int ANotifyId);
	void dragDropHandlerInserted(IRostersDragDropHandler *AHandler);
	void dragDropHandlerRemoved(IRostersDragDropHandler *AHandler);
	//IRosterDataHolder
	void rosterDataChanged(IRosterIndex *AIndex = NULL, int ARole = 0);
public:
	void updateStatusText(IRosterIndex *AIndex = NULL);
protected:
	QStyleOptionViewItemV4 indexOption(const QModelIndex &AIndex) const;
	void appendBlinkItem(int ALabelId, int ANotifyId);
	void removeBlinkItem(int ALabelId, int ANotifyId);
	QString intId2StringId(int AIntId) const;
	void removeLabels();
	void setDropIndicatorRect(const QRect &ARect);
	void setInsertIndicatorRect(const QRect &ARect);
protected:
	//QTreeView
	virtual void drawBranches(QPainter *APainter, const QRect &ARect, const QModelIndex &AIndex) const;
	//QAbstractItemView
	virtual bool viewportEvent(QEvent *AEvent);
	virtual void resizeEvent(QResizeEvent *AEvent);
	//QWidget
	virtual void paintEvent(QPaintEvent *AEvent);
	virtual void contextMenuEvent(QContextMenuEvent *AEvent);
	virtual void mouseDoubleClickEvent(QMouseEvent *AEvent);
	virtual void mousePressEvent(QMouseEvent *AEvent);
	virtual void mouseMoveEvent(QMouseEvent *AEvent);
	virtual void mouseReleaseEvent(QMouseEvent *AEvent);
	virtual void dropEvent(QDropEvent *AEvent);
	virtual void dragEnterEvent(QDragEnterEvent *AEvent);
	virtual void dragMoveEvent(QDragMoveEvent *AEvent);
	virtual void dragLeaveEvent(QDragLeaveEvent *AEvent);
protected slots:
	void onRosterIndexContextMenu(IRosterIndex *AIndex, Menu *AMenu);
	void onRosterLabelToolTips(IRosterIndex *AIndex, int ALabelId, QMultiMap<int,QString> &AToolTips, ToolBarChanger* AToolBarChanger);
	void onCopyToClipboardActionTriggered(bool);
	void onIndexInserted(IRosterIndex *AIndex);
	void onIndexEntered(const QModelIndex &AIndex);
	void onIndexDestroyed(IRosterIndex *AIndex);
	void onRemoveIndexNotifyTimeout();
	void onUpdateIndexNotifyTimeout();
	void onBlinkTimerTimeout();
	void onDragExpandTimer();
	void onViewportEntered();
	void onChangeGroupState();
	void onExpandAllGroups();
	void onCollapseAllGroups();
private:
	IRostersModel *FRostersModel;
private:
	int FPressedLabel;
	QPoint FPressedPos;
	QModelIndex FGroupIndex;
	QModelIndex FPressedIndex;
private:
	bool FBlinkVisible;
	QTimer FBlinkTimer;
	QSet<int> FBlinkLabels;
	QSet<int> FBlinkNotifies;
private:
	QMap<int, IRostersLabel> FLabelItems;
	QMultiMap<IRosterIndex *, int> FIndexLabels;
private:
	QMap<QTimer *, int> FNotifyTimer;
	QSet<IRosterIndex *> FNotifyUpdates;
	QMap<int, IRostersNotify> FNotifyItems;
	QMap<IRosterIndex *, int> FActiveNotifies;
	QMultiMap<IRosterIndex *, int> FIndexNotifies;
private:
	QMultiMap<int, IRostersClickHooker *> FClickHookers;
private:
	RosterIndexDelegate *FRosterIndexDelegate;
	QMultiMap<int, QAbstractProxyModel *> FProxyModels;
private:
	QRect FDragRect;
	bool FStartDragFailed;
	QTimer FDragExpandTimer;
	QRect FDropIndicatorRect;
	QRect FInsertIndicatorRect;
	QList<IRostersDragDropHandler *> FDragDropHandlers;
	QList<IRostersDragDropHandler *> FActiveDragHandlers;
	QBrush groupBackground;
	QColor groupForeground;
	int groupFont;
	QColor footerTextColor;
};

#endif // ROSTERSVIEW_H
