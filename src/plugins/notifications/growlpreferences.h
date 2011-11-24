#ifndef GROWLPREFERENCES_H
#define GROWLPREFERENCES_H

#include <QWidget>
#include <interfaces/ioptionsmanager.h>

namespace Ui {
	class GrowlPreferences;
}

class GrowlPreferences : public QWidget, public IOptionsWidget
{
	Q_OBJECT
	Q_INTERFACES(IOptionsWidget)

public:
	explicit GrowlPreferences(QWidget *parent = 0);
	~GrowlPreferences();
	// IOptionsWidget
	QWidget* instance() { return this; }
public slots:
	void apply() {}
	void reset() {}
signals:
	void modified();
	void updated();
	void childApply();
	void childReset();

	// growl preferences
	void showGrowlPreferences();

private slots:
    void onGrowlSettingsButtonClicked();

private:
	Ui::GrowlPreferences *ui;
};

#endif // GROWLPREFERENCES_H
