#ifndef TUNNELTHREAD_H
#define TUNNELTHREAD_H

#include <QThread>
#include <QSslSocket>
#include <QNetworkProxy>

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
	QString sessionKey() const; // !!Call only after connected or disconnected signals
signals:
	void connected(const QString &AKey);
	void disconnected(const QString &ACondition);
protected:
	virtual void run();
	void setErrorCondition(const QString &ACondition); 
private:
	int FThreadState;
	QString FSessionKey;
	QString FErrorCondition;
	ConnectRequest FRequest;
};

class TunnelDataDispetcher :
	public QObject
{
	Q_OBJECT;
public:
	TunnelDataDispetcher(QSslSocket *AProxy, QSslSocket *ARemote);
public slots:
	void onProxyReadyRead();
	void onRemoteReadyRead();
private:
	QSslSocket *FProxy;
	QSslSocket *FRemote;
};

#endif // TUNNELTHREAD_H
