#include "contactselector.h"

ContactSelector::ContactSelector(QWidget *parent)
	: QWidget(parent)
{
	ui.setupUi(this);

	connect(ui.listContacts, SIGNAL(itemDoubleClicked(QListWidgetItem*)), this, SLOT(contactChanging(QListWidgetItem*)));


	//updateList();
}

ContactSelector::~ContactSelector()
{

}

void ContactSelector::updateList()
{

}

Jid ContactSelector::streamJid() const
{
	return _streamJid;
}

QString ContactSelector::metaId() const
{
	return _metaId;
}

void ContactSelector::setStreamJid( Jid streamJidParam )
{
	_streamJid = streamJidParam;
}

void ContactSelector::setMetaId( QString metaIdParam )
{
	_metaId = metaIdParam;
}

void ContactSelector::contactChanging(QListWidgetItem* item)
{
	emit contactSelected(item->text());
}

void ContactSelector::setStringList( QStringList contacts )
{
	foreach(QString str, contacts)
	{
		QListWidgetItem *pItem = new QListWidgetItem(str);
		ui.listContacts->addItem(pItem);
	}
}

Action* ContactSelector::action() const
{
	return _action;
}

void ContactSelector::setAction( Action * actionparam )
{
	_action = actionparam;
}
