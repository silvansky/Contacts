#ifndef CONTACTSELECTOR_H
#define CONTACTSELECTOR_H

#include <QWidget>
#include "ui_contactselector.h"
#include <utils/jid.h>
#include <utils/action.h>

class ContactSelector : public QWidget
{
	Q_OBJECT

public:
	ContactSelector(QWidget *parent = 0);
	~ContactSelector();

	void updateList();
	Jid streamJid() const;
	QString metaId() const;
	void setStreamJid( Jid streamJid );
	void setMetaId( QString metaId );
	void setStringList( QStringList contacts );
	Action* action() const;
	void setAction( Action * action );

signals:
	void contactSelected(const QString&);

private slots:
	void contactChanging(QListWidgetItem*);

private:
	Jid _streamJid;
	QString _metaId;
	Action* _action;
	Ui::ContactSelector ui;
};

#endif // CONTACTSELECTOR_H
