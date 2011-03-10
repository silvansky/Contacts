#ifndef OBJECTSHUTDOWNMANAGER_H
#define OBJECTSHUTDOWNMANAGER_H

#include <QObject>
#include "RShutDownManager.h"

class ObjectShutdownManager : public QObject, public holdem_utils::RAutoShutDownManager
{
	Q_OBJECT
public:
	explicit ObjectShutdownManager(const GUID& guid);
	explicit ObjectShutdownManager(LPCTSTR name);
	virtual void OnShutDown();
signals:
	void shutdownRequested();

public slots:

};

#endif // OBJECTSHUTDOWNMANAGER_H
