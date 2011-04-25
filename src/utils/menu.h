#ifndef MENU_H
#define MENU_H

#include <QMenu>
#include <QMultiMap>
#include "utilsexport.h"
#include "action.h"
#include "iconstorage.h"
#include <QWidgetAction>

#define AG_NULL                   -1
#define AG_DEFAULT                500

class Action;
class CustomBorderContainer;

class UTILS_EXPORT Menu :
			public QMenu
{
	Q_OBJECT
public:
	Menu(QWidget *AParent = NULL);
	~Menu();
	bool isEmpty() const;
	Action *menuAction() const;
	int actionGroup(const Action *AAction) const;
	QAction *nextGroupSeparator(int AGroup) const;
	QList<Action *> groupActions(int AGroup = AG_NULL) const;
	QList<Action *> findActions(const QMultiHash<int, QVariant> AData, bool ASearchInSubMenu = false) const;
	void addAction(Action *AAction, int AGroup = AG_DEFAULT, bool ASort = false);
	void addMenuActions(const Menu *AMenu, int AGroup = AG_DEFAULT, bool ASort = false);
	void removeAction(Action *AAction);
	void addWidgetAction(QWidgetAction * action);
	void clear();
	void setIcon(const QIcon &AIcon);
	void setIcon(const QString &AStorageName, const QString &AIconKey, int AIconIndex = 0);
	void setTitle(const QString &ATitle);
signals:
	void actionInserted(QAction *ABefour, Action *AAction, int AGroup, bool ASort);
	void actionRemoved(Action *AAction);
	void separatorInserted(Action *ABefour, QAction *ASeparator);
	void separatorRemoved(QAction *ASeparator);
	void menuDestroyed(Menu *AMenu);
protected slots:
	void onActionDestroyed(Action *AAction);
public slots:
	void onAboutToShow();
	void onAboutToHide();
protected:
	bool event(QEvent *);
private:
	Action *FMenuAction;
	IconStorage *FIconStorage;
private:
	QMultiMap<int, Action *> FActions;
	QMap<int, QAction *> FSeparators;
	CustomBorderContainer * border;
	bool menuAboutToShow;
};

#endif // MENU_H
