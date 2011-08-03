#ifndef EMOTICONSCONTAINER_H
#define EMOTICONSCONTAINER_H

#include <QPushButton>
#include <interfaces/imessagewidgets.h>
#include "selecticonmenu.h"

class EmoticonsContainer : 
	public QWidget
{
	Q_OBJECT;
public:
	EmoticonsContainer(IEditWidget *AParent);
	IEditWidget *editWidget() const;
	void insertMenu(SelectIconMenu *AMenu);
	void removeMenu(SelectIconMenu *AMenu);
protected slots:
	void onShowEmoticonsMenuButtonClicked();
	void onMenuAboutToShow();
	void onMenuAboutToHide();
private:
	IEditWidget *FEditWidget;
	QMap<SelectIconMenu *, QPushButton *> FWidgets;
};

#endif // EMOTICONSCONTAINER_H
