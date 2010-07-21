#ifndef ROSTERTOOLTIP_H
#define ROSTERTOOLTIP_H

#include <QWidget>
#include <utils/toolbarchanger.h>
#include <interfaces/irostersview.h>

namespace Ui
{
	class RosterToolTip;
}

class RosterToolTip : public QWidget, public IRosterToolTip
{
	Q_OBJECT
public:
	RosterToolTip(QWidget *parent = 0);
	~RosterToolTip();
	//IRosterToolTip
	virtual QWidget * instance();
	virtual ToolBarChanger * sideBarChanger();
	virtual QString caption() const;
	virtual void setCaption(const QString &);
	virtual IRosterIndex * rosterIndex() const;
	virtual void setRosterIndex(IRosterIndex *);
protected:
	void changeEvent(QEvent *e);

private:
	Ui::RosterToolTip *ui;
	QToolBar * rightToolBar;
	QString tipCaption;
	ToolBarChanger * rightToolBarChanger;
	IRosterIndex * index;
};

#endif // ROSTERTOOLTIP_H
