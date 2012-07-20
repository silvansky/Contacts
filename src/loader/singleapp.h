#ifndef SINGLEAPP_H
#define SINGLEAPP_H

#include <QApplication>
#include <QSharedMemory>
#include <QLocalServer>

// based on http://www.qtcentre.org/wiki/index.php?title=SingleApplication

class SingleApp : 
	public QApplication
{
	Q_OBJECT
public:
	SingleApp(int &argc, char *argv[], const QString uniqueKey);
	bool isRunning();
	bool sendMessage(const QString &message);
public slots:
	void receiveMessage();
signals:
	void messageAvailable(QString message);
private:
	bool _isRunning;
	QString _uniqueKey;
	QSharedMemory sharedMemory;
	QLocalServer *localServer;
	static const int timeout = 1000;
};

#endif // SINGLEAPP_H
