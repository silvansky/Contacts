#include "defaultconnection.h"

#include <QNetworkProxy>
#include <utils/log.h>

#define START_QUERY_ID        0
#define STOP_QUERY_ID         -1

#define DISCONNECT_TIMEOUT    5000

DefaultConnection::DefaultConnection(IConnectionPlugin *APlugin, QObject *AParent) : QObject(AParent)
{
	FPlugin = APlugin;

	FSrvQueryId = START_QUERY_ID;
	connect(&FDns, SIGNAL(resultsReady(int, const QJDns::Response &)),SLOT(onDnsResultsReady(int, const QJDns::Response &)));
	connect(&FDns, SIGNAL(error(int, QJDns::Error)),SLOT(onDnsError(int, QJDns::Error)));
	connect(&FDns, SIGNAL(shutdownFinished()),SLOT(onDnsShutdownFinished()));

	FSocket.setProtocol(QSsl::AnyProtocol);
	connect(&FSocket, SIGNAL(connected()), SLOT(onSocketConnected()));
	connect(&FSocket, SIGNAL(encrypted()), SLOT(onSocketEncrypted()));
	connect(&FSocket, SIGNAL(readyRead()), SLOT(onSocketReadyRead()));
	connect(&FSocket, SIGNAL(modeChanged(QSslSocket::SslMode)), SIGNAL(modeChanged(QSslSocket::SslMode)));
	connect(&FSocket, SIGNAL(error(QAbstractSocket::SocketError)), SLOT(onSocketError(QAbstractSocket::SocketError)));
	connect(&FSocket, SIGNAL(sslErrors(const QList<QSslError> &)), SLOT(onSocketSSLErrors(const QList<QSslError> &)));
	connect(&FSocket, SIGNAL(disconnected()), SLOT(onSocketDisconnected()));
}

DefaultConnection::~DefaultConnection()
{
	disconnectFromHost();
	emit connectionDestroyed();
}

bool DefaultConnection::isOpen() const
{
	return FSocket.state() == QAbstractSocket::ConnectedState;
}

bool DefaultConnection::isEncrypted() const
{
	return FSocket.isEncrypted();
}

QString DefaultConnection::errorString() const
{
	return FErrorString;
}

bool DefaultConnection::connectToHost()
{
	if (FSrvQueryId==START_QUERY_ID && FSocket.state()==QAbstractSocket::UnconnectedState)
	{
		LogDetaile(QString("[DefaultConnection] Starting connection"));
		emit aboutToConnect();

		FRecords.clear();
		FSSLError = false;
		FErrorString = QString::null;

		QString host = option(IDefaultConnection::COR_HOST).toString();
		quint16 port = option(IDefaultConnection::COR_PORT).toInt();
		QString domain = option(IDefaultConnection::COR_DOMAINE).toString();
		FSSLConnection = option(IDefaultConnection::COR_USE_SSL).toBool();
		FIgnoreSSLErrors = option(IDefaultConnection::COR_IGNORE_SSL_ERRORS).toBool();
		FChangeProxyType = option(IDefaultConnection::COR_CHANGE_PROXY_TYPE).toBool();

		QJDns::Record record;
		record.name = !host.isEmpty() ? host.toLatin1() : QByteArray("xmpp.rambler.ru");
		record.port = port;
		record.priority = 0;
		record.weight = 0;
		FRecords.append(record);

		if (host.isEmpty() && FDns.init(QJDns::Unicast, QHostAddress::Any))
		{
			LogDetaile(QString("[DefaultConnection] Starting SRV lookup on host '%1'").arg(domain));
			FDns.setNameServers(QJDns::systemInfo().nameServers);
			FSrvQueryId = FDns.queryStart(QString("_xmpp-client._tcp.%1.").arg(domain).toLatin1(),QJDns::Srv);
		}
		else
		{
			connectToNextHost();
		}
		return true;
	}
	return false;
}

void DefaultConnection::disconnectFromHost()
{
	FRecords.clear();
	if (FSocket.state() != QSslSocket::UnconnectedState)
	{
		if (FSocket.state() == QSslSocket::ConnectedState)
		{
			LogDetaile(QString("[DefaultConnection] Starting socket disconnection from host"));
			emit aboutToDisconnect();
			FSocket.flush();
			FSocket.disconnectFromHost();
		}
		else
		{
			LogError(QString("[DefaultConnection] Aborting socket connection to host"));
			FSocket.abort();
			emit disconnected();
		}
	}
	else if (FSrvQueryId != START_QUERY_ID)
	{
		LogDetaile(QString("[DefaultConnection] Shutdown SRV lookup"));
		FSrvQueryId = STOP_QUERY_ID;
		FDns.shutdown();
	}

	if (FSocket.state()!=QSslSocket::UnconnectedState && !FSocket.waitForDisconnected(DISCONNECT_TIMEOUT))
	{
		LogError(QString("[DefaultConnection] Disconnection timed out"));
		setError(tr("Disconnection timed out"));
		emit disconnected();
	}
}

qint64 DefaultConnection::write(const QByteArray &AData)
{
	return FSocket.write(AData);
}

QByteArray DefaultConnection::read(qint64 ABytes)
{
	return FSocket.read(ABytes);
}

void DefaultConnection::startClientEncryption()
{
	LogDetaile(QString("[DefaultConnection] Starting connection encryption"));
	FSocket.startClientEncryption();
}

QSsl::SslProtocol DefaultConnection::protocol() const
{
	return FSocket.protocol();
}

void DefaultConnection::setProtocol(QSsl::SslProtocol AProtocol)
{
	FSocket.setProtocol(AProtocol);
}

void DefaultConnection::addCaCertificate(const QSslCertificate &ACertificate)
{
	FSocket.addCaCertificate(ACertificate);
}

