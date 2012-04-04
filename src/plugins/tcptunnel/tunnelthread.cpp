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
	FAbort = false;
	FRequest = ARequest;
	FThreadState = TS_INIT;

	FProxy.setProtocol(QSsl::AnyProtocol);
	FProxy.setProxy(ARequest.connectProxy);
	FProxy.setSocketOption(QSslSocket::KeepAliveOption,true);
	connect(&FProxy, SIGNAL(readyRead()), SLOT(onProxyReadyRead()));
	connect(&FProxy, SIGNAL(disconnected()), SLOT(onProxyDisconnected()));
	connect(&FProxy, SIGNAL(sslErrors(const QList<QSslError> &)), SLOT(onProxySSLErrors(const QList<QSslError> &)));

	FRemote.setProtocol(QSsl::AnyProtocol);
	FRemote.setProxy(ARequest.connectProxy);
	FRemote.setSocketOption(QSslSocket::KeepAliveOption,true);
	connect(&FRemote, SIGNAL(readyRead()), SLOT(onRemoteReadyRead()));
	connect(&FRemote, SIGNAL(disconnected()), SLOT(onRemoteDisconnected()));
	connect(&FRemote, SIGNAL(sslErrors(const QList<QSslError> &)), SLOT(onRemoteSSLErrors(const QList<QSslError> &)));
}

TunnelThread::~TunnelThread()
{

}

void TunnelThread::abort()
{
	FMutex.lock();
	FAbort = true;
	FMutex.unlock();
}

void TunnelThread::run()
{
	FThreadState = TS_PROXY_CONNECT;
	if (!FRequest.proxyEncrypted)
		FProxy.connectToHost(FRequest.proxyHost,FRequest.proxyPort);
	else
		FProxy.connectToHostEncrypted(FRequest.proxyHost,FRequest.proxyPort);

	if (FProxy.waitForConnected() && (!FRequest.proxyEncrypted || FProxy.waitForEncrypted()))
	{
		FThreadState = TS_PROXY_GETKEY;

		QStringList elemStack;
		QXmlStreamReader reader;
		while(FThreadState==TS_PROXY_GETKEY && FProxy.waitForReadyRead())
		{
			reader.addData(FProxy.read(FProxy.bytesAvailable()));
			while (!reader.atEnd())
			{
				QString elemPath = elemStack.join("/");
				if (reader.isStartElement())
				{
					elemStack.append(reader.qualifiedName().toString());
				}
				else if (reader.isCharacters())
				{
					if (elemPath == "proxy/sessionkey")
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

			if (reader.hasError() && reader.error()!=QXmlStreamReader::PrematureEndOfDocumentError)
				setErrorCondition("proxy-xml-not-well-formed");
		}

		if (FSessionKey.isEmpty())
			setErrorCondition("proxy-invalid-session-key");

		if (FThreadState == TS_REMOTE_CONNECT)
		{
			if (!FRequest.remoteEncrypted)
				FRemote.connectToHost(FRequest.remoteHost,FRequest.remotePort);
			else
				FRemote.connectToHostEncrypted(FRequest.remoteHost,FRequest.remotePort);

			if (FRemote.waitForConnected() && (!FRequest.remoteEncrypted || FRemote.waitForEncrypted()))
			{
				FThreadState = TS_CONNECTED;
				emit connected(FSessionKey);
			}
			else
			{
				setErrorCondition("remote-connect-error");
			}
		}

		while (FThreadState==TS_CONNECTED && FProxy.state()==QSslSocket::ConnectedState && FRemote.state()==QSslSocket::ConnectedState)
		{
			FMutex.lock();
			FWakeup.wait(&FMutex);

			if (FProxy.bytesAvailable() > 0)
				FRemote.write(FProxy.read(FProxy.bytesAvailable()));

			if (FRemote.bytesAvailable() > 0)
				FProxy.write(FRemote.read(FRemote.bytesAvailable()));

			if (FAbort)
				FThreadState = TS_ABORTING;

			FMutex.unlock();
		}
	}
	else
	{
		setErrorCondition("proxy-connect-error");
	}

	FThreadState = TS_DISCONNECTING;
	FProxy.disconnectFromHost();
	if (!FProxy.waitForDisconnected())
		FProxy.abort();

	FRemote.disconnectFromHost();
	if (!FRemote.waitForDisconnected())
		FRemote.abort();

	FThreadState = TS_DISCONNECTED;
	emit disconnected(FErrorCondition);
	deleteLater();
}

void TunnelThread::setErrorCondition(const QString &ACondition)
{
	FThreadState = TS_ERROR;
	FErrorCondition = ACondition;
}

void TunnelThread::onProxyReadyRead()
{
	FWakeup.wakeAll();
}

void TunnelThread::onProxySSLErrors(const QList<QSslError> &AErrors)
{
	Q_UNUSED(AErrors);
	FProxy.ignoreSslErrors();
}

void TunnelThread::onProxyDisconnected()
{
	FWakeup.wakeAll();
}

void TunnelThread::onRemoteReadyRead()
{
	FWakeup.wakeAll();
}

void TunnelThread::onRemoteSSLErrors(const QList<QSslError> &AErrors)
{
	Q_UNUSED(AErrors);
	FRemote.ignoreSslErrors();
}

void TunnelThread::onRemoteDisconnected()
{
	FWakeup.wakeAll();
}
