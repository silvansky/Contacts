#ifndef NOTIFYKINDSWIDGETS_H
#define NOTIFYKINDSWIDGETS_H

#include <QWidget>
#include <interfaces/inotifications.h>
#include <interfaces/ioptionsmanager.h>

class NotifyKindsWidgets :
		public QWidget,
		public IOptionsWidget

{
	Q_OBJECT
	Q_INTERFACES(IOptionsWidget)
public:
	NotifyKindsWidgets(QWidget * parent);
	virtual QWidget* instance() { return this; }
	void addWidget(IOptionsWidget * widget);
public slots:
	virtual void apply();
	virtual void reset();
signals:
	void modified();
	void updated();
	void childApply();
	void childReset();
};

#endif // NOTIFYKINDSWIDGETS_H
