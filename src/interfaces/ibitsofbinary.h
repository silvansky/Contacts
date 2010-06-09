#ifndef IBITSOFBINARY_H
#define IBITSOFBINARY_H

#include <QString>
#include <QByteArray>
#include <QObject>

class Jid;
class Stanza;

#define BITSOFBINARY_UUID       "{2100ab72-9e46-43f9-a6c7-fd4810436a52}"

class IBitsOfBinary
{
public:
	virtual QObject *instance() =0;
	virtual QString contentIdentifier(const QByteArray &AData) const =0;
	virtual bool hasBinary(const QString &AContentId) const =0;
	virtual bool loadBinary(const QString &AContentId, const Jid &AStreamJid, const Jid &AContactJid) =0;
	virtual bool loadBinary(const QString &AContentId, QString &AType, QByteArray &AData, quint64 &AMaxAge) =0;
	virtual bool saveBinary(const QString &AContentId, const QString &AType, const QByteArray &AData, quint64 AMaxAge) =0;
	virtual bool saveBinary(const QString &AContentId, const QString &AType, const QByteArray &AData, quint64 AMaxAge, Stanza &AStanza) =0;
	virtual bool removeBinary(const QString &AContentId) =0;
protected:
	virtual void binaryCached(const QString &AContentId, const QString &AType, const QByteArray &AData, quint64 AMaxAge) =0;
	virtual void binaryError(const QString &AContentId, const QString &AError) =0;
	virtual void binaryRemoved(const QString &AContentId) =0;
};

Q_DECLARE_INTERFACE(IBitsOfBinary, "Virtus.Plugin.IBitsOfBinary/1.0")

#endif
