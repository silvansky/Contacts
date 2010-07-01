#ifndef ABOUTBOX_H
#define ABOUTBOX_H

#include <QDialog>
#include <definations/version.h>
#include <interfaces/ipluginmanager.h>
#include "ui_aboutbox.h"
#include "updater.h"

class AboutBox :
			public QDialog
{
	Q_OBJECT;
public:
	AboutBox(IPluginManager *APluginManager, Updater* updater, QWidget *AParent = NULL);
	~AboutBox();

	// Параметры обновления
	void setVersion(QString);
	void setSize(int);
	//QUrl setUrl() const { return url; }
	void setDescription(QString);

protected slots:
	void onLabelLinkActivated(const QString &ALink);
	public slots:
	void startUpdate();
	void updateFinish(QString, bool);
private:
	Ui::AboutBoxClass ui;
};

#endif // ABOUTBOX_H
