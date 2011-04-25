#include "menu.h"
#include <QResizeEvent>
#include "customborderstorage.h"
#include <definitions/resources.h>
#include <definitions/customborder.h>

Menu::Menu(QWidget *AParent) : QMenu(AParent)
{
	menuAboutToShow = false;
	FIconStorage = NULL;

	FMenuAction = new Action(this);
	FMenuAction->setMenu(this);

	border = CustomBorderStorage::staticStorage(RSR_STORAGE_CUSTOMBORDER)->addBorder(this, CBS_MENU);
	if (border)
	{
		setWindowFlags(Qt::Widget);
		border->setShowInTaskBar(false);
		border->setResizable(false);
		border->setMovable(false);
		border->setMinimizeButtonVisible(false);
		border->setMaximizeButtonVisible(false);
		border->setCloseButtonVisible(false);
		border->setCloseOnDeactivate(true);
		border->setStaysOnTop(true);
		if (AParent)
			connect(AParent,SIGNAL(destroyed()),SLOT(deleteLater()));
		connect(this, SIGNAL(aboutToShow()), SLOT(onAboutToShow()));
		connect(this, SIGNAL(aboutToHide()), SLOT(onAboutToHide()));
		connect(this, SIGNAL(triggered(QAction*)), SLOT(hide()));
	}

	setSeparatorsCollapsible(true);
}

Menu::~Menu()
{
	if (FIconStorage)
		FIconStorage->removeAutoIcon(this);
	emit menuDestroyed(this);
}

bool Menu::isEmpty() const
{
	return FActions.isEmpty();
}

Action *Menu::menuAction() const
{
	return FMenuAction;
}

int Menu::actionGroup(const Action *AAction) const
{
	QMultiMap<int,Action *>::const_iterator it = qFind(FActions.begin(),FActions.end(),AAction);
	if (it != FActions.constEnd())
		return it.key();
	return AG_NULL;
}

QAction *Menu::nextGroupSeparator(int AGroup) const
{
	QMultiMap<int,Action *>::const_iterator it = FActions.lowerBound(AGroup);
	if (it != FActions.end())
		return FSeparators.value(it.key());
	return NULL;
}

QList<Action *> Menu::groupActions(int AGroup) const
{
	if (AGroup == AG_NULL)
		return FActions.values();
	return FActions.values(AGroup);
}

QList<Action *> Menu::findActions(const QMultiHash<int, QVariant> AData, bool ASearchInSubMenu /*= false*/) const
{
	QList<Action *> actionList;
	QList<int> keys = AData.keys();
	foreach(Action *action,FActions)
	{
		foreach (int key, keys)
		{
			if (AData.values(key).contains(action->data(key)))
			{
				actionList.append(action);
				break;
			}
		}
		if (ASearchInSubMenu && action->menu())
			actionList += action->menu()->findActions(AData,ASearchInSubMenu);
	}
	return actionList;
}

void Menu::addAction(Action *AAction, int AGroup, bool ASort)
{
	QAction *before = NULL;
	QAction *separator = NULL;
	QMultiMap<int,Action *>::iterator it = qFind(FActions.begin(),FActions.end(),AAction);
	if (it != FActions.end())
	{
		if (FActions.values(it.key()).count() == 1)
			FSeparators.remove(it.key());
		FActions.erase(it);
		QMenu::removeAction(AAction);
	}

	it = FActions.find(AGroup);
	if (it == FActions.end())
	{
		before = nextGroupSeparator(AGroup);
		before != NULL ? QMenu::insertAction(before,AAction) : QMenu::addAction(AAction);
		separator = insertSeparator(AAction);
		FSeparators.insert(AGroup,separator);
	}
	else
	{
		if (ASort)
		{
			QList<QAction *> actionList = QMenu::actions();

			bool sortRole = true;
			QString sortString = AAction->data(Action::DR_SortString).toString();
			if (sortString.isEmpty())
			{
				sortString = AAction->text();
				sortRole = false;
			}

			for (int i = 0; !before && i<actionList.count(); ++i)
			{
				QAction *qaction = actionList.at(i);
				if (FActions.key((Action *)qaction)==AGroup)
				{
					QString curSortString = qaction->text();
					if (sortRole)
					{
						Action *action = qobject_cast<Action *>(qaction);
						if (action)
							curSortString = action->data(Action::DR_SortString).toString();
					}
					if (QString::localeAwareCompare(curSortString,sortString) > 0)
						before = actionList.at(i);
				}
			}
		}

		if (!before)
		{
			QMap<int,QAction *>::const_iterator sepIt= FSeparators.upperBound(AGroup);
			if (sepIt != FSeparators.constEnd())
				before = sepIt.value();
		}

		if (before)
			QMenu::insertAction(before,AAction);
		else
			QMenu::addAction(AAction);
	}

	FActions.insertMulti(AGroup,AAction);
	connect(AAction,SIGNAL(actionDestroyed(Action *)),SLOT(onActionDestroyed(Action *)));
	if (AAction->menu())
		connect(AAction->menu(), SIGNAL(triggered(QAction*)), SLOT(hide()));
	emit actionInserted(before,AAction,AGroup,ASort);
	if (separator)
		emit separatorInserted(AAction,separator);
}

