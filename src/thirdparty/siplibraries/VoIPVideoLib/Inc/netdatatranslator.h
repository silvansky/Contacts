#ifndef NETDATATRANSLATOR_H
#define NETDATATRANSLATOR_H

#include <QThread>
#include <QUdpSocket>
#include <QMutex>

class NetDataTranslator : public QThread
{
	Q_OBJECT

public:
	NetDataTranslator(int socketDescriptor, QObject *parent);
	~NetDataTranslator();


	bool getDataPortion(QByteArray& data);

public:
	void stop() { _isStoped = true; }
	bool isStoped() const { return _isStoped; }

signals:
	void newDataArrived(const QByteArray&);
	void bufferNotEmpty();

protected slots:
	void dataArrived();

protected:
	void run();
	
private:
	QUdpSocket*_udpSocket;
	QList<QByteArray> _buffer;
	bool _isStoped;

	QMutex mutex;
};

#endif // NETDATATRANSLATOR_H
