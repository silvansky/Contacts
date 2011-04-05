#include "rostercontactorderoptions.h"

RosterContactOrderOptions::RosterContactOrderOptions(QWidget *AParent) : QWidget(AParent)
{
	ui.setupUi(this);

	connect(ui.rbtOrderByName,SIGNAL(toggled(bool)),SIGNAL(modified()));
	connect(ui.rbtOrderByStatus,SIGNAL(toggled(bool)),SIGNAL(modified()));
	connect(ui.rbtOrderManualy,SIGNAL(toggled(bool)),SIGNAL(modified()));

	ui.rbtOrderManualy->setVisible(false);
	reset();
}

RosterContactOrderOptions::~RosterContactOrderOptions()
{

}

void RosterContactOrderOptions::apply()
{
	if (ui.rbtOrderByName->isChecked())
		Options::node(OPV_ROSTER_SORTBYNAME).setValue(true);
	else
		Options::node(OPV_ROSTER_SORTBYNAME).setValue(false);

	if (ui.rbtOrderByStatus->isChecked())
		Options::node(OPV_ROSTER_SORTBYSTATUS).setValue(true);
	else
		Options::node(OPV_ROSTER_SORTBYSTATUS).setValue(false);

	if (ui.rbtOrderManualy->isChecked())
		Options::node(OPV_ROSTER_SORTBYHAND).setValue(true);
	else
		Options::node(OPV_ROSTER_SORTBYHAND).setValue(false);

	emit childApply();

}

void RosterContactOrderOptions::reset()
{
	if (Options::node(OPV_ROSTER_SORTBYNAME).value().toBool())
		ui.rbtOrderByName->setChecked(true);
	else if (Options::node(OPV_ROSTER_SORTBYSTATUS).value().toBool())
		ui.rbtOrderByStatus->setChecked(true);
	else if (Options::node(OPV_ROSTER_SORTBYHAND).value().toBool())
		ui.rbtOrderManualy->setChecked(true);

	emit childReset();
}
