#include "emoticonscontainer.h"

#include <QVBoxLayout>

EmoticonsContainer::EmoticonsContainer( IEditWidget *AParent ) : QWidget(AParent->instance())
{
	FEditWidget = AParent;
	setLayout(new QVBoxLayout);
	layout()->setMargin(0);
}

IEditWidget *EmoticonsContainer::editWidget() const
{
	return FEditWidget;
}

void EmoticonsContainer::insertMenu(SelectIconMenu *AMenu)
{
	if (!FWidgets.contains(AMenu))
	{
		QPushButton *button = new QPushButton(this);
		button->setObjectName("emoticonsButton");
		button->setToolTip(tr("Add emoticon"));
		connect(button, SIGNAL(clicked()), SLOT(onShowEmoticonsMenuButtonClicked()));
		button->setFlat(true);
		FWidgets.insert(AMenu,button);
		layout()->addWidget(button);
	}
}

void EmoticonsContainer::removeMenu(SelectIconMenu *AMenu)
{
	if (FWidgets.contains(AMenu))
	{
		delete FWidgets.take(AMenu);
	}
}

void EmoticonsContainer::onShowEmoticonsMenuButtonClicked()
{
	QPushButton * button = qobject_cast<QPushButton*>(sender());
	if (button)
	{
		SelectIconMenu * menu = FWidgets.key(button, NULL);
		if (menu)
		{
			menu->showMenu(button->mapToGlobal(QPoint(button->geometry().width(), 0)), Menu::TopLeft);
		}
	}
}

void EmoticonsContainer::onMenuAboutToShow()
{
	SelectIconMenu * menu = qobject_cast<SelectIconMenu*>(sender());
	QPushButton * button = FWidgets.value(menu, NULL);
	if (button)
	{
		button->setProperty("isDown", true);
		StyleStorage::updateStyle(this);
	}
}

void EmoticonsContainer::onMenuAboutToHide()
{
	SelectIconMenu * menu = qobject_cast<SelectIconMenu*>(sender());
	QPushButton * button = FWidgets.value(menu, NULL);
	if (button)
	{
		button->setProperty("isDown", false);
		StyleStorage::updateStyle(this);
	}
}