QList<QSslCertificate> DefaultConnection::caCertificates() const
{
	return FSocket.caCertificates();
}

QSslCertificate DefaultConnection::peerCertificate() const
{
	return FSocket.peerCertificate();
}

void DefaultConnection::ignoreSslErrors()
{
	FSSLError = false;
	FSocket.ignoreSslErrors();
}

QList<QSslError> DefaultConnection::sslErrors() const
{
	return FSocket.sslErrors();
}

QNetworkProxy DefaultConnection::proxy() const
{
	return FSocket.proxy();
}

void DefaultConnection::setProxy(const QNetworkProxy &AProxy)
{
	if (AProxy!= FSocket.proxy())
	{
		LogDetaile(QString("[DefaultConnection] Connection proxy changed, host='%1', port='%2', type='%3'").arg(AProxy.hostName()).arg(AProxy.port()).arg(AProxy.type()));
		FSocket.setProxy(AProxy);
		emit proxyChanged(AProxy);
	}
}

QVariant DefaultConnection::option(int ARole) const
{
	return FOptions.value(ARole);
}

void DefaultConnection::setOption(int ARole, const QVariant &AValue)
{
	FOptions.insert(ARole, AValue);
}

QString DefaultConnection::localAddress()
{
	QHostAddress hostAddress = FSocket.localAddress();
	if(hostAddress != QHostAddress::Null)
		return hostAddress.toString();
	return QString::null;
}

void DefaultConnection::connectToNextHost()
{
	if (!FRecords.isEmpty())
	{
		QJDns::Record record = FRecords.takeFirst();

		while (record.name.endsWith('.'))
			record.name.chop(1);

		if (FChangeProxyType && FSocket.proxy().type()!=QNetworkProxy::NoProxy)
		{
			QNetworkProxy httpProxy = FSocket.proxy();
			httpProxy.setType(QNetworkProxy::HttpProxy);
			FSocket.setProxy(httpProxy);
		}

		connectSocketToHost(record.name,record.port);
	}
}

void DefaultConnection::connectSocketToHost(const QString &AHost, quint16 APort)
{
	FHost = AHost;
	FPort = APort;

	if (FSSLConnection)
	{
		LogDetaile(QString("[DefaultConnection] Connecting socket to host with legacy-SSL, host='%1', port='%2'").arg(AHost).arg(APort));
		FSocket.connectToHostEncrypted(FHost, FPort);
	}
	else
	{
		LogDetaile(QString("[DefaultConnection] Connecting socket to host, host='%1', port='%2'").arg(AHost).arg(APort));
		FSocket.connectToHost(FHost, FPort);
	}
}

void DefaultConnection::setError(const QString &AError)
{
	FErrorString = AError;
	emit error(FErrorString);
}

void DefaultConnection::onDnsResultsReady(int AId, const QJDns::Response &AResults)
{
	if (FSrvQueryId == AId)
	{
		LogDetaile(QString("[DefaultConnection] SRV lookup results ready"));
		if (!AResults.answerRecords.isEmpty())
		{
			FSSLConnection = false;
			FRecords = AResults.answerRecords;
		}
		FDns.shutdown();
	}
}

void DefaultConnection::onDnsError(int AId, QJDns::Error AError)
{
	if (FSrvQueryId == AId)
	{
		LogError(QString("[DefaultConnection] Failed to lookup SRV records: %1").arg(AError));
		FDns.shutdown();
	}
}

void DefaultConnection::onDnsShutdownFinished()
{
	LogDetaile(QString("[DefaultConnection] SRV lookup finished"));
	if (FSrvQueryId != STOP_QUERY_ID)
	{
		FSrvQueryId = START_QUERY_ID;
		connectToNextHost();
	}
	else
	{
		FSrvQueryId = START_QUERY_ID;
		emit disconnected();
	}
}

void DefaultConnection::onSocketConnected()
{
	LogDetaile(QString("[DefaultConnection] Socket connected to host"));
	if (!FSSLConnection)
	{
		FRecords.clear();
		emit connected();
	}
}

void DefaultConnection::onSocketEncrypted()
{
	LogDetaile(QString("[DefaultConnection] Socket connection encrypted"));
	emit encrypted();
	if (FSSLConnection)
	{
		FRecords.clear();
		emit connected();
	}
}

void DefaultConnection::onSocketReadyRead()
{
	emit readyRead(FSocket.bytesAvailable());
}

void DefaultConnection::onSocketSSLErrors(const QList<QSslError> &AErrors)
{
	FSSLError = true;
	if (!FIgnoreSSLErrors)
		emit sslErrors(AErrors);
	else
		ignoreSslErrors();
}

void DefaultConnection::onSocketError(QAbstractSocket::SocketError AError)
{
	Q_UNUSED(AError);
	LogError(QString("[DefaultConnection] Socket connection error: %1").arg(FSocket.errorString()));
	if (FChangeProxyType && FSocket.proxy().type()==QNetworkProxy::HttpProxy)
	{
		QNetworkProxy socksProxy = FSocket.proxy();
		socksProxy.setType(QNetworkProxy::Socks5Proxy);
		FSocket.setProxy(socksProxy);
		connectSocketToHost(FHost,FPort);
	}
	else if (FRecords.isEmpty())
	{
		if (FSocket.state()!=QSslSocket::ConnectedState || FSSLError)
		{
			setError(FSocket.errorString());
			emit disconnected();
		}
		else
		{
			setError(FSocket.errorString());
		}
	}
	else
	{
		connectToNextHost();
	}
}

void DefaultConnection::onSocketDisconnected()
{
	LogDetaile(QString("[DefaultConnection] Socket disconnected from host"));
	emit disconnected();
}
