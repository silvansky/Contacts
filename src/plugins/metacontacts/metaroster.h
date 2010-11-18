#ifndef METAROSTER_H
#define METAROSTER_H

#include <interfaces/imetacontacts.h>
#include <interfaces/istanzaprocessor.h>

class MetaRoster : 
	public QObject,
	public IMetaRoster
{
	Q_OBJECT;
	Q_INTERFACES(IMetaRoster);
public:
	MetaRoster(IRoster *ARoster, IStanzaProcessor *AStanzaProcessor);
	~MetaRoster();
	virtual QObject *instance() { return this; }
	//IMetaRoster
	virtual IRoster *roster() const;
private:
	IRoster *FRoster;
	IStanzaProcessor *FStanzaProcessor;
};

#endif // METAROSTER_H
