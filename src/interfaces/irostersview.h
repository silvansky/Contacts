#ifndef IROSTERSVIEW_H
#define IROSTERSVIEW_H

#include <QTreeView>
#include <QAbstractProxyModel>
#include <interfaces/irostersmodel.h>
#include <utils/menu.h>
#include <utils/toolbarchanger.h>

#define ROSTERSVIEW_UUID "{81ebf318-5ecd-4e4b-8f4a-cac65f7a911c}"

//! интерфейс обработчика кликов на элементе ростера
class IRostersClickHooker
{
public:
	/**
	* @brief обработка события клика на элементе ростера
	* @param AIndex индекс кликнутого объекта
	* @param AOrder порядок
	* @return bool
	*/
	virtual bool rosterIndexClicked(IRosterIndex *AIndex, int AOrder) =0;
};

//! интерфейс обработчика драг-н-дропа для ростера. реализуется классами @ref FileTransfer и @ref RosterChanger.
class IRostersDragDropHandler
{
public:
	/**
	* @brief возвращает тип дропа
	* @param AEvent событие
	* @param AIndex индекс в модели
	* @param ADrag собственно драг
	* @return Qt::DropActions
	*/
	virtual Qt::DropActions rosterDragStart(const QMouseEvent *AEvent, const QModelIndex &AIndex, QDrag *ADrag) =0;
	/**
	* @brief проверка, будет ли плагин обрабатывать этот дроп
	* @param AEvent событие
	* @return bool
	*/
	virtual bool rosterDragEnter(const QDragEnterEvent *AEvent) =0;
	/**
	* @brief проверка, будет ли плагин принимать этот дроп для данного индекса в модели
	* @param AEvent событие
	* @param AHover индекс в модели, над которым проходит дроп
	* @return bool
	*/
	virtual bool rosterDragMove(const QDragMoveEvent *AEvent, const QModelIndex &AHover) =0;
	/**
	* @brief обработка события выхода драга из виджета
	* @param AEvent событие
	*/
	virtual void rosterDragLeave(const QDragLeaveEvent *AEvent) =0;
	/**
	* @brief обработка собственно дропа на индекс модели
	* @param AEvent событие
	* @param AIndex индекс в модели, на который дропают
	* @param AMenu меню (?)
	* @return bool
	*/
	virtual bool rosterDropAction(const QDropEvent *AEvent, const QModelIndex &AIndex, Menu *AMenu) =0;
};

