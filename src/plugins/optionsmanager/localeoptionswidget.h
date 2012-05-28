#ifndef LANGUAGEOPTIONSWIDGET_H
#define LANGUAGEOPTIONSWIDGET_H

#include <QWidget>
#include <QRadioButton>
#include <interfaces/ioptionsmanager.h>

class LocaleOptionsWidget : 
	public QWidget,
	public IOptionsWidget
{
	Q_OBJECT;
	Q_INTERFACES(IOptionsWidget);
public:
	LocaleOptionsWidget(QWidget *AParent = NULL);
	~LocaleOptionsWidget();
	virtual QWidget* instance() { return this; }
public slots:
	virtual void apply();
	virtual void reset();
signals:
	void updated();
	void modified();
	void childApply();
	void childReset();
protected:
	QString selectedLocale() const;
	void setSelectedLocale(const QString &ALocaleName);
	QString localeString(const QString &ALocaleName) const;
private:
	QMap<QRadioButton *, QString> FLocales;
};

#endif // LANGUAGEOPTIONSWIDGET_H
