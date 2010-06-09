#ifndef IFILETRANSFER_H
#define IFILETRANSFER_H

#include <QString>

class Jid;
class IFileStream;

#define FILETRANSFER_UUID     "{344ce5b1-5ea5-4316-a0cd-bf51543e38f1}"

class IFileTransfer
{
public:
	virtual QObject *instance() =0;
	virtual bool isSupported(const Jid &AStreamJid, const Jid &AContactJid) const =0;
	virtual IFileStream *sendFile(const Jid &AStreamJid, const Jid &AContactJid, const QString &AFileName = QString::null, const QString &AFileDesc = QString::null) =0;
};

Q_DECLARE_INTERFACE(IFileTransfer,"Virtus.Plugin.IFileTransfer/1.0")

#endif // IFILETRANSFER_H