//! интерфейс для вью ростера. для реализации классом @ref RostersView
class IRostersView
{
public:
	enum LabelFlags {
		LabelBlink                    =0x01,
		LabelVisible                  =0x02,
		LabelExpandParents            =0x04
	};
public:
	//--RostersModel
	/**
	* @brief возвращает объект (класс @ref RostersView реализует интерфейс @ref IRostersView, а так же наследует класс QTreeView)
	* @return QTreeView*
	*/
	virtual QTreeView *instance() = 0;
	/**
	* @brief возвращает модель ростера
	* @return IRostersModel*
	*/
	virtual IRostersModel *rostersModel() const =0;
	/**
	* @brief устанавливает модель ростера
	*/
	virtual void setRostersModel(IRostersModel *AModel) =0;
	/**
	* @brief перерисовывает элемент ростера
	* @param AIndex индекс модели, который надо перерисовать
	* @return bool
	*/
	virtual bool repaintRosterIndex(IRosterIndex *AIndex) =0;
	/**
	* @brief раскрывает всех родителей индекса в дереве
	* @param AIndex индекс модели
	*/
	virtual void expandIndexParents(IRosterIndex *AIndex) =0;
	/**
	* @brief раскрывает всех родителей индекса в дереве
	* @param AIndex индекс модели
	*/
	virtual void expandIndexParents(const QModelIndex &AIndex) =0;
	//--ProxyModels
	/**
	* @brief вставка прокси-модели (для сортировки/фильтрования)
	* @param AProxyModel вставляемая модель
	* @param AOrder порядок модели (приоритет)
	*/
	virtual void insertProxyModel(QAbstractProxyModel *AProxyModel, int AOrder) =0;
	/**
	* @brief возвращает список установленных прокси-моделей
	* @return QList<QAbstractProxyModel *>
	*/
	virtual QList<QAbstractProxyModel *> proxyModels() const =0;
	/**
	* @brief удаляет прокси-модель
	* @param AProxyModel модель, которую следует удалить
	*/
	virtual void removeProxyModel(QAbstractProxyModel *AProxyModel) =0;
	virtual QModelIndex mapToModel(const QModelIndex &AProxyIndex) const=0;
	virtual QModelIndex mapFromModel(const QModelIndex &AModelIndex) const=0;
	virtual QModelIndex mapToProxy(QAbstractProxyModel *AProxyModel, const QModelIndex &AModelIndex) const=0;
	virtual QModelIndex mapFromProxy(QAbstractProxyModel *AProxyModel, const QModelIndex &AProxyIndex) const=0;
	//--IndexLabel
	virtual int createIndexLabel(int AOrder, const QVariant &ALabel, int AFlags = 0) =0;
	virtual void updateIndexLabel(int ALabelId, const QVariant &ALabel, int AFlags = 0) =0;
	virtual void insertIndexLabel(int ALabelId, IRosterIndex *AIndex) =0;
	virtual void removeIndexLabel(int ALabelId, IRosterIndex *AIndex) =0;
	virtual void destroyIndexLabel(int ALabelId) =0;
	virtual int labelAt(const QPoint &APoint, const QModelIndex &AIndex) const =0;
	virtual QRect labelRect(int ALabeld, const QModelIndex &AIndex) const =0;
	//--IndexNotify
	virtual int appendNotify(QList<IRosterIndex *> AIndexes, int AOrder, const QIcon &AIcon, const QString &AToolTip, int AFlags=0) =0;
	virtual QList<int> indexNotifies(IRosterIndex *Index, int AOrder) const =0;
	virtual void updateNotify(int ANotifyId, const QIcon &AIcon, const QString &AToolTip, int AFlags=0) =0;
	virtual void removeNotify(int ANotifyId) =0;
	//--ClickHookers
	/**
	* @brief вставка обработчика кликов
	* @param AOrder порядок
	* @param AHooker обработчик (должен реализовывать интерфейс @ref IRostersClickHooker)
	*/
	virtual void insertClickHooker(int AOrder, IRostersClickHooker *AHooker) =0;
	/**
	* @brief удаление обработчика кликов
	* @param AOrder порядок
	* @param AHooker обработчик (должен реализовывать интерфейс @ref IRostersClickHooker)
	*/
	virtual void removeClickHooker(int AOrder, IRostersClickHooker *AHooker) =0;
	//--DragDrop
	/**
	* @brief вставка обработчика драг-н-дропов
	* @param AHandler обработчик (должен реализовывать интерфейс @ref IRostersDragDropHandler)
	*/
	virtual void insertDragDropHandler(IRostersDragDropHandler *AHandler) =0;
	/**
	* @brief удаление обработчика драг-н-дропов
	* @param AHandler обработчик (должен реализовывать интерфейс @ref IRostersDragDropHandler)
	*/
	virtual void removeDragDropHandler(IRostersDragDropHandler *AHandler) =0;
	//--FooterText
	virtual void insertFooterText(int AOrderAndId, const QVariant &AValue, IRosterIndex *AIndex) =0;
	virtual void removeFooterText(int AOrderAndId, IRosterIndex *AIndex) =0;
	//--ContextMenu
	/**
	* @brief
	* @param AIndex
	* @param ALabelId
	* @param AMenu
	*/
	virtual void contextMenuForIndex(IRosterIndex *AIndex, int ALabelId, Menu *AMenu) =0;
	//--ClipboardMenu
	virtual void clipboardMenuForIndex(IRosterIndex *AIndex, Menu *AMenu) =0;
	//--Others
	virtual void selectFirstItem() = 0;
protected:
	virtual void modelAboutToBeSet(IRostersModel *AIndex) = 0;
	virtual void modelSet(IRostersModel *AIndex) = 0;
	virtual void proxyModelAboutToBeInserted(QAbstractProxyModel *AProxyModel, int AOrder) =0;
	virtual void proxyModelInserted(QAbstractProxyModel *AProxyModel) =0;
	virtual void proxyModelAboutToBeRemoved(QAbstractProxyModel *AProxyModel) =0;
	virtual void proxyModelRemoved(QAbstractProxyModel *AProxyModel) =0;
	virtual void viewModelAboutToBeChanged(QAbstractItemModel *AModel) =0;
	virtual void viewModelChanged(QAbstractItemModel *AModel) =0;
	virtual void indexContextMenu(IRosterIndex *AIndex, Menu *AMenu) =0;
	virtual void indexClipboardMenu(IRosterIndex *AIndex, Menu *AMenu) =0;
	virtual void labelContextMenu(IRosterIndex *AIndex, int ALabelId, Menu *AMenu) =0;
	virtual void labelToolTips(IRosterIndex *AIndex, int ALabelId, QMultiMap<int,QString> &AToolTips, ToolBarChanger * AToolBarChanger) =0;
	virtual void labelClicked(IRosterIndex *AIndex, int ALabelId) =0;
	virtual void labelDoubleClicked(IRosterIndex *AIndex, int ALabelId, bool &AAccepted) =0;
	virtual void notifyContextMenu(IRosterIndex *AIndex, int ANotifyId, Menu *AMenu) =0;
	virtual void notifyActivated(IRosterIndex *AIndex, int ANotifyId) =0;
	virtual void notifyRemovedByIndex(IRosterIndex *AIndex, int ANotifyId) =0;
	virtual void dragDropHandlerInserted(IRostersDragDropHandler *AHandler) =0;
	virtual void dragDropHandlerRemoved(IRostersDragDropHandler *AHandler) =0;
};

//! интерфейс плагина для вью ростера
class IRostersViewPlugin
{
public:
	/**
	* @brief возвращает указатель на QObject для использования сигналов/слотов
	* @return QObject*
	*/
	virtual QObject *instance() = 0;
	/**
	* @brief возвращает указатель на @ref IRostersView
	* @return IRostersView*
	*/
	virtual IRostersView *rostersView() =0;
	virtual void startRestoreExpandState() =0;
	virtual void restoreExpandState(const QModelIndex &AParent = QModelIndex()) =0;
};

Q_DECLARE_INTERFACE(IRostersClickHooker,"Virtus.Plugin.IRostersClickHooker/1.0");
Q_DECLARE_INTERFACE(IRostersDragDropHandler,"Virtus.Plugin.IRostersDragDropHandler/1.0");
Q_DECLARE_INTERFACE(IRostersView,"Virtus.Plugin.IRostersView/1.0");
Q_DECLARE_INTERFACE(IRostersViewPlugin,"Virtus.Plugin.IRostersViewPlugin/1.0");

#endif
