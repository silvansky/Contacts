#ifndef METACONTEXTMENU_H
#define METACONTEXTMENU_H

#include <definitions/rosterindextyperole.h>
#include <definitions/rosterlabelorders.h>
#include <interfaces/irostersview.h>
#include <interfaces/irostersmodel.h>
#include <interfaces/imessagewidgets.h>
#include <interfaces/imetacontacts.h>
#include <interfaces/ivcard.h>
#include <interfaces/irosterchanger.h>
#include <utils/menu.h>

class MetaContextMenu :
	public Menu
{
	Q_OBJECT
public:
	MetaContextMenu(IRostersModel *AModel, IMetaContacts *AMetaContacts, IMetaTabWindow *AWindow);
	~MetaContextMenu();
protected:
	bool isAcceptedIndex(IRosterIndex *AIndex);
	void updateMenu();
protected slots:
	void onRosterIndexInserted(IRosterIndex *AIndex);
	void onRosterIndexDataChanged(IRosterIndex *AIndex, int ARole);
	void onRosterIndexRemoved(IRosterIndex *AIndex);
	void onContactInformationAction();
	void onCopyInfoAction();
	void onRenameAction();
private:
	IRosterIndex *FRosterIndex;
	IRostersModel *FRostersModel;
	IMetaTabWindow *FMetaTabWindow;
	IMetaContacts *FMetaContacts;
};

#endif // METACONTEXTMENU_H
