#ifndef MACINTEGRATION_P_H
#define MACINTEGRATION_P_H

#include <QObject>

// private class
class MacIntegrationPrivate : public QObject
{
	Q_OBJECT
	friend class MacIntegrationPlugin;
private:
	MacIntegrationPrivate();
public:
	~MacIntegrationPrivate();
	static MacIntegrationPrivate * instance();
signals:
	void dockClicked();
public:
	void emitClick();
	void setDockBadge(const QString & badgeText);
private:
	static MacIntegrationPrivate * _instance;
};

#endif // MACINTEGRATION_P_H
