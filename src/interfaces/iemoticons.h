#ifndef IEMOTICONS_H
#define IEMOTICONS_H

#include <QUrl>
#include <QMap>
#include <QIcon>
#include <QString>
#include <QStringList>
#include <QTextDocument>

#define EMOTICONS_UUID "{567dda17-ae34-4392-b6f1-d21320af994b}"

class IEmoticons
{
public:
	virtual QObject *instance() =0;
	virtual QList<QString> activeIconsets() const =0;
	virtual QUrl urlByKey(const QString &AKey) const =0;
	virtual QString keyByUrl(const QUrl &AUrl) const =0;
	virtual QMap<int, QString> findTextEmoticons(const QTextDocument *ADocument, int AStartPos=0, int ALength=-1) const =0;
	virtual QMap<int, QString> findImageEmoticons(const QTextDocument *ADocument, int AStartPos=0, int ALength=-1) const =0;
};

Q_DECLARE_INTERFACE(IEmoticons,"Virtus.Plugin.IEmoticons/1.0")

#endif
