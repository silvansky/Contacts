#ifndef IWIN7INTEGRATION_H
#define IWIN7INTEGRATION_H

#include <QObject>

#define WIN7INTEGRATION_UUID "{ac9102d0-6218-11e1-8ce7-109add56f308}"

class IWin7Integration
{
public:
	virtual QObject * instance() = 0;
};

Q_DECLARE_INTERFACE(IWin7Integration,"Virtus.Core.IWin7Integration/1.0")


#endif // IWIN7INTEGRATION_H
