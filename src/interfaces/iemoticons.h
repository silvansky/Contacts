#ifndef IEMOTICONS_H
#define IEMOTICONS_H

#include <QUrl>
#include <QIcon>
#include <QString>
#include <QStringList>

#define EMOTICONS_UUID "{567dda17-ae34-4392-b6f1-d21320af994b}"

class IEmoticons
{
public:
	virtual QObject *instance() =0;
	virtual QList<QString> activeIconsets() const =0;
	virtual QUrl urlByKey(const QString &AKey) const =0;
	virtual QString keyByUrl(const QUrl &AUrl) const =0;
};

Q_DECLARE_INTERFACE(IEmoticons,"Virtus.Plugin.IEmoticons/1.0")

#endif
