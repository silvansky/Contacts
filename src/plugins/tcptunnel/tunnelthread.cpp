#include "tunnelthread.h"

#include <QStringList>
#include <QApplication>
#include <QXmlStreamReader>

enum ThreadState {
	TS_INIT,
	TS_PROXY_CONNECT,
	TS_PROXY_GETKEY,
	TS_REMOTE_CONNECT,
	TS_CONNECTED,
	TS_DISCONNECTING,
	TS_DISCONNECTED,
	TS_ABORTING,
	TS_ERROR
};

TunnelThread::TunnelThread(const ConnectRequest &ARequest, QObject *AParent)	: QThread(AParent)
{
	FRequest = ARequest;
	FThreadState = TS_INIT;
}

TunnelThread::~TunnelThread()
{

}

void TunnelThread::abort()
{
	quit();
}

QString TunnelThread::sessionKey() const
{
	return FSessionKey;
}

void TunnelThread::run()
{
	QSslSocket proxySocket;
	proxySocket.setProtocol(QSsl::AnyProtocol);
	proxySocket.setProxy(FRequest.connectProxy);
	proxySocket.setSocketOption(QSslSocket::KeepAliveOption,true);
	connect(&proxySocket,SIGNAL(disconnected()),this,SLOT(quit()));
	connect(&proxySocket,SIGNAL(sslErrors(const QList<QSslError> &)),&proxySocket,SLOT(ignoreSslErrors()));

	QSslSocket remoteSocket;
	remoteSocket.setProtocol(QSsl::AnyProtocol);
	remoteSocket.setProxy(FRequest.connectProxy);
	remoteSocket.setSocketOption(QSslSocket::KeepAliveOption,true);
	connect(&remoteSocket,SIGNAL(disconnected()),this,SLOT(quit()));
	connect(&remoteSocket,SIGNAL(sslErrors(const QList<QSslError> &)),&remoteSocket,SLOT(ignoreSslErrors()));

	FThreadState = TS_PROXY_CONNECT;
	if (!FRequest.proxyEncrypted)
		proxySocket.connectToHost(FRequest.proxyHost,FRequest.proxyPort);
	else
		proxySocket.connectToHostEncrypted(FRequest.proxyHost,FRequest.proxyPort);

	if (proxySocket.waitForConnected() && (!FRequest.proxyEncrypted || proxySocket.waitForEncrypted()))
	{
		FThreadState = TS_PROXY_GETKEY;

		QByteArray readerData;
		while (!readerData.endsWith('\0') && proxySocket.waitForReadyRead())
			readerData += proxySocket.read(proxySocket.bytesAvailable());

		if (!readerData.endsWith('\0'))
			setErrorCondition("proxy-invalid-session-data");
		else
			readerData.chop(1);

		QStringList elemStack;
		QXmlStreamReader reader;
		reader.addData(readerData);
		while (FThreadState==TS_PROXY_GETKEY && !reader.atEnd() && !reader.hasError())
		{
			reader.readNext();
			QString elemPath = elemStack.join("/");
			if (reader.isStartElement())
			{
				elemStack.append(reader.qualifiedName().toString());
			}
			else if (reader.isCharacters())
			{
				if (elemPath == "proxy/session-key")
				{
					FSessionKey = reader.text().toString();
				}
			}
			else if (reader.isEndElement())
			{
				elemStack.removeLast();
			}
			else if (reader.isEndDocument())
			{
				FThreadState = TS_REMOTE_CONNECT;
			}
		}

		if (reader.hasError())
			setErrorCondition("proxy-xml-not-well-formed");
		else if (FSessionKey.isEmpty())
			setErrorCondition("proxy-invalid-session-key");

		if (FThreadState == TS_REMOTE_CONNECT)
		{
			if (!FRequest.remoteEncrypted)
				remoteSocket.connectToHost(FRequest.remoteHost,FRequest.remotePort);
			else
				remoteSocket.connectToHostEncrypted(FRequest.remoteHost,FRequest.remotePort);

			if (remoteSocket.waitForConnected() && (!FRequest.remoteEncrypted || remoteSocket.waitForEncrypted()))
			{
				FThreadState = TS_CONNECTED;
				TunnelDataDispetcher dispetcher(&proxySocket,&remoteSocket);
				emit connected(FSessionKey);
				exec();
			}
			else
			{
				setErrorCondition("remote-connect-error");
			}
		}
	}
	else
	{
		setErrorCondition("proxy-connect-error");
	}

	FThreadState = TS_DISCONNECTING;
	proxySocket.disconnectFromHost();
	if (proxySocket.state()!=QAbstractSocket::UnconnectedState && !proxySocket.waitForDisconnected())
		proxySocket.abort();

	remoteSocket.disconnectFromHost();
	if (remoteSocket.state()!=QAbstractSocket::UnconnectedState && !remoteSocket.waitForDisconnected())
		remoteSocket.abort();

	FThreadState = TS_DISCONNECTED;

	emit disconnected(FErrorCondition);
}

void TunnelThread::setErrorCondition(const QString &ACondition)
{
	if (FErrorCondition.isEmpty())
	{
		FThreadState = TS_ERROR;
		FErrorCondition = ACondition;
	}
}

TunnelDataDispetcher::TunnelDataDispetcher(QSslSocket *AProxy, QSslSocket *ARemote)
{
	FProxy = AProxy;
	FRemote = ARemote;
	connect(FProxy,SIGNAL(readyRead()),SLOT(onProxyReadyRead()));
	connect(FRemote,SIGNAL(readyRead()),SLOT(onRemoteReadyRead()));
}

void TunnelDataDispetcher::onProxyReadyRead()
{
	FRemote->write(FProxy->read(FProxy->bytesAvailable()));
	FRemote->flush();
}

void TunnelDataDispetcher::onRemoteReadyRead()
{
	FProxy->write(FRemote->read(FRemote->bytesAvailable()));
	FProxy->flush();
}
