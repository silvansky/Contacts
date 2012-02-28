#ifndef GROWLPREFERENCES_H
#define GROWLPREFERENCES_H

#include <QWidget>
#include <interfaces/ioptionsmanager.h>

namespace Ui {
	class SystemNotifyPreferences;
}

class SystemNotifyPreferences : public QWidget, public IOptionsWidget
{
	Q_OBJECT
	Q_INTERFACES(IOptionsWidget)

public:
	explicit SystemNotifyPreferences(QWidget *parent = 0);
	~SystemNotifyPreferences();
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
	void showPreferences();

private slots:
	void onSettingsButtonClicked();

private:
	Ui::SystemNotifyPreferences *ui;
};

#endif // GROWLPREFERENCES_H
