#include "tunnelthread.h"

#include <QStringList>
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

void TunnelThread::run()
{
	FProxySocket = new QSslSocket();
	FProxySocket->setProtocol(QSsl::AnyProtocol);
	FProxySocket->setProxy(FRequest.connectProxy);
	FProxySocket->setSocketOption(QSslSocket::KeepAliveOption,true);
	connect(FProxySocket,SIGNAL(disconnected()),this,SLOT(quit()));
	connect(FProxySocket,SIGNAL(sslErrors(const QList<QSslError> &)),FProxySocket,SLOT(ignoreSslErrors()));

	FRemoteSocket = new QSslSocket();
	FRemoteSocket->setProtocol(QSsl::AnyProtocol);
	FRemoteSocket->setProxy(FRequest.connectProxy);
	FRemoteSocket->setSocketOption(QSslSocket::KeepAliveOption,true);
	connect(FRemoteSocket,SIGNAL(disconnected()),this,SLOT(quit()));
	connect(FRemoteSocket,SIGNAL(sslErrors(const QList<QSslError> &)),FRemoteSocket,SLOT(ignoreSslErrors()));

	FThreadState = TS_PROXY_CONNECT;
	if (!FRequest.proxyEncrypted)
		FProxySocket->connectToHost(FRequest.proxyHost,FRequest.proxyPort);
	else
		FProxySocket->connectToHostEncrypted(FRequest.proxyHost,FRequest.proxyPort);

	if (FProxySocket->waitForConnected() && (!FRequest.proxyEncrypted || FProxySocket->waitForEncrypted()))
	{
		FThreadState = TS_PROXY_GETKEY;

		QByteArray readerData;
		while (!readerData.endsWith('\0') && FProxySocket->waitForReadyRead())
			readerData += FProxySocket->read(FProxySocket->bytesAvailable());

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
				FRemoteSocket->connectToHost(FRequest.remoteHost,FRequest.remotePort);
			else
				FRemoteSocket->connectToHostEncrypted(FRequest.remoteHost,FRequest.remotePort);

			if (FRemoteSocket->waitForConnected() && (!FRequest.remoteEncrypted || FRemoteSocket->waitForEncrypted()))
			{
				FThreadState = TS_CONNECTED;
				connect(FProxySocket,SIGNAL(readyRead()),SLOT(onProxyReadyRead()));
				connect(FRemoteSocket,SIGNAL(readyRead()),SLOT(onRemoteReadyRead()));
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
	FProxySocket->disconnectFromHost();
	if (FProxySocket->state()!=QAbstractSocket::UnconnectedState && !FProxySocket->waitForDisconnected())
		FProxySocket->abort();

	FRemoteSocket->disconnectFromHost();
	if (FRemoteSocket->state()!=QAbstractSocket::UnconnectedState && !FRemoteSocket->waitForDisconnected())
		FRemoteSocket->abort();

	FThreadState = TS_DISCONNECTED;
	delete FProxySocket;
	delete FRemoteSocket;

	emit disconnected(FErrorCondition);
	deleteLater();
}

void TunnelThread::setErrorCondition(const QString &ACondition)
{
	if (FErrorCondition.isEmpty())
	{
		FThreadState = TS_ERROR;
		FErrorCondition = ACondition;
	}
}

void TunnelThread::onProxyReadyRead()
{
	FRemoteSocket->write(FProxySocket->read(FProxySocket->bytesAvailable()));
	FRemoteSocket->flush();
}

void TunnelThread::onRemoteReadyRead()
{
	FProxySocket->write(FRemoteSocket->read(FRemoteSocket->bytesAvailable()));
	FProxySocket->flush();
}
