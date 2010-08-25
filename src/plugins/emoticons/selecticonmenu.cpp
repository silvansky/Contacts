#include "selecticonmenu.h"

SelectIconMenu::SelectIconMenu(const QString &AIconset, QWidget *AParent) : Menu(AParent)
{
	FLayout = new QVBoxLayout(this);
	FLayout->setMargin(0);
	setAttribute(Qt::WA_AlwaysShowToolTips,true);
	connect(this,SIGNAL(aboutToShow()),SLOT(onAboutToShow()));

	FStorage = NULL;
	setIconset(AIconset);
}

SelectIconMenu::~SelectIconMenu()
{

}

QString SelectIconMenu::iconset() const
{
	return FStorage!=NULL ? FStorage->subStorage() : QString::null;
}

void SelectIconMenu::setIconset(const QString &ASubStorage)
{
	if (FStorage==NULL || FStorage->subStorage()!=ASubStorage)
	{
		delete FStorage;
		FStorage = new IconStorage(RSR_STORAGE_EMOTICONS,ASubStorage,this);
		FStorage->insertAutoIcon(this,FStorage->fileKeys().value(0));
	}
}

IconStorage *SelectIconMenu::iconStorage() const
{
	return FStorage;
}

QSize SelectIconMenu::sizeHint() const
{
	return FSizeHint;
}

void SelectIconMenu::onAboutToShow()
{
	QWidget *widget = new SelectIconWidget(FStorage,this);
	FLayout->addWidget(widget);
	FSizeHint = FLayout->sizeHint();
	connect(this,SIGNAL(aboutToHide()),widget,SLOT(deleteLater()));
	connect(widget,SIGNAL(iconSelected(const QString &, const QString &)),SIGNAL(iconSelected(const QString &, const QString &)));
}
