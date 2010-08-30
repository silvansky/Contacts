#ifndef OPTIONSHEADER_H
#define OPTIONSHEADER_H

#include <QFrame>
#include <definations/resources.h>
#include <interfaces/ioptionsmanager.h>
#include <utils/iconstorage.h>

class OptionsHeader : 
	public QFrame,
	public IOptionsWidget
{
	Q_OBJECT;
	Q_INTERFACES(IOptionsWidget);
public:
	OptionsHeader(const QString &AIconKey, const QString &ACaption, QWidget *AParent);
	~OptionsHeader();
	virtual QWidget *instance() { return this; }
public slots:
	virtual void apply();
	virtual void reset();
signals:
	void modified();
	void childApply();
	void childReset();
};

#endif // OPTIONSHEADER_H
