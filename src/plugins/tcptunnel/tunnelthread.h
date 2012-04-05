#ifndef TUNNELTHREAD_H
#define TUNNELTHREAD_H

#include <QMutex>
#include <QThread>
#include <QSslSocket>
#include <QNetworkProxy>
#include <QWaitCondition>

struct ConnectRequest
{
	bool isValid() {
		return !remoteHost.isEmpty() && remotePort>0 && !proxyHost.isEmpty() && proxyPort>0;
	}
	QString remoteHost;
	quint16 remotePort;
	bool remoteEncrypted;
	QString proxyHost;
	quint16 proxyPort;
	bool proxyEncrypted;
	QNetworkProxy connectProxy;
};

class TunnelThread : 
	public QThread
{
	Q_OBJECT;
public:
	TunnelThread(const ConnectRequest &ARequest, QObject *AParent);
	~TunnelThread();
	void abort();
signals:
	void connected(const QString &AKey);
	void disconnected(const QString &ACondition);
protected:
	virtual void run();
	void setErrorCondition(const QString &ACondition); 
protected slots:
	void onProxyReadyRead();
	void onRemoteReadyRead();
private:
	int FThreadState;
	QString FSessionKey;
	QString FErrorCondition;
	ConnectRequest FRequest;
private:
	QSslSocket *FProxySocket;
	QSslSocket *FRemoteSocket;
};

#endif // TUNNELTHREAD_H
