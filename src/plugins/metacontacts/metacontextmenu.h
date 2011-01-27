#ifndef METACONTEXTMENU_H
#define METACONTEXTMENU_H

#include <definitions/rosterindextyperole.h>
#include <definitions/rosterlabelorders.h>
#include <interfaces/irostersview.h>
#include <interfaces/irostersmodel.h>
#include <interfaces/imessagewidgets.h>
#include <interfaces/imetacontacts.h>
#include <utils/menu.h>

class MetaContextMenu :
	public Menu
{
	Q_OBJECT;
public:
	MetaContextMenu(IRostersModel *AModel, IRostersView *AView, IMetaTabWindow *AWindow);
	~MetaContextMenu();
protected:
	bool isAcceptedIndex(IRosterIndex *AIndex);
	void updateMenu();
protected slots:
	void onAboutToShow();
	void onAboutToHide();
	void onRosterIndexInserted(IRosterIndex *AIndex);
	void onRosterIndexDataChanged(IRosterIndex *AIndex, int ARole);
	void onRosterIndexRemoved(IRosterIndex *AIndex);
private:
	IRosterIndex *FRosterIndex;
	IRostersModel *FRostersModel;
	IRostersView *FRostersView;
	IMetaTabWindow *FMetaTabWindow;
};

#endif // METACONTEXTMENU_H