void Menu::addMenuActions(const Menu *AMenu, int AGroup, bool ASort)
{
	foreach(Action *action,AMenu->groupActions(AGroup))
		addAction(action,AMenu->actionGroup(action),ASort);
}

void Menu::removeAction(Action *AAction)
{
	QMultiMap<int,Action *>::iterator it = qFind(FActions.begin(),FActions.end(),AAction);
	if (it != FActions.end())
	{
		disconnect(AAction,SIGNAL(actionDestroyed(Action *)),this,SLOT(onActionDestroyed(Action *)));

		if (FActions.values(it.key()).count() == 1)
		{
			QAction *separator = FSeparators.value(it.key());
			FSeparators.remove(it.key());
			QMenu::removeAction(separator);
			emit separatorRemoved(separator);
		}

		FActions.erase(it);
		QMenu::removeAction(AAction);

		emit actionRemoved(AAction);

		Menu *menu = AAction->menu();
		if (menu && menu->parent() == this)
			menu->deleteLater();
		else if (AAction->parent() == this)
			AAction->deleteLater();
	}
}

void Menu::addWidgetAction(QWidgetAction * action)
{
	QMenu::addAction(action);
}

void Menu::clear()
{
	foreach(Action *action,FActions.values())
		removeAction(action);
	QMenu::clear();
}

void Menu::setIcon(const QIcon &AIcon)
{
	setIcon(QString::null,QString::null,0);
	FMenuAction->setIcon(AIcon);
	QMenu::setIcon(AIcon);
}

void Menu::setIcon(const QString &AStorageName, const QString &AIconKey, int AIconIndex)
{
	if (!AStorageName.isEmpty() && !AIconKey.isEmpty())
	{
		FIconStorage = IconStorage::staticStorage(AStorageName);
		FIconStorage->insertAutoIcon(this,AIconKey,AIconIndex);
	}
	else if (FIconStorage)
	{
		FIconStorage->removeAutoIcon(this);
		FIconStorage = NULL;
	}
}

void Menu::setTitle(const QString &ATitle)
{
	FMenuAction->setText(ATitle);
	QMenu::setTitle(ATitle);
}

void Menu::onActionDestroyed(Action *AAction)
{
	removeAction(AAction);
}

void Menu::onAboutToShow()
{
	menuAboutToShow = true;
	setVisible(false);
//	if (border)
//	{
//		border->setGeometry(geometry());
//		border->show();
//		//setVisible(true);
//		border->adjustSize();
//		border->layout()->update();
//	}
}

void Menu::onAboutToHide()
{
	if (border)
		border->hide();
}

bool Menu::event(QEvent * evt)
{
	switch(evt->type())
	{
	case QEvent::ShowToParent:
		if (border && menuAboutToShow)
		{
			QRect geom = geometry();
			QPoint p = geom.topLeft();
			p.setX(p.x() - border->leftBorderWidth());
			p.setY(p.y() - border->topBorderWidth());
			geom.moveTopLeft(p);
			geom.setWidth(geom.width() + border->leftBorderWidth() + border->rightBorderWidth());
			geom.setHeight(geom.height() + border->topBorderWidth() + border->bottomBorderWidth());
			border->setGeometry(geom);
			border->show();
			menuAboutToShow = false;
		}
		return QMenu::event(evt);
		break;
	case QEvent::Hide:
		if (border)
		{
			border->hide();
		}
		return QMenu::event(evt);
		break;
	default:
		return QMenu::event(evt);
		break;
	}
}
