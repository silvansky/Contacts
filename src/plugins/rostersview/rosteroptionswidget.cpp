#include "rosteroptionswidget.h"

RosterOptionsWidget::RosterOptionsWidget(QWidget *AParent) : QWidget(AParent)
{
	ui.setupUi(this);
	
	connect(ui.rbtOrderByName,SIGNAL(toggled(bool)),SIGNAL(modified()));
	connect(ui.rbtOrderByStatus,SIGNAL(toggled(bool)),SIGNAL(modified()));
	connect(ui.rbtOrderManualy,SIGNAL(toggled(bool)),SIGNAL(modified()));

	connect(ui.rdbViewFull,SIGNAL(toggled(bool)),SIGNAL(modified()));
	connect(ui.rdbViewSimplified,SIGNAL(toggled(bool)),SIGNAL(modified()));
	connect(ui.rdbViewCompact,SIGNAL(toggled(bool)),SIGNAL(modified()));
	
	// TODO: render index in RosterIndexDelegate for translatable results
	IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->insertAutoIcon(ui.lblViewFull,MNI_ROSTERVIEW_VIEW_FULL,0,0,"pixmap");
	IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->insertAutoIcon(ui.lblViewSimplified,MNI_ROSTERVIEW_VIEW_SIMPLIFIED,0,0,"pixmap");
	IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->insertAutoIcon(ui.lblViewCompact,MNI_ROSTERVIEW_VIEW_COMPACT,0,0,"pixmap");

	reset();
}

RosterOptionsWidget::~RosterOptionsWidget()
{

}

void RosterOptionsWidget::apply()
{
	if (ui.rbtOrderByStatus->isChecked())
		Options::node(OPV_ROSTER_SORTBYSTATUS).setValue(true);
	else
		Options::node(OPV_ROSTER_SORTBYSTATUS).setValue(false);

	if (ui.rdbViewFull->isChecked())
	{
		Options::node(OPV_AVATARS_SHOW).setValue(true);
		Options::node(OPV_ROSTER_SHOWSTATUSTEXT).setValue(true);
	}
	else if (ui.rdbViewSimplified->isChecked())
	{
		Options::node(OPV_AVATARS_SHOW).setValue(true);
		Options::node(OPV_ROSTER_SHOWSTATUSTEXT).setValue(false);
	}
	else
	{
		Options::node(OPV_AVATARS_SHOW).setValue(false);
		Options::node(OPV_ROSTER_SHOWSTATUSTEXT).setValue(false);
	}
	emit childApply();
}

void RosterOptionsWidget::reset()
{
	if (Options::node(OPV_ROSTER_SORTBYSTATUS).value().toBool())
		ui.rbtOrderByStatus->setChecked(true);
	else
		ui.rbtOrderByName->setChecked(true);

	if (Options::node(OPV_ROSTER_SHOWSTATUSTEXT).value().toBool() && Options::node(OPV_AVATARS_SHOW).value().toBool())
		ui.rdbViewFull->setChecked(true);
	else if (Options::node(OPV_ROSTER_SHOWSTATUSTEXT).value().toBool())
		ui.rdbViewSimplified->setChecked(true);
	else
		ui.rdbViewCompact->setChecked(true);

	emit childReset();
}
