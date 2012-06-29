#ifndef PHONECALLWINDOW_H
#define PHONECALLWINDOW_H

#include <QWidget>
#include <interfaces/isipphone.h>
#include <interfaces/ipluginmanager.h>
#include "callcontrolwidget.h"
#include "ui_phonecallwindow.h"

class PhoneCallWindow : 
	public QWidget
{
	Q_OBJECT;
public:
	PhoneCallWindow(IPluginManager *APluginManager, ISipCall *ASipCall, QWidget *AParent = NULL);
	~PhoneCallWindow();
	ISipCall *sipCall() const;
public:
	QSize sizeHint() const;
protected:
	void saveWindowGeometry();
	void restoreWindowGeometryWithAnimation();
	void closeWindowWithAnimation(int ATimeout = 0);
	void setWindowGeometryWithAnimation(const QRect &AGeometry, int ADuration);
protected:
	void setWindowResizeEnabled(bool AEnabled);
protected:
	void showEvent(QShowEvent *AEvent);
protected slots:
	void onCallStateChanged(int AState);
private:
	Ui::PhoneCallWindowClass ui;
private:
	bool FFirstShow;
	bool FFirstRestore;
	CallControlWidget *FCtrlWidget;
};

#endif // PHONECALLWINDOW_H
