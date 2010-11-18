#include "metaroster.h"

MetaRoster::MetaRoster(IRoster *ARoster, IStanzaProcessor *AStanzaProcessor) : QObject(ARoster->instance())
{
	FRoster = ARoster;
	FStanzaProcessor = AStanzaProcessor;
}

MetaRoster::~MetaRoster()
{

}

IRoster *MetaRoster::roster() const
{
	return FRoster;
}
