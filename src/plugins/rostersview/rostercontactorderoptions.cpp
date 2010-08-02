#include "rostercontactorderoptions.h"

RosterContactOrderOptions::RosterContactOrderOptions(QWidget *AParent) : QWidget(AParent)
{
	ui.setupUi(this);

	connect(ui.rbtOrderByName,SIGNAL(toggled(bool)),SIGNAL(modified()));
	connect(ui.rbtOrderByStatus,SIGNAL(toggled(bool)),SIGNAL(modified()));
	connect(ui.rbtOrderManualy,SIGNAL(toggled(bool)),SIGNAL(modified()));

	reset();
}

RosterContactOrderOptions::~RosterContactOrderOptions()
{

}

void RosterContactOrderOptions::apply()
{
	if (ui.rbtOrderByStatus->isChecked())
		Options::node(OPV_ROSTER_SORTBYSTATUS).setValue(true);
	else
		Options::node(OPV_ROSTER_SORTBYSTATUS).setValue(false);

	emit childApply();

}

void RosterContactOrderOptions::reset()
{
	if (Options::node(OPV_ROSTER_SORTBYSTATUS).value().toBool())
		ui.rbtOrderByStatus->setChecked(true);
	else
		ui.rbtOrderByName->setChecked(true);

	emit childReset();
}
